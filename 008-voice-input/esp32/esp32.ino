#define VOICE_INPUT_UPLOAD

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Arduino.h>
#include <ESP_I2S.h>
#include <SHA2Builder.h>
#include <SPI.h>
#include <mbedtls/base64.h>

#ifdef VOICE_INPUT_UPLOAD
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_system.h>
#include "heartbeat_protocol.h"
#include "secrets.h"
#include "recording_runtime.h"
#include "recording_upload_policy.h"
#include "wav_builder.h"
#endif

#include "button_debouncer.h"
#include "audio_envelope.h"
#include "capture_progress.h"
#include "chinese_display.h"
#include "display_scheduler.h"
#include "recording_gesture.h"
#include "serial_audio_protocol.h"
#include "wave_ring.h"

namespace {

constexpr int kMicSckPin = 4;
constexpr int kMicWsPin = 5;
constexpr int kMicSdPin = 6;
constexpr int kButtonPin = 8;
constexpr int kTftSckPin = 9;
constexpr int kTftMosiPin = 10;
constexpr int kTftResetPin = 11;
constexpr int kTftDcPin = 12;
constexpr int kTftCsPin = -1;

constexpr uint32_t kSampleRate = 16000;
constexpr uint32_t kMinimumRecordingMs = 300;
constexpr uint32_t kMaximumRecordingMs = 5000;
constexpr uint32_t kDebounceMs = 40;
constexpr std::size_t kMaximumSamples =
    kSampleRate * kMaximumRecordingMs / 1000;
constexpr std::size_t kReadSamples = 256;
constexpr uint32_t kWaveFrameMs = 40;
constexpr uint32_t kTimerFrameMs = 200;
constexpr std::size_t kWaveBars = 32;
constexpr double kNoiseFloorRms = 1500000.0;
constexpr double kEnvelopeCeilingRms = 12000000.0;
constexpr double kEnvelopeAttack = 0.60;
constexpr double kEnvelopeRelease = 0.15;
constexpr std::size_t kExportChunkBytes = 384;
#ifdef VOICE_INPUT_UPLOAD
constexpr unsigned kMaximumUploadAttempts = 3;
constexpr uint32_t kUploadTimeoutMs = 15000;
constexpr uint32_t kHeartbeatIntervalMs = 10000;
constexpr uint32_t kHeartbeatRetryIntervalMs = 1000;
#endif

I2SClass microphone;
Adafruit_ST7789 tft(&SPI, kTftCsPin, kTftDcPin, kTftResetPin);
ButtonDebouncer button(/*pressed_level=*/HIGH, kDebounceMs);
recording_gesture::Controller gesture(kMinimumRecordingMs,
                                      kMaximumRecordingMs);
audio_envelope::Smoother envelope(kEnvelopeAttack, kEnvelopeRelease);
WaveRing wave_ring(kWaveBars, /*minimum_height=*/2, /*maximum_height=*/45);
CaptureProgress capture_progress(kMaximumSamples);
DisplayScheduler display_scheduler(kWaveFrameMs, kTimerFrameMs);

int32_t *recording_buffer = nullptr;
#ifdef VOICE_INPUT_UPLOAD
uint8_t *wav_buffer = nullptr;
recording_runtime::Controller device_runtime;
uint32_t last_heartbeat_at = 0;
uint32_t next_heartbeat_attempt_at = 0;
String boot_id;
#endif
int32_t read_buffer[kReadSamples];
bool microphone_running = false;
volatile bool capture_stop_requested = false;
volatile bool capture_task_running = false;
volatile bool capture_full = false;
volatile int capture_error = 0;
volatile float latest_envelope = 0.0F;

void updateRecordingDisplay(uint32_t now_ms);

void showChineseCentered(zh_display::Message message, int16_t y,
                         uint16_t color, uint8_t scale) {
  zh_display::drawCentered(tft, zh_display::messageText(message), 240, y,
                           color, scale);
}

void showIdle(zh_display::Message detail = zh_display::Message::kIdleReady) {
  tft.fillScreen(ST77XX_BLACK);
  showChineseCentered(zh_display::Message::kIdleTitle, 48, ST77XX_CYAN, 2);
  showChineseCentered(detail, 188, ST77XX_WHITE, 1);
}

void showError(zh_display::Message detail) {
  tft.fillScreen(ST77XX_BLACK);
  showChineseCentered(zh_display::Message::kErrorTitle, 70, ST77XX_RED, 2);
  showChineseCentered(detail, 132, ST77XX_WHITE, 1);
}

void showRecordingLayout() {
  tft.fillScreen(ST77XX_BLACK);
  zh_display::drawText(tft,
                       zh_display::messageText(zh_display::Message::kRecording),
                       12, 10, ST77XX_RED, 1);

  constexpr int16_t kCenterY = 110;
  tft.drawFastHLine(8, kCenterY, 224, 0x39E7);
  showChineseCentered(zh_display::Message::kReleaseToSave, 190, ST77XX_WHITE,
                      1);
}

void showRecordingTimer(uint32_t elapsed_ms) {
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(180, 10);
  tft.printf("%lu.%lus", static_cast<unsigned long>(elapsed_ms / 1000),
             static_cast<unsigned long>((elapsed_ms % 1000) / 100));
}

void showRecordingWave(double level) {
  constexpr int16_t kCenterY = 110;
  constexpr int16_t kWaveX = 8;
  constexpr int16_t kBarWidth = 4;
  constexpr int16_t kBarStep = 7;
  constexpr int16_t kMaximumHeight = 45;
  const auto update = wave_ring.advance(level);
  const int16_t x = kWaveX + update.index * kBarStep;
  tft.fillRect(x, kCenterY - kMaximumHeight, kBarWidth,
               kMaximumHeight * 2, ST77XX_BLACK);
  tft.fillRect(x, kCenterY - update.new_height, kBarWidth,
               update.new_height * 2, ST77XX_GREEN);
}

bool startMicrophone() {
  microphone.setPins(kMicSckPin, kMicWsPin, -1, kMicSdPin);
  microphone_running = microphone.begin(
      I2S_MODE_STD, kSampleRate, I2S_DATA_BIT_WIDTH_32BIT,
      I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
  if (!microphone_running) {
    Serial.printf("ERROR code=I2S_BEGIN detail=%d\n", microphone.lastError());
  }
  return microphone_running;
}

void stopMicrophone() {
  if (microphone_running) {
    microphone.end();
    microphone_running = false;
  }
}

#ifdef VOICE_INPUT_UPLOAD
void connectWiFi() {
  showChineseCentered(zh_display::Message::kConnectingNetwork, 82,
                      ST77XX_YELLOW, 2);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const uint32_t started_ms = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started_ms < 15000) {
    delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WIFI event=CONNECTED ip=%s\n",
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WIFI event=TIMEOUT action=recording-can-retry-later");
  }
}

void initializeBootId() {
  char output[17];
  snprintf(output, sizeof(output), "%08lx%08lx",
           static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()));
  boot_id = output;
}

void maybeSendHeartbeat() {
  const uint32_t now_ms = millis();
  if (static_cast<int32_t>(now_ms - next_heartbeat_attempt_at) < 0 ||
      !heartbeat_protocol::shouldSend(now_ms, last_heartbeat_at,
                                      kHeartbeatIntervalMs,
                                      device_runtime.stateChanged()) ||
      WiFi.status() != WL_CONNECTED) {
    return;
  }

  next_heartbeat_attempt_at = now_ms + kHeartbeatRetryIntervalMs;
  HTTPClient http;
  if (!http.begin(String(VOICE_SERVER_BASE_URL) + "/api/v1/device/heartbeat")) {
    return;
  }
  http.setTimeout(1000);
  http.addHeader("Authorization", String("Bearer ") + VOICE_DEVICE_TOKEN);
  http.addHeader("Content-Type", "application/json");
  const std::string &recording_id = device_runtime.currentRecordingId();
  const String recording_value = recording_id.empty()
                                     ? "null"
                                     : String("\"") + recording_id.c_str() + "\"";
  const String body =
      String("{\"boot_id\":\"") + boot_id +
      "\",\"firmware_version\":\"0.3.1\",\"capabilities\":[\"recording\"]," +
      "\"state\":\"" + device_runtime.stateName() +
      "\",\"uptime_ms\":" + String(now_ms) +
      ",\"current_recording_id\":" + recording_value +
      ",\"current_job_id\":null,\"error_code\":null}";
  const int status_code = http.POST(body);
  http.end();
  if (status_code == HTTP_CODE_OK) {
    last_heartbeat_at = now_ms;
    device_runtime.markReported();
    Serial.printf("HEARTBEAT state=%s http=%d\n", device_runtime.stateName(),
                  status_code);
  } else {
    Serial.printf("HEARTBEAT event=ERROR state=%s http=%d\n",
                  device_runtime.stateName(), status_code);
  }
}

void makeRecordingId(char output[37]) {
  uint8_t bytes[16];
  esp_fill_random(bytes, sizeof(bytes));
  recording_upload_policy::formatUuid(bytes, output);
}

bool uploadWav(const uint8_t *wav, std::size_t wav_length,
               const char *recording_id, const String &wav_sha256) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (WiFi.status() != WL_CONNECTED) return false;

  const String url =
      String(VOICE_SERVER_BASE_URL) + "/api/v1/device/recordings";
  for (unsigned attempt = 1; attempt <= kMaximumUploadAttempts; ++attempt) {
    HTTPClient http;
    if (!http.begin(url)) {
      Serial.println("UPLOAD event=ERROR reason=invalid-url");
      return false;
    }
    http.setTimeout(kUploadTimeoutMs);
    http.addHeader("Authorization", String("Bearer ") + VOICE_DEVICE_TOKEN);
    http.addHeader("Content-Type", "audio/wav");
    http.addHeader("X-Recording-Id", recording_id);
    http.addHeader("X-Audio-Sha256", wav_sha256);
    http.addHeader("X-Sample-Rate", String(kSampleRate));
    http.addHeader("X-Channels", "1");
    const int status_code =
        http.POST(const_cast<uint8_t *>(wav), wav_length);
    Serial.printf("UPLOAD event=RESPONSE recording_id=%s attempt=%u http=%d bytes=%u\n",
                  recording_id, attempt, status_code,
                  static_cast<unsigned>(wav_length));
    http.end();
    if (status_code == HTTP_CODE_OK || status_code == HTTP_CODE_CREATED) {
      return true;
    }
    if (!recording_upload_policy::shouldRetry(
            status_code, attempt, kMaximumUploadAttempts)) {
      return false;
    }
    delay(250U * attempt);
  }
  return false;
}

void saveCapture() {
  const std::size_t sample_count = capture_progress.sampleCount();
  const std::size_t wav_capacity =
      wav_builder::requiredBytes(kMaximumSamples);
  const std::size_t wav_length = wav_builder::buildMonoPcm16(
      recording_buffer, sample_count, kSampleRate, wav_buffer, wav_capacity);
  if (wav_length == 0) {
    Serial.println("ERROR code=WAV_BUILD");
    showError(zh_display::Message::kAudioBuildFailed);
    device_runtime.fail();
    maybeSendHeartbeat();
    delay(1500);
    showIdle(zh_display::Message::kRetryRecording);
    device_runtime.returnToIdle();
    maybeSendHeartbeat();
    return;
  }

  SHA256Builder digest;
  digest.begin();
  digest.add(wav_buffer, wav_length);
  digest.calculate();
  const String wav_sha256 = digest.toString();
  char recording_id[37];
  makeRecordingId(recording_id);
  device_runtime.startUploading(recording_id);
  maybeSendHeartbeat();

  tft.fillScreen(ST77XX_BLACK);
  showChineseCentered(zh_display::Message::kUploading, 82, ST77XX_YELLOW, 2);
  showChineseCentered(zh_display::Message::kKeepConnected, 132, ST77XX_WHITE,
                      1);
  Serial.printf("UPLOAD event=START recording_id=%s samples=%u sha256=%s\n",
                recording_id, static_cast<unsigned>(sample_count),
                wav_sha256.c_str());
  if (uploadWav(wav_buffer, wav_length, recording_id, wav_sha256)) {
    device_runtime.finishUpload();
    maybeSendHeartbeat();
    tft.fillScreen(ST77XX_BLACK);
    showChineseCentered(zh_display::Message::kStored, 82, ST77XX_GREEN, 2);
    Serial.printf("UPLOAD event=STORED recording_id=%s\n", recording_id);
    delay(1200);
    showIdle(zh_display::Message::kUploadComplete);
  } else {
    device_runtime.fail();
    maybeSendHeartbeat();
    showError(zh_display::Message::kUploadFailed);
    Serial.printf("UPLOAD event=ERROR recording_id=%s action=retry-new-recording\n",
                  recording_id);
    delay(1800);
    showIdle(zh_display::Message::kNetworkRetry);
    device_runtime.returnToIdle();
    maybeSendHeartbeat();
  }
  Serial.println("STATUS=IDLE action=hold-button-to-record-again");
}
#endif

void exportRawCapture() {
#ifdef VOICE_INPUT_UPLOAD
  saveCapture();
#else
  const std::size_t recorded_samples = capture_progress.sampleCount();
  const std::size_t byte_count = recorded_samples * sizeof(int32_t);
  SHA256Builder digest;
  digest.begin();
  digest.add(reinterpret_cast<const uint8_t *>(recording_buffer), byte_count);
  digest.calculate();
  const String sha256 = digest.toString();

  tft.fillScreen(ST77XX_BLACK);
  showChineseCentered(zh_display::Message::kExporting, 82, ST77XX_YELLOW, 2);
  showChineseCentered(zh_display::Message::kKeepConnected, 132, ST77XX_WHITE,
                      1);

  Serial.printf("AUDIO_BEGIN sample_rate=%lu samples=%u sha256=%s\n",
                static_cast<unsigned long>(kSampleRate),
                static_cast<unsigned>(recorded_samples), sha256.c_str());

  const uint8_t *raw = reinterpret_cast<const uint8_t *>(recording_buffer);
  uint8_t encoded[
      serial_audio_protocol::encodedBufferSize(kExportChunkBytes)];
  for (std::size_t offset = 0; offset < byte_count;
       offset += kExportChunkBytes) {
    const std::size_t chunk =
        min(kExportChunkBytes, byte_count - offset);
    std::size_t encoded_length = 0;
    const int error = mbedtls_base64_encode(
        encoded, sizeof(encoded), &encoded_length, raw + offset, chunk);
    if (error != 0) {
      Serial.printf("ERROR code=BASE64 detail=%d offset=%u\n", error,
                    static_cast<unsigned>(offset));
      showError(zh_display::Message::kEncodingFailed);
      return;
    }
    encoded[encoded_length] = '\0';
    Serial.print("AUDIO_DATA=");
    Serial.println(reinterpret_cast<const char *>(encoded));
  }
  Serial.printf("AUDIO_END samples=%u sha256=%s\n",
                static_cast<unsigned>(recorded_samples), sha256.c_str());
  Serial.println("STATUS=IDLE action=hold-button-to-record-again");
  showIdle(zh_display::Message::kExportComplete);
#endif
}

void beginRecording(uint32_t now_ms) {
  capture_progress.reset();
  capture_stop_requested = false;
  capture_full = false;
  capture_error = 0;
  latest_envelope = 0.0F;
  envelope.reset();
  wave_ring.reset();
  if (!startMicrophone()) {
    gesture.abort();
    showError(zh_display::Message::kMicStartFailed);
#ifdef VOICE_INPUT_UPLOAD
    device_runtime.fail();
    maybeSendHeartbeat();
#endif
    return;
  }

  capture_task_running = true;
  const BaseType_t task_result = xTaskCreatePinnedToCore(
      [](void *) {
        while (!capture_stop_requested) {
          const std::size_t bytes_read = microphone.readBytes(
              reinterpret_cast<char *>(read_buffer), sizeof(read_buffer));
          if (bytes_read == 0) {
            capture_error = microphone.lastError();
            break;
          }

          const std::size_t samples_read = bytes_read / sizeof(int32_t);
          const std::size_t offset = capture_progress.sampleCount();
          const std::size_t accepted = capture_progress.reserve(samples_read);
          if (accepted > 0) {
            memcpy(recording_buffer + offset, read_buffer,
                   accepted * sizeof(int32_t));
            const double rms =
                audio_envelope::centeredRms(read_buffer, accepted);
            const double normalized = audio_envelope::normalize(
                rms, kNoiseFloorRms, kEnvelopeCeilingRms);
            latest_envelope =
                static_cast<float>(envelope.update(normalized));
          }
          if (capture_progress.full()) {
            capture_full = true;
            break;
          }
        }
        capture_task_running = false;
        vTaskDelete(nullptr);
      },
      "voice_input_capture", 4096, nullptr, 2, nullptr, 0);
  if (task_result != pdPASS) {
    capture_task_running = false;
    gesture.abort();
    stopMicrophone();
    Serial.println("ERROR code=CAPTURE_TASK_CREATE");
    showError(zh_display::Message::kCaptureTaskFailed);
#ifdef VOICE_INPUT_UPLOAD
    device_runtime.fail();
    maybeSendHeartbeat();
#endif
    return;
  }
  display_scheduler.reset(now_ms);
  Serial.println("RECORDING event=START");
#ifdef VOICE_INPUT_UPLOAD
  device_runtime.startRecording();
  maybeSendHeartbeat();
#endif
  showRecordingLayout();
  updateRecordingDisplay(now_ms);
}

void finishRecording(recording_gesture::Event event, uint32_t now_ms) {
  capture_stop_requested = true;
  const uint32_t wait_started_ms = millis();
  while (capture_task_running &&
         static_cast<uint32_t>(millis() - wait_started_ms) < 500) {
    delay(1);
  }
  stopMicrophone();
  if (capture_task_running) {
    Serial.println("ERROR code=CAPTURE_TASK_STOP_TIMEOUT");
    gesture.abort();
    showError(zh_display::Message::kCaptureStopFailed);
#ifdef VOICE_INPUT_UPLOAD
    device_runtime.fail();
    maybeSendHeartbeat();
#endif
    return;
  }
  const uint32_t elapsed_ms = gesture.elapsed(now_ms);
  const std::size_t recorded_samples = capture_progress.sampleCount();
  if (event == recording_gesture::Event::kCancelledTooShort) {
    Serial.printf("RECORDING event=CANCEL reason=too-short elapsed_ms=%lu samples=%u\n",
                  static_cast<unsigned long>(elapsed_ms),
                  static_cast<unsigned>(recorded_samples));
    showIdle(zh_display::Message::kTooShortRetry);
#ifdef VOICE_INPUT_UPLOAD
    device_runtime.returnToIdle();
    maybeSendHeartbeat();
#endif
    return;
  }

  Serial.printf("RECORDING event=%s elapsed_ms=%lu samples=%u\n",
                event == recording_gesture::Event::kForcedMaximum
                    ? "FORCED_MAXIMUM"
                    : "FINISH",
                static_cast<unsigned long>(elapsed_ms),
                static_cast<unsigned>(recorded_samples));
  exportRawCapture();
}

void updateRecordingDisplay(uint32_t now_ms) {
  const DisplayDue due = display_scheduler.poll(now_ms);
  if (due.wave) showRecordingWave(latest_envelope);
  if (due.timer) showRecordingTimer(gesture.elapsed(now_ms));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
#ifdef VOICE_INPUT_UPLOAD
  Serial.println("ESP32-S3 INMP441 Voice Input");
#else
  Serial.println("ESP32-S3 INMP441 Voice Input Diagnostic");
#endif
  Serial.println("mic=SCK:GPIO4 WS:GPIO5 SD:GPIO6 L/R:GND left-slot");
  Serial.println("button=GPIO8 released:LOW pressed:HIGH debounce_ms=40");
  Serial.println("screen=SCK:GPIO9 MOSI:GPIO10 RST:GPIO11 DC:GPIO12 MODE3 1MHz");

  if (!psramFound()) {
    Serial.println("ERROR code=PSRAM_NOT_FOUND");
    while (true) delay(1000);
  }
  recording_buffer = static_cast<int32_t *>(
      ps_malloc(kMaximumSamples * sizeof(int32_t)));
  if (recording_buffer == nullptr) {
    Serial.println("ERROR code=PSRAM_ALLOC");
    while (true) delay(1000);
  }
#ifdef VOICE_INPUT_UPLOAD
  wav_buffer = static_cast<uint8_t *>(
      ps_malloc(wav_builder::requiredBytes(kMaximumSamples)));
  if (wav_buffer == nullptr) {
    Serial.println("ERROR code=WAV_PSRAM_ALLOC");
    while (true) delay(1000);
  }
#endif

  SPI.begin(kTftSckPin, -1, kTftMosiPin, kTftCsPin);
  tft.init(240, 240, SPI_MODE3);
  tft.setSPISpeed(1000000);
  tft.setRotation(2);

#ifdef VOICE_INPUT_UPLOAD
  initializeBootId();
  connectWiFi();
  maybeSendHeartbeat();
#endif

  pinMode(kButtonPin, INPUT);
  const bool raw_level = digitalRead(kButtonPin) == HIGH;
  button.begin(raw_level, millis());
  showIdle();
  Serial.printf("STATUS=IDLE button_level=%s action=hold-button-to-record\n",
                raw_level ? "HIGH" : "LOW");
}

void loop() {
  const uint32_t now_ms = millis();
#ifdef VOICE_INPUT_UPLOAD
  maybeSendHeartbeat();
#endif
  const bool raw_level = digitalRead(kButtonPin) == HIGH;
  if (button.update(raw_level, now_ms)) {
    if (button.changedToPressed()) {
      const auto event = gesture.onPressed(now_ms);
      if (event == recording_gesture::Event::kStarted) beginRecording(now_ms);
    } else if (button.changedToReleased()) {
      const auto event = gesture.onReleased(now_ms);
      if (event != recording_gesture::Event::kNone) {
        finishRecording(event, now_ms);
      }
    }
  }

  const auto timeout_event = gesture.onTick(now_ms);
  if (timeout_event == recording_gesture::Event::kForcedMaximum) {
    finishRecording(timeout_event, now_ms);
  }

  if (gesture.isRecording() && capture_error != 0) {
    const int error = capture_error;
    capture_stop_requested = true;
    gesture.abort();
    stopMicrophone();
    Serial.printf("ERROR code=I2S_READ detail=%d\n", error);
    showError(zh_display::Message::kMicReadFailed);
#ifdef VOICE_INPUT_UPLOAD
    device_runtime.fail();
    maybeSendHeartbeat();
#endif
  } else if (gesture.isRecording() && microphone_running) {
    updateRecordingDisplay(now_ms);
    if (capture_full) {
      const auto full_event = gesture.forceMaximum();
      if (full_event != recording_gesture::Event::kNone) {
        finishRecording(full_event, millis());
      }
    }
  } else {
    delay(1);
  }
}

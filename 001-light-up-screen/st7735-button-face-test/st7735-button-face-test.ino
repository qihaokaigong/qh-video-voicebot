#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#include "face_logic.h"

constexpr int TFT_SCLK = 12;
constexpr int TFT_MOSI = 11;
constexpr int TFT_RST = 14;
constexpr int TFT_DC = 13;
constexpr int TFT_CS = 10;

constexpr int BUTTON_PIN = 8;
constexpr unsigned long DEBOUNCE_MS = 30;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

int lastRawButton = LOW;
int stableButton = LOW;
unsigned long lastButtonChangeMs = 0;

void drawFace(FaceExpression expression) {
  const uint16_t background = tft.color565(18, 30, 48);
  const uint16_t faceColor = tft.color565(255, 210, 45);

  tft.fillScreen(background);

  // 128x160 portrait layout.
  tft.fillCircle(64, 80, 52, faceColor);
  tft.drawCircle(64, 80, 52, ST77XX_WHITE);
  tft.drawCircle(64, 80, 51, ST77XX_WHITE);

  tft.fillCircle(45, 65, 6, ST77XX_BLACK);
  tft.fillCircle(83, 65, 6, ST77XX_BLACK);
  tft.fillCircle(43, 63, 2, ST77XX_WHITE);
  tft.fillCircle(81, 63, 2, ST77XX_WHITE);

  if (expression == FaceExpression::Smile) {
    tft.fillCircle(32, 91, 7, ST77XX_RED);
    tft.fillCircle(96, 91, 7, ST77XX_RED);

    // A filled mouth makes the smile easy to see on the small display.
    tft.fillRoundRect(40, 93, 48, 24, 10, ST77XX_BLACK);
    tft.fillRect(40, 93, 48, 10, faceColor);
    tft.drawLine(45, 105, 52, 111, ST77XX_WHITE);
    tft.drawLine(52, 111, 64, 114, ST77XX_WHITE);
    tft.drawLine(64, 114, 76, 111, ST77XX_WHITE);
    tft.drawLine(76, 111, 83, 105, ST77XX_WHITE);
  } else {
    tft.fillRoundRect(44, 101, 40, 5, 2, ST77XX_BLACK);
  }
}

void showExpression(FaceExpression expression) {
  drawFace(expression);
  Serial.println(expression == FaceExpression::Smile ? "Face: SMILE"
                                                       : "Face: NEUTRAL");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(BUTTON_PIN, INPUT);

  tft.initR(INITR_GREENTAB);
  tft.setRotation(0);

  lastRawButton = digitalRead(BUTTON_PIN);
  stableButton = lastRawButton;
  showExpression(faceExpressionForPressed(stableButton == HIGH));

  Serial.println("ST7735 face screen ready. Hold the button to smile.");
}

void loop() {
  const int rawButton = digitalRead(BUTTON_PIN);

  if (rawButton != lastRawButton) {
    lastRawButton = rawButton;
    lastButtonChangeMs = millis();
  }

  if ((millis() - lastButtonChangeMs) >= DEBOUNCE_MS &&
      rawButton != stableButton) {
    stableButton = rawButton;
    showExpression(faceExpressionForPressed(stableButton == HIGH));
  }

  delay(5);
}

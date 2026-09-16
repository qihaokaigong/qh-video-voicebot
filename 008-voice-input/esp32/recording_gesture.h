#pragma once

#include <cstdint>

namespace recording_gesture {

enum class Event {
  kNone,
  kStarted,
  kCancelledTooShort,
  kFinished,
  kForcedMaximum,
};

class Controller {
 public:
  Controller(uint32_t minimum_ms, uint32_t maximum_ms)
      : minimum_ms_(minimum_ms), maximum_ms_(maximum_ms) {}

  Event onPressed(uint32_t now_ms) {
    if (recording_) return Event::kNone;
    recording_ = true;
    started_at_ms_ = now_ms;
    return Event::kStarted;
  }

  Event onReleased(uint32_t now_ms) {
    if (!recording_) return Event::kNone;
    recording_ = false;
    return elapsed(now_ms) < minimum_ms_ ? Event::kCancelledTooShort
                                         : Event::kFinished;
  }

  Event onTick(uint32_t now_ms) {
    if (!recording_ || elapsed(now_ms) < maximum_ms_) return Event::kNone;
    return forceMaximum();
  }

  Event forceMaximum() {
    if (!recording_) return Event::kNone;
    recording_ = false;
    return Event::kForcedMaximum;
  }

  void abort() { recording_ = false; }

  bool isRecording() const { return recording_; }
  uint32_t elapsed(uint32_t now_ms) const {
    return static_cast<uint32_t>(now_ms - started_at_ms_);
  }

 private:
  const uint32_t minimum_ms_;
  const uint32_t maximum_ms_;
  bool recording_ = false;
  uint32_t started_at_ms_ = 0;
};

}  // namespace recording_gesture

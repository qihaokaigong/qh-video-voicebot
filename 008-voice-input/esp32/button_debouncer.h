#pragma once

#include <cstdint>

class ButtonDebouncer {
 public:
  ButtonDebouncer(bool pressed_level, uint32_t debounce_ms)
      : pressed_level_(pressed_level), debounce_ms_(debounce_ms) {}

  bool begin(bool raw_level, uint32_t now_ms) {
    initialized_ = true;
    stable_level_ = raw_level;
    candidate_level_ = raw_level;
    candidate_since_ms_ = now_ms;
    changed_to_pressed_ = false;
    changed_to_released_ = false;
    return false;
  }

  bool update(bool raw_level, uint32_t now_ms) {
    changed_to_pressed_ = false;
    changed_to_released_ = false;

    if (!initialized_) return begin(raw_level, now_ms);

    if (raw_level != candidate_level_) {
      candidate_level_ = raw_level;
      candidate_since_ms_ = now_ms;
      return false;
    }

    if (candidate_level_ == stable_level_ ||
        static_cast<uint32_t>(now_ms - candidate_since_ms_) < debounce_ms_) {
      return false;
    }

    stable_level_ = candidate_level_;
    changed_to_pressed_ = stable_level_ == pressed_level_;
    changed_to_released_ = !changed_to_pressed_;
    return true;
  }

  bool isPressed() const { return stable_level_ == pressed_level_; }
  bool changedToPressed() const { return changed_to_pressed_; }
  bool changedToReleased() const { return changed_to_released_; }

 private:
  const bool pressed_level_;
  const uint32_t debounce_ms_;
  bool initialized_ = false;
  bool stable_level_ = false;
  bool candidate_level_ = false;
  uint32_t candidate_since_ms_ = 0;
  bool changed_to_pressed_ = false;
  bool changed_to_released_ = false;
};

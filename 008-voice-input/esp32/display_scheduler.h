#pragma once

#include <cstdint>

struct DisplayDue {
  bool wave;
  bool timer;
};

class DisplayScheduler {
 public:
  DisplayScheduler(uint32_t wave_period_ms, uint32_t timer_period_ms)
      : wave_period_ms_(wave_period_ms), timer_period_ms_(timer_period_ms) {}

  void reset(uint32_t now_ms) {
    last_wave_ms_ = now_ms - wave_period_ms_;
    last_timer_ms_ = now_ms - timer_period_ms_;
  }

  DisplayDue poll(uint32_t now_ms) {
    DisplayDue due{false, false};
    if (static_cast<uint32_t>(now_ms - last_wave_ms_) >= wave_period_ms_) {
      due.wave = true;
      last_wave_ms_ = now_ms;
    }
    if (static_cast<uint32_t>(now_ms - last_timer_ms_) >= timer_period_ms_) {
      due.timer = true;
      last_timer_ms_ = now_ms;
    }
    return due;
  }

 private:
  const uint32_t wave_period_ms_;
  const uint32_t timer_period_ms_;
  uint32_t last_wave_ms_ = 0;
  uint32_t last_timer_ms_ = 0;
};

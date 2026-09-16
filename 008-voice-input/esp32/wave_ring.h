#pragma once

#include <cstddef>
#include <cstdint>

class WaveRing {
 public:
  struct Update {
    std::size_t index;
    uint8_t old_height;
    uint8_t new_height;
  };

  WaveRing(std::size_t bar_count, uint8_t minimum_height,
           uint8_t maximum_height)
      : bar_count_(bar_count > kCapacity ? kCapacity : bar_count),
        minimum_height_(minimum_height),
        maximum_height_(maximum_height) {
    if (bar_count_ == 0) bar_count_ = 1;
    reset();
  }

  Update advance(double level) {
    if (level < 0.0) level = 0.0;
    if (level > 1.0) level = 1.0;
    const uint8_t new_height = static_cast<uint8_t>(
        minimum_height_ + level * (maximum_height_ - minimum_height_));
    const Update result{next_index_, heights_[next_index_], new_height};
    heights_[next_index_] = new_height;
    next_index_ = (next_index_ + 1) % bar_count_;
    return result;
  }

  void reset() {
    next_index_ = 0;
    for (std::size_t index = 0; index < kCapacity; ++index) {
      heights_[index] = minimum_height_;
    }
  }

 private:
  static constexpr std::size_t kCapacity = 32;
  std::size_t bar_count_;
  const uint8_t minimum_height_;
  const uint8_t maximum_height_;
  uint8_t heights_[kCapacity] = {};
  std::size_t next_index_ = 0;
};

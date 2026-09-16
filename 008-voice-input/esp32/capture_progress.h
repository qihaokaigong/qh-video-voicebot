#pragma once

#include <cstddef>

class CaptureProgress {
 public:
  explicit CaptureProgress(std::size_t maximum_samples)
      : maximum_samples_(maximum_samples) {}

  std::size_t reserve(std::size_t requested_samples) {
    const std::size_t remaining = maximum_samples_ - sample_count_;
    const std::size_t accepted =
        requested_samples < remaining ? requested_samples : remaining;
    sample_count_ += accepted;
    return accepted;
  }

  void reset() { sample_count_ = 0; }
  std::size_t sampleCount() const { return sample_count_; }
  bool full() const { return sample_count_ >= maximum_samples_; }

 private:
  const std::size_t maximum_samples_;
  volatile std::size_t sample_count_ = 0;
};

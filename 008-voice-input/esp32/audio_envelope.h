#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace audio_envelope {

inline double centeredRms(const int32_t *samples, std::size_t sample_count) {
  if (samples == nullptr || sample_count == 0) return 0.0;

  long double sum = 0.0;
  for (std::size_t index = 0; index < sample_count; ++index) {
    sum += samples[index];
  }
  const long double mean = sum / sample_count;

  long double square_sum = 0.0;
  for (std::size_t index = 0; index < sample_count; ++index) {
    const long double centered = samples[index] - mean;
    square_sum += centered * centered;
  }
  return static_cast<double>(std::sqrt(square_sum / sample_count));
}

inline double normalize(double value, double noise_floor, double ceiling) {
  if (ceiling <= noise_floor || value <= noise_floor) return 0.0;
  if (value >= ceiling) return 1.0;
  return (value - noise_floor) / (ceiling - noise_floor);
}

class Smoother {
 public:
  Smoother(double attack, double release)
      : attack_(attack), release_(release) {}

  double update(double input) {
    if (input < 0.0) input = 0.0;
    if (input > 1.0) input = 1.0;
    const double coefficient = input > value_ ? attack_ : release_;
    value_ += coefficient * (input - value_);
    return value_;
  }

  void reset() { value_ = 0.0; }

 private:
  const double attack_;
  const double release_;
  double value_ = 0.0;
};

}  // namespace audio_envelope

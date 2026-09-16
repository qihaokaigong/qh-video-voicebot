#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace wav_builder {

constexpr std::size_t kHeaderBytes = 44;

inline void writeLe16(uint8_t *target, uint16_t value) {
  target[0] = static_cast<uint8_t>(value & 0xffU);
  target[1] = static_cast<uint8_t>((value >> 8U) & 0xffU);
}

inline void writeLe32(uint8_t *target, uint32_t value) {
  target[0] = static_cast<uint8_t>(value & 0xffU);
  target[1] = static_cast<uint8_t>((value >> 8U) & 0xffU);
  target[2] = static_cast<uint8_t>((value >> 16U) & 0xffU);
  target[3] = static_cast<uint8_t>((value >> 24U) & 0xffU);
}

inline std::size_t requiredBytes(std::size_t sample_count) {
  return kHeaderBytes + sample_count * sizeof(int16_t);
}

inline std::size_t buildMonoPcm16(const int32_t *source,
                                  std::size_t sample_count,
                                  uint32_t sample_rate, uint8_t *target,
                                  std::size_t capacity) {
  if (source == nullptr || target == nullptr || sample_count == 0 ||
      sample_rate == 0 || capacity < requiredBytes(sample_count)) {
    return 0;
  }
  const uint32_t data_bytes =
      static_cast<uint32_t>(sample_count * sizeof(int16_t));
  std::memcpy(target, "RIFF", 4);
  writeLe32(target + 4, 36U + data_bytes);
  std::memcpy(target + 8, "WAVEfmt ", 8);
  writeLe32(target + 16, 16);
  writeLe16(target + 20, 1);
  writeLe16(target + 22, 1);
  writeLe32(target + 24, sample_rate);
  writeLe32(target + 28, sample_rate * sizeof(int16_t));
  writeLe16(target + 32, sizeof(int16_t));
  writeLe16(target + 34, 16);
  std::memcpy(target + 36, "data", 4);
  writeLe32(target + 40, data_bytes);

  for (std::size_t index = 0; index < sample_count; ++index) {
    const int16_t pcm16 = static_cast<int16_t>(source[index] >> 16);
    writeLe16(target + kHeaderBytes + index * 2,
              static_cast<uint16_t>(pcm16));
  }
  return requiredBytes(sample_count);
}

}  // namespace wav_builder

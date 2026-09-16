#pragma once

#include <cstddef>
#include <cstdint>

namespace recording_upload_policy {

inline void formatUuid(const uint8_t source[16], char output[37]) {
  static constexpr char kHex[] = "0123456789abcdef";
  uint8_t bytes[16];
  for (std::size_t index = 0; index < 16; ++index) bytes[index] = source[index];
  bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0fU) | 0x40U);
  bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3fU) | 0x80U);

  std::size_t position = 0;
  for (std::size_t index = 0; index < 16; ++index) {
    if (index == 4 || index == 6 || index == 8 || index == 10) {
      output[position++] = '-';
    }
    output[position++] = kHex[bytes[index] >> 4U];
    output[position++] = kHex[bytes[index] & 0x0fU];
  }
  output[position] = '\0';
}

inline bool shouldRetry(int http_status, unsigned attempt,
                        unsigned maximum_attempts) {
  if (attempt >= maximum_attempts) return false;
  return http_status < 0 || http_status >= 500;
}

}  // namespace recording_upload_policy

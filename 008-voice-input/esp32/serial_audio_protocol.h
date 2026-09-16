#pragma once

#include <cstddef>

namespace serial_audio_protocol {

constexpr std::size_t encodedBufferSize(std::size_t input_bytes) {
  return 4 * ((input_bytes + 2) / 3) + 1;
}

}  // namespace serial_audio_protocol

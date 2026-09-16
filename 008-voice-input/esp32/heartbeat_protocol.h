#pragma once

#include <cstdint>

namespace heartbeat_protocol {

inline bool shouldSend(std::uint32_t now_ms, std::uint32_t last_sent_ms,
                       std::uint32_t interval_ms, bool state_changed) {
  return state_changed ||
         static_cast<std::uint32_t>(now_ms - last_sent_ms) >= interval_ms;
}

}  // namespace heartbeat_protocol

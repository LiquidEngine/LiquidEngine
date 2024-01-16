#pragma once

namespace quoll {

constexpr inline u64 createVersion(u8 major, u8 minor = 0, u8 patch = 0,
                                   u32 build = 0) {
  return (static_cast<u64>(major) << 56) | (static_cast<u64>(minor) << 48) |
         (static_cast<u64>(patch) << 40) | (static_cast<u64>(build));
}

} // namespace quoll

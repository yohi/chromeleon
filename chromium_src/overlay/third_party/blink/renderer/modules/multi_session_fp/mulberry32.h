// chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/mulberry32.h
#ifndef CHROMIUM_SRC_OVERLAY_BLINK_MODULES_MULTI_SESSION_FP_MULBERRY32_H_
#define CHROMIUM_SRC_OVERLAY_BLINK_MODULES_MULTI_SESSION_FP_MULBERRY32_H_

#include <cstdint>

namespace blink {

// Allocation-free, deterministic 32-bit PRNG.
// 同一 seed + 同一呼出回数 → 同一出力を保証する。
class Mulberry32 {
 public:
  explicit Mulberry32(uint32_t seed) : state_(seed) {}

  uint32_t Next() {
    uint32_t z = (state_ += 0x6D2B79F5);
    z = (z ^ (z >> 15)) * (z | 1);
    z ^= z + (z ^ (z >> 7)) * (z | 61);
    return z ^ (z >> 14);
  }

 private:
  uint32_t state_;
};

}  // namespace blink

#endif  // CHROMIUM_SRC_OVERLAY_BLINK_MODULES_MULTI_SESSION_FP_MULBERRY32_H_

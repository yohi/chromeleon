// chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.cc
#include "chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.h"

#include <algorithm>

#include "chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/mulberry32.h"
#include "third_party/blink/renderer/core/html/canvas/image_data.h"

namespace blink {

const char FingerprintNoiseSource::kSupplementName[] = "FingerprintNoiseSource";

// static
FingerprintNoiseSource &FingerprintNoiseSource::From(LocalDOMWindow &window) {
  auto *supplement =
      Supplement<LocalDOMWindow>::From<FingerprintNoiseSource>(window);
  if (!supplement) {
    supplement = MakeGarbageCollected<FingerprintNoiseSource>(window);
    Supplement<LocalDOMWindow>::ProvideTo(window, supplement);
  }
  return *supplement;
}

FingerprintNoiseSource::FingerprintNoiseSource(LocalDOMWindow &window)
    : Supplement<LocalDOMWindow>(window),
      receiver_(this, window.GetExecutionContext()) {}

void FingerprintNoiseSource::SetSeed(uint64_t seed) {
  seed_ = seed;
  seed_received_ = true;
}

void FingerprintNoiseSource::ApplyCanvasNoise(ImageData *data) const {
  if (!seed_received_)
    return;
  const uint32_t base_seed = static_cast<uint32_t>(seed_);
  auto bytes = data->data()->Data();
  for (size_t i = 0; i < data->data()->length(); i += 4) {
    Mulberry32 rng(base_seed ^ static_cast<uint32_t>(i));
    for (int c = 0; c < 3; ++c) { // R,G,B のみ (Alpha は触らない)
      const int delta = static_cast<int>(rng.Next() & 1) * 2 - 1;
      const int v = static_cast<int>(bytes[i + c]) + delta;
      bytes[i + c] = static_cast<uint8_t>(std::clamp(v, 0, 255));
    }
  }
}

void FingerprintNoiseSource::ApplyWebGLNoise(base::span<uint8_t> pixels,
                                             GLenum format, GLenum type) const {
  if (!seed_received_)
    return;
  const uint32_t base_seed = static_cast<uint32_t>(seed_);
  // RGBA 前提（format/type による分岐は後続拡張で対応）
  for (size_t i = 0; i < pixels.size(); i += 4) {
    Mulberry32 rng(base_seed ^ static_cast<uint32_t>(i));
    for (int c = 0; c < 3; ++c) {
      const int delta = static_cast<int>(rng.Next() & 1) * 2 - 1;
      const int v = static_cast<int>(pixels[i + c]) + delta;
      pixels[i + c] = static_cast<uint8_t>(std::clamp(v, 0, 255));
    }
  }
}

void FingerprintNoiseSource::Trace(Visitor *visitor) const {
  Supplement<LocalDOMWindow>::Trace(visitor);
  visitor->Trace(receiver_);
}

} // namespace blink

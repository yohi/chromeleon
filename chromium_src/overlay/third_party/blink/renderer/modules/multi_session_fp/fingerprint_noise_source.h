// chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.h
#ifndef CHROMIUM_SRC_OVERLAY_BLINK_MODULES_MULTI_SESSION_FP_FINGERPRINT_NOISE_SOURCE_H_
#define CHROMIUM_SRC_OVERLAY_BLINK_MODULES_MULTI_SESSION_FP_FINGERPRINT_NOISE_SOURCE_H_

#include <cstdint>

#include "chromium_src/overlay/public/mojom/multi_session/fingerprint.mojom-blink.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/opengl/gl.h"

namespace blink {

class ImageData;

class MODULES_EXPORT FingerprintNoiseSource final
    : public GarbageCollected<FingerprintNoiseSource>,
      public Supplement<LocalDOMWindow>,
      public mojom::blink::FingerprintSeedReceiver {
public:
  static const char kSupplementName[];
  static FingerprintNoiseSource &From(LocalDOMWindow &window);

  explicit FingerprintNoiseSource(LocalDOMWindow &window);

  // mojom::blink::FingerprintSeedReceiver:
  void SetSeed(uint64_t seed) override;

  void ApplyCanvasNoise(ImageData *data) const;
  void ApplyWebGLNoise(base::span<uint8_t> pixels, GLenum format,
                       GLenum type) const;
  static bool WebDriverEnabled() { return false; }

  bool has_seed() const { return seed_received_; }

  void Trace(Visitor *) const override;

private:
  uint64_t seed_ = 0;
  bool seed_received_ = false;
  HeapMojoAssociatedReceiver<mojom::blink::FingerprintSeedReceiver,
                             FingerprintNoiseSource>
      receiver_;
};

} // namespace blink

#endif // CHROMIUM_SRC_OVERLAY_BLINK_MODULES_MULTI_SESSION_FP_FINGERPRINT_NOISE_SOURCE_H_

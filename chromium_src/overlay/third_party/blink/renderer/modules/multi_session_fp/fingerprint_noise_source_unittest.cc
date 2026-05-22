// chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source_unittest.cc
#include "chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/html/canvas/image_data.h"
#include "third_party/blink/renderer/platform/testing/task_environment.h"

namespace blink {
namespace {

TEST(FingerprintNoiseSourceTest, WebDriverEnabled_AlwaysFalse) {
  EXPECT_FALSE(FingerprintNoiseSource::WebDriverEnabled());
}

TEST(FingerprintNoiseSourceTest, NoSeed_NoChange) {
  test::TaskEnvironment task_environment;
  V8TestingScope scope;

  auto& noise_source = FingerprintNoiseSource::From(scope.GetWindow());
  EXPECT_FALSE(noise_source.has_seed());

  // ImageDataを作成 (2x2)
  ImageData* image_data = ImageData::CreateForTest(2, 2);
  ASSERT_NE(image_data, nullptr);

  auto* data_array = image_data->data();
  ASSERT_NE(data_array, nullptr);

  // 初期値を128に設定
  for (size_t i = 0; i < data_array->length(); ++i) {
    data_array->Data()[i] = 128;
  }

  // シードがない状態（受信前）で適用を試みる
  noise_source.ApplyCanvasNoise(image_data);

  // 変更されていないことを確認
  for (size_t i = 0; i < data_array->length(); ++i) {
    EXPECT_EQ(data_array->Data()[i], 128);
  }

  // WebGLノイズでも検証
  uint8_t pixels[8] = {128, 128, 128, 128, 128, 128, 128, 128};
  base::span<uint8_t> pixel_span(pixels, 8);
  noise_source.ApplyWebGLNoise(pixel_span, 0, 0);

  for (size_t i = 0; i < pixel_span.size(); ++i) {
    EXPECT_EQ(pixel_span[i], 128);
  }
}

TEST(FingerprintNoiseSourceTest, SameSeed_SameOutput) {
  test::TaskEnvironment task_environment;
  V8TestingScope scope;

  // 1つ目のウィンドウでシード12345を設定してCanvasにノイズを適用
  auto& noise_source1 = FingerprintNoiseSource::From(scope.GetWindow());
  noise_source1.SetSeed(12345ULL);

  ImageData* image_data1 = ImageData::CreateForTest(2, 2);
  auto* data_array1 = image_data1->data();
  for (size_t i = 0; i < data_array1->length(); ++i) {
    data_array1->Data()[i] = 128;
  }
  noise_source1.ApplyCanvasNoise(image_data1);

  // WebGL用
  uint8_t pixels1[8] = {128, 128, 128, 128, 128, 128, 128, 128};
  base::span<uint8_t> pixel_span1(pixels1, 8);
  noise_source1.ApplyWebGLNoise(pixel_span1, 0, 0);

  // 2つ目のウィンドウで、同じシードを設定
  V8TestingScope scope2;
  auto& noise_source2 = FingerprintNoiseSource::From(scope2.GetWindow());
  noise_source2.SetSeed(12345ULL);

  ImageData* image_data2 = ImageData::CreateForTest(2, 2);
  auto* data_array2 = image_data2->data();
  for (size_t i = 0; i < data_array2->length(); ++i) {
    data_array2->Data()[i] = 128;
  }
  noise_source2.ApplyCanvasNoise(image_data2);

  // WebGL用
  uint8_t pixels2[8] = {128, 128, 128, 128, 128, 128, 128, 128};
  base::span<uint8_t> pixel_span2(pixels2, 8);
  noise_source2.ApplyWebGLNoise(pixel_span2, 0, 0);

  // 同一のシードであれば、出力も同一になることを確認
  for (size_t i = 0; i < data_array1->length(); ++i) {
    EXPECT_EQ(data_array1->Data()[i], data_array2->Data()[i]);
  }
  for (size_t i = 0; i < pixel_span1.size(); ++i) {
    EXPECT_EQ(pixel_span1[i], pixel_span2[i]);
  }
}

TEST(FingerprintNoiseSourceTest, DifferentSeed_DifferentOutput) {
  test::TaskEnvironment task_environment;
  V8TestingScope scope1;
  V8TestingScope scope2;

  auto& noise_source1 = FingerprintNoiseSource::From(scope1.GetWindow());
  noise_source1.SetSeed(11111ULL);

  auto& noise_source2 = FingerprintNoiseSource::From(scope2.GetWindow());
  noise_source2.SetSeed(22222ULL);

  ImageData* image_data1 = ImageData::CreateForTest(2, 2);
  auto* data_array1 = image_data1->data();
  for (size_t i = 0; i < data_array1->length(); ++i) {
    data_array1->Data()[i] = 128;
  }
  noise_source1.ApplyCanvasNoise(image_data1);

  ImageData* image_data2 = ImageData::CreateForTest(2, 2);
  auto* data_array2 = image_data2->data();
  for (size_t i = 0; i < data_array2->length(); ++i) {
    data_array2->Data()[i] = 128;
  }
  noise_source2.ApplyCanvasNoise(image_data2);

  // WebGL用
  uint8_t pixels1[8] = {128, 128, 128, 128, 128, 128, 128, 128};
  base::span<uint8_t> pixel_span1(pixels1, 8);
  noise_source1.ApplyWebGLNoise(pixel_span1, 0, 0);

  uint8_t pixels2[8] = {128, 128, 128, 128, 128, 128, 128, 128};
  base::span<uint8_t> pixel_span2(pixels2, 8);
  noise_source2.ApplyWebGLNoise(pixel_span2, 0, 0);

  // 異なるシードであれば、出力結果に差異があることを確認
  bool canvas_different = false;
  for (size_t i = 0; i < data_array1->length(); ++i) {
    if (data_array1->Data()[i] != data_array2->Data()[i]) {
      canvas_different = true;
      break;
    }
  }
  EXPECT_TRUE(canvas_different);

  bool webgl_different = false;
  for (size_t i = 0; i < pixel_span1.size(); ++i) {
    if (pixel_span1[i] != pixel_span2[i]) {
      webgl_different = true;
      break;
    }
  }
  EXPECT_TRUE(webgl_different);
}

}  // namespace
}  // namespace blink

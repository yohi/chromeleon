// chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/mulberry32_unittest.cc
#include "chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp/mulberry32.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

TEST(Mulberry32Test, SameSeed_SameSequence) {
  Mulberry32 a(42);
  Mulberry32 b(42);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(a.Next(), b.Next()) << "Diverged at iteration " << i;
  }
}

TEST(Mulberry32Test, DifferentSeed_DifferentSequence) {
  Mulberry32 a(42);
  Mulberry32 b(43);
  int differences = 0;
  for (int i = 0; i < 100; ++i) {
    if (a.Next() != b.Next())
      ++differences;
  }
  // 異なる seed なら大部分の出力が異なるはず
  EXPECT_GT(differences, 90);
}

TEST(Mulberry32Test, ZeroSeed_ProducesNonZero) {
  Mulberry32 rng(0);
  bool has_nonzero = false;
  for (int i = 0; i < 10; ++i) {
    if (rng.Next() != 0) {
      has_nonzero = true;
      break;
    }
  }
  EXPECT_TRUE(has_nonzero);
}

}  // namespace
}  // namespace blink

#include <kimia_test.h>
#include <kimia/ProfilerGraphPanel.h>

KIMIA_TEST(ProfilerGraph_DrawEmptyDoesNotCrash) {
  kimia::ui::drawProfilerGraph({0, 0, 200, 40}, {}, 16.0f);
}

KIMIA_TEST(ProfilerGraph_DrawOneSample) {
  std::vector<float> v{8.0f};
  kimia::ui::drawProfilerGraph({0, 0, 200, 40}, v, 16.0f);
}

KIMIA_TEST(ProfilerGraph_DrawManySamples) {
  std::vector<float> v;
  for (int i = 0; i < 60; ++i) {
    v.push_back(8.0f + 4.0f * static_cast<float>(i % 5));
  }
  kimia::ui::drawProfilerGraph({0, 0, 240, 40}, v, 16.0f);
}

KIMIA_TEST(ProfilerGraph_DrawWithZeroMax) {
  // maxValue <= 0 → no draw (no crash).
  std::vector<float> v{1.0f, 2.0f, 3.0f};
  kimia::ui::drawProfilerGraph({0, 0, 200, 40}, v, 0.0f);
  kimia::ui::drawProfilerGraph({0, 0, 200, 40}, v, -1.0f);
}

KIMIA_TEST(ProfilerGraph_DrawWithOverMax) {
  std::vector<float> v{100.0f, 200.0f, 300.0f};
  kimia::ui::drawProfilerGraph({0, 0, 200, 40}, v, 16.0f);
}

KIMIA_TEST(ProfilerGraph_DrawAtPhonePortrait) {
  std::vector<float> v;
  for (int i = 0; i < 30; ++i) v.push_back(static_cast<float>(i) * 0.5f);
  kimia::ui::drawProfilerGraph({0, 0, 240, 40}, v, 16.0f);
}

KIMIA_TEST(ProfilerGraph_DrawAtTabletLandscape) {
  std::vector<float> v;
  for (int i = 0; i < 90; ++i) v.push_back(static_cast<float>(i % 10));
  kimia::ui::drawProfilerGraph({0, 0, 480, 40}, v, 10.0f);
}

KIMIA_TEST(ProfilerGraph_DrawWithTinyRect) {
  std::vector<float> v{5.0f, 10.0f, 15.0f};
  kimia::ui::drawProfilerGraph({0, 0, 10, 10}, v, 16.0f);
}

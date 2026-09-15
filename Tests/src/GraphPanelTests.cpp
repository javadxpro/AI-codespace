#include <kimia_test.h>
#include <kimia/GraphPanel.h>
#include <cmath>

KIMIA_TEST(Graph_DrawEmptyDoesNotCrash) {
  kimia::ui::drawGraphPanel({0, 0, 320, 200}, {}, 0.0f, 1.0f);
}

KIMIA_TEST(Graph_DrawOneSeries) {
  std::vector<kimia::ui::GraphSeries> v(1);
  v[0].values = {0.0f, 0.5f, 1.0f, 0.7f, 0.3f, 0.9f, 1.0f};
  v[0].r = 80; v[0].g = 200; v[0].b = 240;
  kimia::ui::drawGraphPanel({0, 0, 320, 200}, v, 0.0f, 1.0f);
}

KIMIA_TEST(Graph_DrawManySeries) {
  std::vector<kimia::ui::GraphSeries> v;
  for (int s = 0; s < 4; ++s) {
    kimia::ui::GraphSeries gs;
    gs.r = static_cast<kimia::u8>((s * 60) & 255);
    gs.g = static_cast<kimia::u8>((s * 80 + 30) & 255);
    gs.b = static_cast<kimia::u8>((s * 100 + 60) & 255);
    for (int i = 0; i < 60; ++i) {
      const kimia::f32 x = static_cast<kimia::f32>(i) / 60.0f;
      const kimia::f32 val = 0.5f + 0.4f * std::sin(x * 6.28f * (1 + s));
      gs.values.push_back(val);
    }
    v.push_back(gs);
  }
  kimia::ui::drawGraphPanel({0, 0, 360, 240}, v, 0.0f, 1.0f);
}

KIMIA_TEST(Graph_DrawWithNegativeRange) {
  std::vector<kimia::ui::GraphSeries> v(1);
  for (int i = 0; i < 20; ++i) {
    v[0].values.push_back(static_cast<kimia::f32>(i) - 10.0f);
  }
  v[0].r = 200; v[0].g = 80; v[0].b = 80;
  kimia::ui::drawGraphPanel({0, 0, 280, 200}, v, -10.0f, 10.0f);
}

KIMIA_TEST(Graph_DrawWithZeroRange) {
  std::vector<kimia::ui::GraphSeries> v(1);
  v[0].values = {0.5f, 0.5f, 0.5f, 0.5f};
  v[0].r = 80; v[0].g = 200; v[0].b = 80;
  kimia::ui::drawGraphPanel({0, 0, 280, 200}, v, 0.5f, 0.5f);
}

KIMIA_TEST(Graph_DrawWithoutFill) {
  std::vector<kimia::ui::GraphSeries> v(1);
  v[0].values = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
  v[0].fill = false;
  kimia::ui::drawGraphPanel({0, 0, 280, 200}, v, 0.0f, 1.0f);
}

KIMIA_TEST(Graph_DrawAtPhonePortrait) {
  std::vector<kimia::ui::GraphSeries> v(1);
  for (int i = 0; i < 50; ++i) {
    v[0].values.push_back(static_cast<kimia::f32>(i % 10) / 10.0f);
  }
  kimia::ui::drawGraphPanel({0, 0, 240, 320}, v, 0.0f, 1.0f);
}

KIMIA_TEST(Graph_DrawAtTabletLandscape) {
  std::vector<kimia::ui::GraphSeries> v;
  for (int s = 0; s < 3; ++s) {
    kimia::ui::GraphSeries gs;
    gs.r = static_cast<kimia::u8>(80 + s * 60);
    for (int i = 0; i < 100; ++i) {
      gs.values.push_back(static_cast<kimia::f32>((i + s) % 20) / 20.0f);
    }
    v.push_back(gs);
  }
  kimia::ui::drawGraphPanel({0, 0, 480, 320}, v, 0.0f, 1.0f);
}

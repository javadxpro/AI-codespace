#include <kimia_test.h>
#include <kimia/MiniMapPanel.h>

KIMIA_TEST(MiniMap_DrawEmptyDoesNotCrash) {
  kimia::ui::drawMiniMapPanel({0, 0, 160, 120},
                              {},
                              -10.0f, -10.0f, 10.0f, 10.0f,
                              0.0f, 0.0f, 5.0f, 5.0f);
}

KIMIA_TEST(MiniMap_DrawOneItem) {
  std::vector<kimia::ui::MiniMapItem> v(1);
  v[0] = {0.0f, 0.0f, 2.0f, 2.0f, 220, 80, 80};
  kimia::ui::drawMiniMapPanel({0, 0, 160, 120}, v,
                              -10.0f, -10.0f, 10.0f, 10.0f,
                              0.0f, 0.0f, 5.0f, 5.0f);
}

KIMIA_TEST(MiniMap_DrawManyItems) {
  std::vector<kimia::ui::MiniMapItem> v;
  v.push_back({-8.0f, -8.0f, 4.0f, 4.0f, 200, 80, 80});
  v.push_back({-2.0f, -8.0f, 4.0f, 4.0f, 80, 200, 80});
  v.push_back({ 4.0f, -8.0f, 4.0f, 4.0f, 80, 80, 200});
  v.push_back({-8.0f,  4.0f, 4.0f, 4.0f, 200, 200, 80});
  v.push_back({ 4.0f,  4.0f, 4.0f, 4.0f, 200, 80, 200});
  v.push_back({-1.0f, -1.0f, 2.0f, 2.0f, 120, 200, 80});
  kimia::ui::drawMiniMapPanel({0, 0, 200, 160}, v,
                              -10.0f, -10.0f, 10.0f, 10.0f,
                              -2.0f, -2.0f, 4.0f, 4.0f);
}

KIMIA_TEST(MiniMap_DrawWithZeroWorldSize) {
  // Degenerate world: min == max should not divide by zero.
  std::vector<kimia::ui::MiniMapItem> v(1);
  v[0] = {0.0f, 0.0f, 1.0f, 1.0f, 200, 200, 200};
  kimia::ui::drawMiniMapPanel({0, 0, 160, 120}, v,
                              0.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 0.0f, 0.0f);
}

KIMIA_TEST(MiniMap_DrawWithNegativeWorld) {
  std::vector<kimia::ui::MiniMapItem> v;
  v.push_back({-50.0f, -50.0f, 10.0f, 10.0f, 200, 100, 100});
  v.push_back({ 40.0f,  40.0f, 10.0f, 10.0f, 100, 200, 100});
  kimia::ui::drawMiniMapPanel({0, 0, 160, 120}, v,
                              -100.0f, -100.0f, 100.0f, 100.0f,
                              0.0f, 0.0f, 20.0f, 20.0f);
}

KIMIA_TEST(MiniMap_DrawAtPhonePortrait) {
  std::vector<kimia::ui::MiniMapItem> v;
  v.push_back({-1.0f, -1.0f, 1.0f, 1.0f, 200, 80, 80});
  v.push_back({ 0.0f,  0.0f, 1.0f, 1.0f, 80, 200, 80});
  kimia::ui::drawMiniMapPanel({0, 0, 160, 240}, v,
                              -5.0f, -5.0f, 5.0f, 5.0f,
                              -1.0f, -1.0f, 2.0f, 2.0f);
}

KIMIA_TEST(MiniMap_DrawAtTabletLandscape) {
  std::vector<kimia::ui::MiniMapItem> v;
  for (int i = 0; i < 12; ++i) {
    kimia::ui::MiniMapItem m;
    m.worldX = static_cast<kimia::f32>(i);
    m.worldY = static_cast<kimia::f32>(i);
    m.worldW = 2.0f;
    m.worldH = 2.0f;
    m.r = (i * 20) & 255;
    m.g = (i * 40) & 255;
    m.b = (i * 60) & 255;
    v.push_back(m);
  }
  kimia::ui::drawMiniMapPanel({0, 0, 240, 180}, v,
                              -5.0f, -5.0f, 20.0f, 20.0f,
                              0.0f, 0.0f, 8.0f, 6.0f);
}

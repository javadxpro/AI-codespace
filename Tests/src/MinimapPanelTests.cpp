#include <kimia_test.h>
#include <kimia/MinimapPanel.h>

KIMIA_TEST(Minimap_DrawEmptyDoesNotCrash) {
  kimia::ui::drawMinimapPanel({0, 0, 120, 120}, {0, 0}, 20.0f, {});
}

KIMIA_TEST(Minimap_DrawOneDot) {
  std::vector<kimia::ui::MinimapDot> v(1);
  v[0].worldPos = {0.0, 0.0, 0.0};
  v[0].color = {1.0, 0.5, 0.2, 1.0};
  kimia::ui::drawMinimapPanel({0, 0, 120, 120}, {0, 0}, 20.0f, v);
}

KIMIA_TEST(Minimap_DrawManyDots) {
  std::vector<kimia::ui::MinimapDot> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::MinimapDot d;
    d.worldPos = {static_cast<double>(i % 6) * 4.0,
                  static_cast<double>(i / 6) * 4.0, 0.0};
    d.color = {static_cast<float>(i) / 30.0f, 0.5f, 0.5f, 1.0f};
    v.push_back(d);
  }
  kimia::ui::drawMinimapPanel({0, 0, 200, 200}, {0, 0}, 40.0f, v);
}

KIMIA_TEST(Minimap_DrawWithZeroViewSize) {
  std::vector<kimia::ui::MinimapDot> v(1);
  v[0].worldPos = {1, 2, 0};
  // viewSize = 0 → no draw of the inner rect (must not crash).
  kimia::ui::drawMinimapPanel({0, 0, 100, 100}, {0, 0}, 0.0f, v);
}

KIMIA_TEST(Minimap_DrawWithNegativeViewSize) {
  std::vector<kimia::ui::MinimapDot> v(1);
  v[0].worldPos = {1, 2, 0};
  kimia::ui::drawMinimapPanel({0, 0, 100, 100}, {0, 0}, -10.0f, v);
}

KIMIA_TEST(Minimap_DrawWithPannedView) {
  std::vector<kimia::ui::MinimapDot> v(1);
  v[0].worldPos = {5.0, 5.0, 0.0};
  kimia::ui::drawMinimapPanel({0, 0, 120, 120}, {3.0, 3.0}, 20.0f, v);
}

KIMIA_TEST(Minimap_DrawAtPhonePortrait) {
  std::vector<kimia::ui::MinimapDot> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::MinimapDot d;
    d.worldPos = {static_cast<double>(i), static_cast<double>(i), 0.0};
    v.push_back(d);
  }
  kimia::ui::drawMinimapPanel({0, 0, 100, 140}, {0, 0}, 20.0f, v);
}

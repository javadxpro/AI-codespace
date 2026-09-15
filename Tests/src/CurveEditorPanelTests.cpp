#include <kimia_test.h>
#include <kimia/CurveEditorPanel.h>

KIMIA_TEST(CurveEditor_DrawOnePointDoesNotCrash) {
  std::vector<kimia::ui::CurvePoint> v(1);
  v[0] = {0.0f, 0.0f, 0};
  kimia::ui::drawCurveEditorPanel({0, 0, 280, 200}, v, 0.0f, 1.0f, 0.0f, 1.0f, -1);
}

KIMIA_TEST(CurveEditor_DrawTwoPoints) {
  std::vector<kimia::ui::CurvePoint> v(2);
  v[0] = {0.0f, 0.0f, 0};
  v[1] = {1.0f, 1.0f, 0};
  kimia::ui::drawCurveEditorPanel({0, 0, 280, 200}, v, 0.0f, 1.0f, 0.0f, 1.0f, 0);
}

KIMIA_TEST(CurveEditor_DrawManyPoints) {
  std::vector<kimia::ui::CurvePoint> v;
  for (int i = 0; i < 12; ++i) {
    kimia::ui::CurvePoint p;
    p.x = static_cast<kimia::f32>(i) / 12.0f;
    p.y = (i % 2 == 0) ? 0.2f : 0.8f;
    p.selected = (i == 5) ? 3 : 0;
    v.push_back(p);
  }
  kimia::ui::drawCurveEditorPanel({0, 0, 320, 240}, v, 0.0f, 1.0f, 0.0f, 1.0f, 5);
}

KIMIA_TEST(CurveEditor_DrawCustomRange) {
  std::vector<kimia::ui::CurvePoint> v;
  v.push_back({-5.0f, -2.0f, 0});
  v.push_back({ 0.0f,  5.0f, 0});
  v.push_back({ 5.0f, -2.0f, 0});
  kimia::ui::drawCurveEditorPanel({0, 0, 320, 200}, v, -10.0f, 10.0f, -5.0f, 5.0f, 1);
}

KIMIA_TEST(CurveEditor_DrawEmptyDoesNotCrash) {
  std::vector<kimia::ui::CurvePoint> v;
  kimia::ui::drawCurveEditorPanel({0, 0, 280, 200}, v, 0.0f, 1.0f, 0.0f, 1.0f, -1);
}

KIMIA_TEST(CurveEditor_DrawAtPhonePortrait) {
  std::vector<kimia::ui::CurvePoint> v;
  for (int i = 0; i < 6; ++i) {
    v.push_back({static_cast<kimia::f32>(i) / 6.0f,
                 static_cast<kimia::f32>((i * 13) % 100) / 100.0f, 0});
  }
  kimia::ui::drawCurveEditorPanel({0, 0, 240, 320}, v, 0.0f, 1.0f, 0.0f, 1.0f, 2);
}

KIMIA_TEST(CurveEditor_DrawAtTabletLandscape) {
  std::vector<kimia::ui::CurvePoint> v;
  for (int i = 0; i < 20; ++i) {
    v.push_back({static_cast<kimia::f32>(i) / 20.0f,
                 static_cast<kimia::f32>((i * 7) % 50) / 50.0f, 0});
  }
  kimia::ui::drawCurveEditorPanel({0, 0, 480, 320}, v, 0.0f, 1.0f, 0.0f, 1.5f, 10);
}

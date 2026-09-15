#include <kimia_test.h>
#include <kimia/CurveEditorPanel.h>

KIMIA_TEST(CurveEditor_DrawEmptyDoesNotCrash) {
  kimia::ui::drawCurveEditorPanel({0, 0, 240, 160}, {});
}

KIMIA_TEST(CurveEditor_DrawSingleKey) {
  // A single key — no curve lines, just a dot. Must not crash.
  std::vector<kimia::ui::CurveKey> v(1);
  v[0].t = 0.5f;
  v[0].v = 1.0f;
  kimia::ui::drawCurveEditorPanel({0, 0, 240, 160}, v);
}

KIMIA_TEST(CurveEditor_DrawLinearEase) {
  std::vector<kimia::ui::CurveKey> v;
  v.push_back({0.0f, 0.0f, kimia::ui::CurveInterp::Linear});
  v.push_back({1.0f, 1.0f, kimia::ui::CurveInterp::Linear});
  kimia::ui::drawCurveEditorPanel({0, 0, 240, 160}, v);
}

KIMIA_TEST(CurveEditor_DrawEaseInOut) {
  std::vector<kimia::ui::CurveKey> v;
  v.push_back({0.0f,  0.0f,  kimia::ui::CurveInterp::Smooth});
  v.push_back({0.5f,  0.8f,  kimia::ui::CurveInterp::Smooth});
  v.push_back({1.0f,  1.0f,  kimia::ui::CurveInterp::Smooth});
  kimia::ui::drawCurveEditorPanel({0, 0, 240, 160}, v);
}

KIMIA_TEST(CurveEditor_DrawStep) {
  std::vector<kimia::ui::CurveKey> v;
  v.push_back({0.0f, 0.0f, kimia::ui::CurveInterp::Step});
  v.push_back({0.5f, 1.0f, kimia::ui::CurveInterp::Step});
  v.push_back({1.0f, 0.0f, kimia::ui::CurveInterp::Step});
  kimia::ui::drawCurveEditorPanel({0, 0, 240, 160}, v);
}

KIMIA_TEST(CurveEditor_DrawManyKeys) {
  std::vector<kimia::ui::CurveKey> v;
  for (int i = 0; i < 20; ++i) {
    v.push_back({static_cast<float>(i) / 19.0f,
                 static_cast<float>(i % 3) * 0.5f,
                 kimia::ui::CurveInterp::Linear});
  }
  kimia::ui::drawCurveEditorPanel({0, 0, 280, 200}, v);
}

KIMIA_TEST(CurveEditor_DrawAtPhonePortrait) {
  std::vector<kimia::ui::CurveKey> v;
  v.push_back({0.0f, 0.0f, kimia::ui::CurveInterp::Linear});
  v.push_back({1.0f, 1.0f, kimia::ui::CurveInterp::Linear});
  kimia::ui::drawCurveEditorPanel({0, 0, 240, 200}, v);
}

KIMIA_TEST(CurveEditor_DrawAtTabletLandscape) {
  std::vector<kimia::ui::CurveKey> v;
  v.push_back({0.0f, 0.0f,  kimia::ui::CurveInterp::Linear});
  v.push_back({0.5f, 0.5f,  kimia::ui::CurveInterp::Smooth});
  v.push_back({1.0f, 1.0f,  kimia::ui::CurveInterp::Linear});
  kimia::ui::drawCurveEditorPanel({0, 0, 480, 240}, v);
}

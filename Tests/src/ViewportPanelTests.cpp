#include <kimia_test.h>
#include <kimia/ViewportPanel.h>

KIMIA_TEST(Viewport_DrawDefaultDoesNotCrash) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::GizmoMode::None,
                               kimia::ui::ViewportShading::Lit,
                               false, "Main", 1.0f, true, false, 1.0f);
}

KIMIA_TEST(Viewport_DrawWithMoveGizmo) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::GizmoMode::Move,
                               kimia::ui::ViewportShading::Lit,
                               false, "Main", 1.0f, true, true, 1.0f);
}

KIMIA_TEST(Viewport_DrawWithRotateGizmo) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::GizmoMode::Rotate,
                               kimia::ui::ViewportShading::Wireframe,
                               true, "Main", 1.0f, true, false, 1.0f);
}

KIMIA_TEST(Viewport_DrawWithScaleGizmo) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::GizmoMode::Scale,
                               kimia::ui::ViewportShading::Albedo,
                               false, "", 1.0f, true, true, 1.5f);
}

KIMIA_TEST(Viewport_DrawAllShadingModes) {
  const kimia::ui::ViewportShading modes[] = {
    kimia::ui::ViewportShading::Lit,
    kimia::ui::ViewportShading::Unlit,
    kimia::ui::ViewportShading::Wireframe,
    kimia::ui::ViewportShading::Albedo,
    kimia::ui::ViewportShading::Normals,
  };
  for (auto s : modes) {
    kimia::ui::drawViewportPanel({0, 0, 240, 200},
      kimia::ui::GizmoMode::None, s, false, "C", 1.0f, true, true, 1.0f);
  }
}

KIMIA_TEST(Viewport_DrawWithoutGrid) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::GizmoMode::Move,
                               kimia::ui::ViewportShading::Lit,
                               false, "Main", 1.0f, false, false, 1.0f);
}

KIMIA_TEST(Viewport_DrawWithZeroGrid) {
  // Should not divide by zero.
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::GizmoMode::None,
                               kimia::ui::ViewportShading::Lit,
                               false, "Main", 0.0f, true, false, 1.0f);
}

KIMIA_TEST(Viewport_DrawAtTinyZoom) {
  // step < 4 means we skip drawing the grid.
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::GizmoMode::Move,
                               kimia::ui::ViewportShading::Lit,
                               false, "Main", 1.0f, true, false, 0.001f);
}

KIMIA_TEST(Viewport_DrawAtHugeZoom) {
  // Large step should still draw some lines.
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::GizmoMode::None,
                               kimia::ui::ViewportShading::Lit,
                               false, "Main", 1.0f, true, true, 50.0f);
}

KIMIA_TEST(Viewport_DrawAtPhonePortrait) {
  kimia::ui::drawViewportPanel({0, 0, 240, 320},
                               kimia::ui::GizmoMode::Move,
                               kimia::ui::ViewportShading::Unlit,
                               true, "Cam", 1.0f, true, true, 1.0f);
}

KIMIA_TEST(Viewport_DrawAtTabletLandscape) {
  kimia::ui::drawViewportPanel({0, 0, 480, 320},
                               kimia::ui::GizmoMode::Rotate,
                               kimia::ui::ViewportShading::Normals,
                               false, "MainCamera", 2.0f, true, true, 1.0f);
}

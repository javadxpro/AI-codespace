// CameraPanel tests — see Engine/EditorUI/include/kimia/CameraPanel.h.

#include <kimia_test.h>
#include <kimia/CameraPanel.h>

KIMIA_TEST(CameraPanel_DrawDefaultDoesNotCrash) {
  kimia::ui::CameraProps p;
  kimia::ui::drawCameraPanel({0, 0, 240, 280}, p);
}

KIMIA_TEST(CameraPanel_DrawPerspectiveDoesNotCrash) {
  kimia::ui::CameraProps p;
  p.projection = kimia::ui::CameraProjection::Perspective;
  p.fovYDeg = 75.0f;
  p.nearPlane = 0.01f;
  p.farPlane = 500.0f;
  p.eye = {5.0, 3.0, 5.0};
  p.target = {0.0, 1.0, 0.0};
  p.up = {0.0, 1.0, 0.0};
  kimia::ui::drawCameraPanel({0, 0, 280, 320}, p);
}

KIMIA_TEST(CameraPanel_DrawOrthographicDoesNotCrash) {
  kimia::ui::CameraProps p;
  p.projection = kimia::ui::CameraProjection::Orthographic;
  p.orthoSize = 8.0f;
  p.eye = {0.0, 0.0, 10.0};
  p.target = {0.0, 0.0, 0.0};
  p.up = {0.0, 1.0, 0.0};
  kimia::ui::drawCameraPanel({0, 0, 280, 320}, p);
}

KIMIA_TEST(CameraPanel_DrawWithExtremeValues) {
  // Very large / very small / zero / negative — exercise the
  // formatting paths.
  kimia::ui::CameraProps p;
  p.fovYDeg = 0.0f;
  p.nearPlane = 0.0f;
  p.farPlane = 100000.0f;
  p.eye = {-1000.0, -1000.0, -1000.0};
  p.target = {1000.0, 1000.0, 1000.0};
  p.up = {0.0, 0.0, 1.0};
  kimia::ui::drawCameraPanel({0, 0, 280, 320}, p);
}

KIMIA_TEST(CameraPanel_DrawAtPhonePortrait) {
  kimia::ui::CameraProps p;
  p.fovYDeg = 90.0f;
  kimia::ui::drawCameraPanel({0, 0, 240, 360}, p);
}

KIMIA_TEST(CameraPanel_DrawAtTabletLandscape) {
  kimia::ui::CameraProps p;
  p.projection = kimia::ui::CameraProjection::Orthographic;
  p.orthoSize = 20.0f;
  kimia::ui::drawCameraPanel({0, 0, 800, 400}, p);
}

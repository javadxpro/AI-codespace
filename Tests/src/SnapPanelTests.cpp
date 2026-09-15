#include <kimia_test.h>
#include <kimia/SnapPanel.h>

KIMIA_TEST(Snap_DrawDefaultDoesNotCrash) {
  kimia::ui::SnapProps p;
  kimia::ui::drawSnapPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(Snap_DrawWithLargeGrid) {
  kimia::ui::SnapProps p;
  p.snapEnabled = true;
  p.positionStep = 1.0f;
  p.rotationStepDeg = 45.0f;
  p.scaleStep = 0.5f;
  p.gridSize = 5.0f;
  p.snapToGrid = true;
  p.snapToSurface = true;
  p.snapToAngle = true;
  kimia::ui::drawSnapPanel({0, 0, 280, 280}, p);
}

KIMIA_TEST(Snap_DrawWithSmallGrid) {
  kimia::ui::SnapProps p;
  p.snapEnabled = false;
  p.gridSize = 0.05f;
  p.positionStep = 0.01f;
  p.rotationStepDeg = 1.0f;
  p.scaleStep = 0.01f;
  kimia::ui::drawSnapPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(Snap_DrawAtPhonePortrait) {
  kimia::ui::SnapProps p;
  kimia::ui::drawSnapPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(Snap_DrawAtTabletLandscape) {
  kimia::ui::SnapProps p;
  p.gridSize = 2.0f;
  kimia::ui::drawSnapPanel({0, 0, 320, 280}, p);
}

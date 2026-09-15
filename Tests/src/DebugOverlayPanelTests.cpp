#include <kimia_test.h>
#include <kimia/DebugOverlayPanel.h>

KIMIA_TEST(DebugOverlay_DrawDefaultDoesNotCrash) {
  kimia::ui::DebugReadout r;
  kimia::ui::drawDebugOverlayPanel({0, 0, 200, 80}, r);
}

KIMIA_TEST(DebugOverlay_DrawHighFps) {
  kimia::ui::DebugReadout r;
  r.fps = 120.0f;
  r.frameMs = 8.0f;
  r.memMb = 128.0f;
  r.drawCalls = 100;
  r.verts = 50000;
  r.scene = "Main";
  r.camera = "MainCam";
  kimia::ui::drawDebugOverlayPanel({0, 0, 240, 100}, r);
}

KIMIA_TEST(DebugOverlay_DrawLowFps) {
  // red color
  kimia::ui::DebugReadout r;
  r.fps = 10.0f;
  r.frameMs = 100.0f;
  r.memMb = 1024.0f;
  r.drawCalls = 5000;
  r.verts = 5000000;
  r.scene = "Heavy";
  r.camera = "Cam_1";
  kimia::ui::drawDebugOverlayPanel({0, 0, 240, 100}, r);
}

KIMIA_TEST(DebugOverlay_DrawWithoutNames) {
  kimia::ui::DebugReadout r;
  r.scene = "";
  r.camera = "";
  r.fps = 60.0f;
  kimia::ui::drawDebugOverlayPanel({0, 0, 200, 60}, r);
}

KIMIA_TEST(DebugOverlay_DrawAtPhonePortrait) {
  kimia::ui::DebugReadout r;
  r.fps = 60.0f;
  r.frameMs = 16.0f;
  r.scene = "A";
  r.camera = "C";
  kimia::ui::drawDebugOverlayPanel({0, 0, 240, 320}, r);
}

KIMIA_TEST(DebugOverlay_DrawAtTabletLandscape) {
  kimia::ui::DebugReadout r;
  r.fps = 90.0f;
  r.frameMs = 11.0f;
  r.memMb = 512.0f;
  r.drawCalls = 500;
  r.verts = 100000;
  r.scene = "Stadium";
  r.camera = "Spectator";
  kimia::ui::drawDebugOverlayPanel({0, 0, 480, 320}, r);
}

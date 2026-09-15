#include <kimia_test.h>
#include <kimia/StatusBarPanel.h>

KIMIA_TEST(StatusBar_DrawDefaultDoesNotCrash) {
  kimia::ui::StatusInfo s;
  s.sceneName = "Untitled";
  s.version = "v0.30.0";
  s.fps = 60.0f;
  s.triCount = 50000;
  s.branch = "main";
  s.buildConfig = "Debug";
  kimia::ui::drawStatusBarPanel({0, 0, 800, 24}, s);
}

KIMIA_TEST(StatusBar_DrawHighFps) {
  kimia::ui::StatusInfo s;
  s.sceneName = "Soccer";
  s.version = "v0.30.0";
  s.fps = 120.0f;
  s.triCount = 100000;
  s.branch = "main";
  s.buildConfig = "Release";
  kimia::ui::drawStatusBarPanel({0, 0, 800, 24}, s);
}

KIMIA_TEST(StatusBar_DrawLowFps) {
  kimia::ui::StatusInfo s;
  s.sceneName = "Heavy";
  s.version = "v0.30.0";
  s.fps = 10.0f;
  s.triCount = 5000000;
  s.branch = "feature/x";
  s.buildConfig = "Debug";
  kimia::ui::drawStatusBarPanel({0, 0, 800, 24}, s);
}

KIMIA_TEST(StatusBar_DrawEmptyStrings) {
  kimia::ui::StatusInfo s;
  s.sceneName = "";
  s.version = "";
  s.branch = "";
  s.buildConfig = "";
  kimia::ui::drawStatusBarPanel({0, 0, 400, 24}, s);
}

KIMIA_TEST(StatusBar_DrawAtPhonePortrait) {
  // Narrow bar.
  kimia::ui::StatusInfo s;
  s.sceneName = "S";
  s.version = "v0.30";
  s.fps = 60.0f;
  s.triCount = 100;
  s.branch = "m";
  s.buildConfig = "D";
  kimia::ui::drawStatusBarPanel({0, 0, 240, 24}, s);
}

KIMIA_TEST(StatusBar_DrawAtTabletLandscape) {
  kimia::ui::StatusInfo s;
  s.sceneName = "Stadium";
  s.version = "v0.30.0";
  s.fps = 90.0f;
  s.triCount = 1000000;
  s.branch = "release/0.30";
  s.buildConfig = "Release";
  kimia::ui::drawStatusBarPanel({0, 0, 1024, 24}, s);
}

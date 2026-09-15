#include <kimia_test.h>
#include <kimia/PerformancePanel.h>

KIMIA_TEST(Perf_DrawDefaultDoesNotCrash) {
  kimia::ui::PerfStats s;
  kimia::ui::drawPerformancePanel({0, 0, 240, 200}, s);
}

KIMIA_TEST(Perf_DrawHealthy) {
  kimia::ui::PerfStats s;
  s.fps = 60.0f;
  s.frameMs = 16.0f;
  s.cpuMs = 4.0f;
  s.gpuMs = 8.0f;
  s.memMb = 256.0f;
  s.drawCalls = 200;
  s.triangles = 100000;
  s.verts = 50000;
  kimia::ui::drawPerformancePanel({0, 0, 240, 240}, s);
}

KIMIA_TEST(Perf_DrawSlow) {
  kimia::ui::PerfStats s;
  s.fps = 20.0f;
  s.frameMs = 50.0f;
  s.cpuMs = 30.0f;
  s.gpuMs = 15.0f;
  s.memMb = 1024.0f;
  s.drawCalls = 5000;
  s.triangles = 1000000;
  s.verts = 500000;
  kimia::ui::drawPerformancePanel({0, 0, 280, 240}, s);
}

KIMIA_TEST(Perf_DrawVerySlow) {
  // < 30 fps flips to warning, < 15 to error.
  kimia::ui::PerfStats s;
  s.fps = 10.0f;
  s.frameMs = 100.0f;
  kimia::ui::drawPerformancePanel({0, 0, 240, 200}, s);
}

KIMIA_TEST(Perf_DrawAtPhonePortrait) {
  kimia::ui::PerfStats s;
  s.fps = 60.0f;
  kimia::ui::drawPerformancePanel({0, 0, 240, 320}, s);
}

KIMIA_TEST(Perf_DrawAtTabletLandscape) {
  kimia::ui::PerfStats s;
  s.fps = 120.0f;
  s.drawCalls = 100;
  kimia::ui::drawPerformancePanel({0, 0, 320, 280}, s);
}

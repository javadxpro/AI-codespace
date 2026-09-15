// StatsPanel tests — see Engine/EditorUI/include/kimia/StatsPanel.h.

#include <kimia_test.h>
#include <kimia/StatsPanel.h>

KIMIA_TEST(StatsPanel_DrawDefaultDoesNotCrash) {
  kimia::ui::Stats s;
  kimia::ui::drawStatsPanel({0, 0, 100, 80}, s);
}

KIMIA_TEST(StatsPanel_DrawHealthyDoesNotCrash) {
  kimia::ui::Stats s;
  s.fps = 60.0f;
  s.frameMs = 16.7f;
  s.entityCount = 100;
  s.vertexCount = 50000;
  s.drawCalls = 200;
  s.memoryMb = 256;
  kimia::ui::drawStatsPanel({0, 0, 100, 80}, s);
}

KIMIA_TEST(StatsPanel_DrawSlowDoesNotCrash) {
  // The FPS colour flips to warning (< 30) or error (< 15).
  kimia::ui::Stats s;
  s.fps = 12.0f;
  s.frameMs = 83.3f;
  s.entityCount = 1;
  s.vertexCount = 0;
  s.drawCalls = 1;
  s.memoryMb = 16;
  kimia::ui::drawStatsPanel({0, 0, 100, 80}, s);
}

KIMIA_TEST(StatsPanel_DrawHeavyDoesNotCrash) {
  // Million-vertex scene — exercise the formatting paths.
  kimia::ui::Stats s;
  s.fps = 25.0f;
  s.frameMs = 40.0f;
  s.entityCount = 10000;
  s.vertexCount = 1000000;
  s.drawCalls = 5000;
  s.memoryMb = 4096;
  kimia::ui::drawStatsPanel({0, 0, 120, 100}, s);
}

KIMIA_TEST(StatsPanel_DrawZeroDoesNotCrash) {
  kimia::ui::Stats s;
  s.fps = 0.0f;
  s.frameMs = 0.0f;
  s.entityCount = 0;
  s.vertexCount = 0;
  s.drawCalls = 0;
  s.memoryMb = 0;
  kimia::ui::drawStatsPanel({0, 0, 100, 80}, s);
}

KIMIA_TEST(StatsPanel_DrawAtCornerSizes) {
  // Standard corner-overlay sizes.
  kimia::ui::Stats s;
  s.fps = 60.0f;
  kimia::ui::drawStatsPanel({0, 0, 120, 90}, s);
  kimia::ui::drawStatsPanel({0, 0, 140, 90}, s);
  kimia::ui::drawStatsPanel({0, 0, 100, 100}, s);
}

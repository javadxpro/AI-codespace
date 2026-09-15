// StatsPanel implementation — see StatsPanel.h.
#include <kimia/StatsPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <cstdio>

namespace kimia::ui {

namespace {

void drawRow(const Rect& row, const char* label, const char* value,
             Color valueColor) {
  using namespace theme;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kTextMuted);
  drawText(value, row.x + 70.0f, row.y + 4.0f, 1, valueColor);
}

}  // namespace

void drawStatsPanel(const Rect& rect, const Stats& stats) {
  using namespace theme;
  // Semi-transparent dark background so the stats overlay stays
  // readable over any scene colour. The exact alpha is tuned to
  // keep the scene mostly visible.
  drawRect(rect, {0.0f, 0.0f, 0.0f, 0.6f}, 2.0f);

  char buf[32];
  constexpr f32 rowH = 12.0f;
  f32 y = rect.y + 4.0f;

  // FPS — colour-coded so the user can spot problems at a glance.
  Color fpsColor = kSuccess;
  if (stats.fps < 30.0f) fpsColor = kWarning;
  if (stats.fps < 15.0f) fpsColor = kError;
  std::snprintf(buf, sizeof(buf), "%.0f", static_cast<double>(stats.fps));
  drawRow({rect.x, y, rect.w, rowH}, "FPS", buf, fpsColor);
  y += rowH;

  // Frame time.
  std::snprintf(buf, sizeof(buf), "%.1f ms",
                static_cast<double>(stats.frameMs));
  drawRow({rect.x, y, rect.w, rowH}, "Frame", buf, kText);
  y += rowH;

  // Entity / Vertex / Draw call counts.
  std::snprintf(buf, sizeof(buf), "%zu", stats.entityCount);
  drawRow({rect.x, y, rect.w, rowH}, "Entities", buf, kText);
  y += rowH;

  std::snprintf(buf, sizeof(buf), "%zu", stats.vertexCount);
  drawRow({rect.x, y, rect.w, rowH}, "Vertices", buf, kText);
  y += rowH;

  std::snprintf(buf, sizeof(buf), "%zu", stats.drawCalls);
  drawRow({rect.x, y, rect.w, rowH}, "Draw calls", buf, kText);
  y += rowH;

  std::snprintf(buf, sizeof(buf), "%zu MB", stats.memoryMb);
  drawRow({rect.x, y, rect.w, rowH}, "Memory", buf, kText);
}

}  // namespace kimia::ui

#include <kimia/PerformancePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
Color fpsColor(f32 fps) {
  using namespace theme;
  if (fps >= 55.0f) return kSuccess;
  if (fps >= 30.0f) return kWarning;
  return kError;
}
void drawRow(const Rect& row, const char* label, const char* value,
             Color c = theme::kText) {
  using namespace theme;
  constexpr f32 labelW = 70.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kTextMuted);
  drawText(value, row.x + labelW, row.y + 4.0f, 1, c);
}
}

void drawPerformancePanel(const Rect& rect, const PerfStats& stats) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Performance", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 16.0f;
  f32 y = rect.y + 18.0f;
  char buf[32];

  std::snprintf(buf, sizeof(buf), "%.0f fps", static_cast<double>(stats.fps));
  drawRow({rect.x, y, rect.w, rowH}, "FPS", buf, fpsColor(stats.fps));
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.1f ms", static_cast<double>(stats.frameMs));
  drawRow({rect.x, y, rect.w, rowH}, "Frame", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.1f ms", static_cast<double>(stats.cpuMs));
  drawRow({rect.x, y, rect.w, rowH}, "CPU", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.1f ms", static_cast<double>(stats.gpuMs));
  drawRow({rect.x, y, rect.w, rowH}, "GPU", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.0f MB", static_cast<double>(stats.memMb));
  drawRow({rect.x, y, rect.w, rowH}, "Memory", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%llu",
                static_cast<unsigned long long>(stats.drawCalls));
  drawRow({rect.x, y, rect.w, rowH}, "Draw calls", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%llu",
                static_cast<unsigned long long>(stats.triangles));
  drawRow({rect.x, y, rect.w, rowH}, "Triangles", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%llu",
                static_cast<unsigned long long>(stats.verts));
  drawRow({rect.x, y, rect.w, rowH}, "Vertices", buf);
}

}

#include <kimia/CurveEditorPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void valToScreen(const Rect& plot, f32 vx, f32 vy,
                 f32 minX, f32 maxX, f32 minY, f32 maxY,
                 f32& sx, f32& sy) {
  const f32 rngX = maxX - minX;
  const f32 rngY = maxY - minY;
  if (rngX <= 0.0f || rngY <= 0.0f) { sx = plot.x; sy = plot.y; return; }
  sx = plot.x + (vx - minX) / rngX * plot.w;
  sy = plot.y + plot.h - (vy - minY) / rngY * plot.h;
}
}

void drawCurveEditorPanel(const Rect& rect,
                          std::vector<CurvePoint>& points,
                          f32 minX, f32 maxX,
                          f32 minY, f32 maxY,
                          i32 selectedIndex) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Curve", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  const Rect plot = {rect.x + 4.0f, rect.y + 18.0f,
                     rect.w - 8.0f, rect.h - 24.0f};
  drawRect(plot, kPanelAlt, 0.0f);

  if (points.size() < 2) return;

  // Connect points with line approximation.
  for (std::size_t i = 1; i < points.size(); ++i) {
    f32 x0, y0, x1, y1;
    valToScreen(plot, points[i - 1].x, points[i - 1].y,
                minX, maxX, minY, maxY, x0, y0);
    valToScreen(plot, points[i].x, points[i].y,
                minX, maxX, minY, maxY, x1, y1);
    const f32 lx = std::min(x0, x1);
    const f32 lw = std::abs(x1 - x0);
    const f32 ly = std::min(y0, y1);
    const f32 lh = std::max(1.0f, std::abs(y1 - y0));
    drawRect({lx, ly, lw, lh}, kAccent, 0.0f);
  }

  // Points.
  for (std::size_t i = 0; i < points.size(); ++i) {
    f32 sx, sy;
    valToScreen(plot, points[i].x, points[i].y,
                minX, maxX, minY, maxY, sx, sy);
    const bool sel = (static_cast<i32>(i) == selectedIndex);
    drawRect({sx - 3.0f, sy - 3.0f, 6.0f, 6.0f},
             sel ? kAccentHot : kText, 0.0f);
  }

  // Axis labels.
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(maxY));
  drawText(buf, rect.x + 4.0f, rect.y + 18.0f, 1, kTextMuted);
  std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(minY));
  drawText(buf, rect.x + 4.0f, rect.y + rect.h - 14.0f, 1, kTextMuted);
  std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(minX));
  drawText(buf, rect.x + 4.0f, rect.y + rect.h - 14.0f, 1, kTextMuted);
  std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(maxX));
  drawText(buf, rect.x + rect.w - 30.0f, rect.y + rect.h - 14.0f,
           1, kTextMuted);
}

}

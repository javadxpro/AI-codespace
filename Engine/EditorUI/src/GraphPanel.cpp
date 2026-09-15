#include <kimia/GraphPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void drawSeries(const Rect& plotRect,
                const GraphSeries& s,
                f32 minY, f32 maxY) {
  using namespace theme;
  if (s.values.size() < 2) return;
  const f32 rangeY = maxY - minY;
  if (rangeY <= 0.0f) return;

  const f32 dx = plotRect.w / static_cast<f32>(s.values.size() - 1);
  const Color col = {s.r / 255.0f, s.g / 255.0f, s.b / 255.0f, 1.0f};
  const Color colDim = {s.r / 255.0f, s.g / 255.0f, s.b / 255.0f, 0.25f};

  // Fill: vertical bars from baseline to point.
  if (s.fill) {
    for (std::size_t i = 0; i < s.values.size(); ++i) {
      const f32 v = s.values[i];
      const f32 ny = (v - minY) / rangeY;
      if (ny < 0.0f || ny > 1.0f) continue;
      const f32 px = plotRect.x + static_cast<f32>(i) * dx;
      drawRect({px, plotRect.y + plotRect.h * (1.0f - ny), 1.0f,
                plotRect.h * ny},
               colDim, 0.0f);
    }
  }

  // Line.
  for (std::size_t i = 1; i < s.values.size(); ++i) {
    const f32 v0 = s.values[i - 1];
    const f32 v1 = s.values[i];
    if (v0 < minY || v1 < minY || v0 > maxY || v1 > maxY) continue;
    const f32 ny0 = (v0 - minY) / rangeY;
    const f32 ny1 = (v1 - minY) / rangeY;
    const f32 x0 = plotRect.x + static_cast<f32>(i - 1) * dx;
    const f32 x1 = plotRect.x + static_cast<f32>(i) * dx;
    const f32 y0 = plotRect.y + plotRect.h * (1.0f - ny0);
    const f32 y1 = plotRect.y + plotRect.h * (1.0f - ny1);
    // line approximation: thin rect
    {
      const f32 lx = std::min(x0, x1);
      const f32 lw = std::abs(x1 - x0);
      const f32 ly = std::min(y0, y1) - 0.5f;
      const f32 lh = std::max(1.0f, std::abs(y1 - y0));
      drawRect({lx, ly, lw, lh}, col, 0.0f);
    }
  }
}
}

void drawGraphPanel(const Rect& rect,
                    const std::vector<GraphSeries>& series,
                    f32 minY,
                    f32 maxY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Title.
  drawText("Graph", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Mini-legend on the right.
  f32 legX = rect.x + rect.w - 100.0f;
  for (std::size_t i = 0; i < series.size(); ++i) {
    drawRect({legX, rect.y + 8.0f, 8.0f, 6.0f},
             {series[i].r / 255.0f, series[i].g / 255.0f,
              series[i].b / 255.0f, 1.0f}, 0.0f);
    drawText(("S" + std::to_string(i)).c_str(),
             legX + 12.0f, rect.y + 6.0f, 1, kTextMuted);
    legX += 28.0f;
  }

  // Plot area.
  const Rect plot = {rect.x + 4.0f, rect.y + 18.0f,
                     rect.w - 8.0f, rect.h - 24.0f};
  drawRect(plot, kPanelAlt, 0.0f);

  // Baseline.
  const f32 midY = plot.y + plot.h * 0.5f;
  drawRect({plot.x, midY, plot.w, 1.0f}, kTextMuted, 0.0f);

  for (std::size_t i = 0; i < series.size(); ++i) {
    drawSeries(plot, series[i], minY, maxY);
  }

  // Min/Max labels.
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(maxY));
  drawText(buf, rect.x + 4.0f, rect.y + 18.0f, 1, kTextMuted);
  std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(minY));
  drawText(buf, rect.x + 4.0f,
           rect.y + rect.h - 14.0f, 1, kTextMuted);
}

}

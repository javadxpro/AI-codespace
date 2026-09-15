// ProfilerPanel implementation — see ProfilerPanel.h.
#include <kimia/ProfilerPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <algorithm>
#include <cstdio>

namespace kimia::ui {

void drawProfilerPanel(const Rect& rect,
                       const std::vector<ProfilerRow>& rows) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText("Profiler", rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  constexpr f32 rowH = 16.0f;
  constexpr f32 labelW = 70.0f;
  constexpr f32 barMaxW = 100.0f;
  const f32 startY = rect.y + 26.0f;

  pushClip(rect);
  char buf[32];
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const f32 y = startY + static_cast<f32>(i) * rowH;
    if (y + rowH > rect.y + rect.h) break;

    drawText(rows[i].label.c_str(),
             rect.x + 4.0f, y + 4.0f, 1, kTextMuted);

    // The bar background.
    const f32 barX = rect.x + labelW;
    drawRect({barX, y + 6.0f, barMaxW, 4.0f}, kPanelAlt, 2.0f);

    // The filled portion — relative to the row's maxMs.
    const f32 ratio = rows[i].maxMs > 0.0f
        ? std::max(0.0f, std::min(1.0f, rows[i].lastMs / rows[i].maxMs))
        : 0.0f;
    drawRect({barX, y + 6.0f, barMaxW * ratio, 4.0f}, kAccent, 2.0f);

    // Numeric readout on the right.
    std::snprintf(buf, sizeof(buf), "%.2f ms",
                  static_cast<double>(rows[i].lastMs));
    drawText(buf,
             barX + barMaxW + 6.0f, y + 4.0f, 1,
             rows[i].lastMs > 16.0f ? kWarning : kText);
  }
  popClip();
}

}  // namespace kimia::ui

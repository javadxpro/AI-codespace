#include <kimia/ProgressBarPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>
#include <algorithm>

namespace kimia::ui {

void drawProgressBarPanel(const Rect& rect,
                          const std::string& label,
                          float progress,
                          bool indeterminate) {
  using namespace theme;
  drawRect(rect, kPanel, 4.0f);

  drawText(label.c_str(), rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  const f32 barX = rect.x + 4.0f;
  const f32 barY = rect.y + 18.0f;
  const f32 barW = rect.w - 8.0f;
  const f32 barH = 10.0f;
  drawRect({barX, barY, barW, barH}, kPanelAlt, 2.0f);

  char pct[16];
  if (indeterminate) {
    // A pulsing 60% fill that's purely visual — the user just
    // sees motion.
    drawRect({barX, barY, barW * 0.6f, barH}, kAccent, 2.0f);
    std::snprintf(pct, sizeof(pct), "...");
  } else {
    const float p = std::max(0.0f, std::min(1.0f, progress));
    drawRect({barX, barY, barW * p, barH},
             p >= 1.0f ? kSuccess : kAccent, 2.0f);
    std::snprintf(pct, sizeof(pct), "%d%%", (int)(p * 100.0f));
  }
  drawText(pct, rect.x + rect.w - 36.0f, rect.y + 4.0f, 1, kTextMuted);
}

}

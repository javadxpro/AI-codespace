#include <kimia/HistoryPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawHistoryPanel(const Rect& rect,
                      const std::vector<HistoryEntry>& entries,
                      i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("History", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 22.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    // Active row gets accent highlight.
    if (!entries[i].undone) {
      drawRect({rect.x, y, rect.w, rowH}, kAccentDim, 0.0f);
    }

    // Marker on the left.
    drawText(entries[i].undone ? "o" : "*",
             rect.x + 4.0f, y + 4.0f, 1,
             entries[i].undone ? kTextMuted : kAccentHot);
    drawText(entries[i].description.c_str(),
             rect.x + 18.0f, y + 4.0f, 1,
             entries[i].undone ? kTextMuted : kText);
  }
  popClip();
}

}

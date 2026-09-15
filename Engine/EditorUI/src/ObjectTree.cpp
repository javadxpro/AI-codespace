// ObjectTree implementation — see ObjectTree.h.
#include <kimia/ObjectTree.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawObjectTree(const Rect& rect,
                    const std::vector<ObjectTreeEntry>& entries,
                    i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Hierarchy", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  const f32 rowH = 16.0f;
  const f32 padX = 6.0f;
  const f32 swatchW = 8.0f;
  const f32 startY = rect.y + 16.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const f32 y = startY + static_cast<f32>(i) * rowH;
    if (y + rowH < rect.y + 16.0f) continue;
    if (y > rect.y + rect.h) break;

    // Highlight row if selected.
    if (entries[i].selected) {
      drawRect({rect.x, y, rect.w, rowH}, kAccentDim, 0.0f);
    }
    // Colour swatch on the left.
    drawRect({rect.x + padX, y + 4.0f, swatchW, rowH - 8.0f},
             entries[i].swatch, 1.0f);
    // Name on the right of the swatch.
    const Color textColor = entries[i].selected ? kAccentHot : kText;
    drawText(entries[i].name.c_str(),
             rect.x + padX + swatchW + 4.0f, y + 4.0f, 1, textColor);
  }
  popClip();
}

}  // namespace kimia::ui

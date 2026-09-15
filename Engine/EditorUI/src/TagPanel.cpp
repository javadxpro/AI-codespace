#include <kimia/TagPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

void drawTagPanel(const Rect& rect,
                  const std::vector<Tag>& tags,
                  const std::string& newTagInput) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Tags", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Flow-layout the tag chips across the panel width.
  const f32 padX = 6.0f;
  const f32 chipH = 18.0f;
  const f32 padY = 18.0f;
  f32 x = rect.x + padX;
  f32 y = rect.y + padY;
  for (std::size_t i = 0; i < tags.size(); ++i) {
    const f32 w = static_cast<f32>(tags[i].name.size()) * 6.0f + 16.0f;
    if (x + w > rect.x + rect.w - padX) {
      x = rect.x + padX;
      y += chipH + 4.0f;
    }
    if (y + chipH > rect.y + rect.h - 18.0f) break;
    drawRect({x, y, w, chipH}, tags[i].color, 8.0f);
    drawText(tags[i].name.c_str(),
             x + 8.0f, y + 4.0f, 1, kText);
    x += w + 4.0f;
  }

  // New-tag input at the bottom.
  const Rect inputBar{
      rect.x + padX, rect.y + rect.h - chipH - 4.0f,
      rect.w - padX * 2.0f - 24.0f, chipH};
  drawRect(inputBar, kPanelAlt, 2.0f);
  drawText(newTagInput.empty() ? "new tag…" : newTagInput.c_str(),
           inputBar.x + 4.0f, inputBar.y + 4.0f, 1,
           newTagInput.empty() ? kTextDim : kText);
  if (button("+", {inputBar.x + inputBar.w + 4.0f, inputBar.y,
                    20.0f, chipH})) {
  }
}

}

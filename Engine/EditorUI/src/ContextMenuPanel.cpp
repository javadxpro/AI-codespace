// ContextMenuPanel implementation — see ContextMenuPanel.h.
#include <kimia/ContextMenuPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <algorithm>

namespace kimia::ui {

void drawContextMenu(f32 x, f32 y, const ContextMenuState& state) {
  using namespace theme;
  if (!state.open) return;

  constexpr f32 itemH = 18.0f;
  constexpr f32 sepH = 6.0f;
  constexpr f32 padX = 8.0f;
  constexpr f32 padY = 4.0f;

  // Compute total height from item types.
  f32 totalH = padY * 2.0f;
  f32 maxW = 120.0f;  // minimum menu width
  for (const auto& item : state.items) {
    totalH += item.separator ? sepH : itemH;
    const f32 textW = static_cast<f32>(item.label.size()) * 6.0f + 16.0f;
    if (textW > maxW) maxW = textW;
  }

  const Rect menu{x, y, maxW, totalH};
  drawRect(menu, kPanel, 2.0f);

  f32 curY = y + padY;
  for (const auto& item : state.items) {
    if (item.separator) {
      // Horizontal divider.
      drawRect({x + 4.0f, curY + sepH * 0.5f - 1.0f,
                maxW - 8.0f, 1.0f}, kBorder, 0.0f);
      curY += sepH;
      continue;
    }
    const Color textColor = item.disabled ? kTextDim : kText;
    drawText(item.label.c_str(),
             x + padX, curY + 4.0f, 1, textColor);
    if (button(item.label.c_str(),
               {x, curY, maxW, itemH})) {
      // Phase 5+ will route the click through a UiCommand.
    }
    curY += itemH;
  }
  (void)std::max(0.0f, 0.0f);
}

}  // namespace kimia::ui

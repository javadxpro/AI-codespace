#include <kimia/MenuBarPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

void drawMenuBarPanel(const Rect& rect,
                      const std::vector<Menu>& menus,
                      int openMenuIndex) {
  using namespace theme;
  drawRect(rect, kTitlebar, 0.0f);

  constexpr f32 itemH = 18.0f;
  f32 x = rect.x + 4.0f;
  const f32 y = rect.y + (rect.h - itemH) * 0.5f;

  for (std::size_t i = 0; i < menus.size(); ++i) {
    const f32 w = static_cast<f32>(menus[i].title.size()) * 6.0f + 12.0f;
    const Rect item{x, y, w, itemH};
    const bool open = static_cast<int>(i) == openMenuIndex;
    drawRect(item, open ? kAccentDim : kTitlebar, 0.0f);
    drawText(menus[i].title.c_str(),
             item.x + 6.0f, item.y + 4.0f, 1,
             open ? kAccentHot : kText);
    x += w;

    // Drop-down body.
    if (open) {
      f32 dy = rect.y + rect.h;
      f32 maxW = 160.0f;
      for (const auto& mi : menus[i].items) {
        const f32 w2 = static_cast<f32>(mi.label.size()) * 6.0f +
                       (mi.shortcut.empty() ? 12.0f :
                        static_cast<float>(mi.shortcut.size()) * 6.0f + 24.0f);
        if (w2 > maxW) maxW = w2;
      }
      const Rect dd{item.x, dy, maxW,
                    itemH * static_cast<float>(menus[i].items.size())};
      drawRect(dd, kPanel, 2.0f);
      for (std::size_t j = 0; j < menus[i].items.size(); ++j) {
        const auto& mi = menus[i].items[j];
        const Rect miRect{dd.x, dd.y + itemH * static_cast<float>(j),
                          dd.w, itemH};
        if (mi.separator) {
          drawRect({miRect.x + 4.0f, miRect.y + itemH * 0.5f,
                    miRect.w - 8.0f, 1.0f}, kBorder, 0.0f);
          continue;
        }
        drawText(mi.label.c_str(),
                 miRect.x + 8.0f, miRect.y + 4.0f, 1,
                 mi.disabled ? kTextDim : kText);
        if (!mi.shortcut.empty()) {
          drawText(mi.shortcut.c_str(),
                   miRect.x + miRect.w -
                       static_cast<float>(mi.shortcut.size()) * 6.0f - 8.0f,
                   miRect.y + 4.0f, 1, kTextMuted);
        }
      }
    }
  }
  (void)std::max(0.0, 0.0);
}

}

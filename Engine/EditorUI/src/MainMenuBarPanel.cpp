#include <kimia/MainMenuBarPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
void drawMenuBar(const Rect& rect, const std::vector<Menu>& menus) {
  using namespace theme;
  drawRect(rect, kPanelAlt, 0.0f);

  f32 x = rect.x + 6.0f;
  const f32 midY = rect.y + rect.h * 0.5f - 5.0f;
  for (std::size_t m = 0; m < menus.size(); ++m) {
    const bool open = (menus[m].selectedIndex >= 0);
    if (open) {
      drawRect({x - 4.0f, rect.y, 60.0f, rect.h},
               kAccent, 0.0f);
    }
    drawText(menus[m].name.c_str(),
             x, midY, 1, open ? kAccentHot : kText);
    x += 60.0f;
  }
}
void drawOpenMenu(const Rect& rect, const Menu& menu) {
  using namespace theme;
  constexpr f32 rowH = 18.0f;
  const f32 w = 180.0f;
  const f32 h = menu.items.size() * rowH + 4.0f;
  // Place below menu bar.
  const Rect drop = {rect.x + 4.0f, rect.y + rect.h + 2.0f, w, h};
  drawRect(drop, kPanel, 2.0f);

  f32 y = drop.y + 2.0f;
  for (std::size_t i = 0; i < menu.items.size(); ++i) {
    if (menu.items[i].separator) {
      drawRect({drop.x + 4.0f, y + rowH * 0.5f,
                drop.w - 8.0f, 1.0f},
               kPanelAlt, 0.0f);
      y += rowH;
      continue;
    }
    const bool sel = (static_cast<i32>(i) == menu.selectedIndex);
    if (sel) {
      drawRect({drop.x + 2.0f, y,
                drop.w - 4.0f, rowH},
               kAccent, 0.0f);
    }
    drawText(menu.items[i].label.c_str(),
             drop.x + 8.0f, y + 2.0f, 1,
             menu.items[i].disabled ? kTextMuted :
             (sel ? kAccentHot : kText));
    if (!menu.items[i].shortcut.empty()) {
      drawText(menu.items[i].shortcut.c_str(),
               drop.x + drop.w - 50.0f, y + 2.0f, 1, kTextMuted);
    }
    y += rowH;
  }
}
}

void drawMainMenuBarPanel(const Rect& rect, const std::vector<Menu>& menus) {
  drawMenuBar(rect, menus);
  for (std::size_t i = 0; i < menus.size(); ++i) {
    if (menus[i].selectedIndex >= 0) {
      drawOpenMenu(rect, menus[i]);
      break; // only one open at a time
    }
  }
}

}

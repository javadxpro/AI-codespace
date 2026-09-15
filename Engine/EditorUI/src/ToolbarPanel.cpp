#include <kimia/ToolbarPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawToolbarPanel(const Rect& rect,
                      const std::vector<ToolbarButton>& buttons) {
  using namespace theme;
  drawRect(rect, kTitlebar, 0.0f);

  constexpr f32 btnH = 22.0f;
  const f32 btnW = std::max(28.0f, rect.h - 4.0f);
  f32 x = rect.x + 4.0f;
  const f32 y = rect.y + (rect.h - btnH) * 0.5f;

  for (std::size_t i = 0; i < buttons.size(); ++i) {
    const Rect btn{x, y, btnW, btnH};
    const bool pressed = buttons[i].toggle && buttons[i].toggledOn;
    drawRect(btn, pressed ? kAccent : kPanelAlt, 2.0f);
    // Glyph in the centre.
    const std::string& g = buttons[i].iconGlyph.empty()
        ? buttons[i].label : buttons[i].iconGlyph;
    drawText(g.c_str(),
             btn.x + 6.0f, btn.y + 6.0f, 1,
             pressed ? kAccentHot : kText);

    // Separator after each group of 4.
    if ((i + 1) % 4 == 0 && i + 1 < buttons.size()) {
      drawRect({x + btnW + 2.0f, btn.y + 2.0f,
                1.0f, btn.h - 4.0f}, kBorder, 0.0f);
      x += 6.0f;
    }

    x += btnW + 4.0f;
    if (x + btnW > rect.x + rect.w) break;
  }
}

}

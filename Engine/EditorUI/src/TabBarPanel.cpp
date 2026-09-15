#include <kimia/TabBarPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

void drawTabBarPanel(const Rect& rect,
                     const std::vector<Tab>& tabs,
                     i32 activeTab) {
  using namespace theme;
  drawRect(rect, kTitlebar, 0.0f);

  if (tabs.empty()) return;

  const f32 tabW = 100.0f;
  const f32 tabH = rect.h;
  f32 x = rect.x;
  for (std::size_t i = 0; i < tabs.size(); ++i) {
    if (x + tabW > rect.x + rect.w) break;
    const bool active = static_cast<i32>(i) == activeTab;
    const Rect tab{x, rect.y, tabW, tabH};
    drawRect(tab, active ? kPanel : kPanelAlt, 0.0f);

    // Optional glyph on the left.
    f32 textX = tab.x + 8.0f;
    if (!tabs[i].glyph.empty()) {
      drawText(tabs[i].glyph.c_str(),
               tab.x + 4.0f, tab.y + 4.0f, 1,
               active ? kAccent : kTextMuted);
      textX = tab.x + 16.0f;
    }
    // Dirty marker.
    if (tabs[i].dirty) {
      drawText("*", textX, tab.y + 4.0f, 1, kWarning);
      textX += 6.0f;
    }
    drawText(tabs[i].title.c_str(),
             textX, tab.y + 4.0f, 1,
             active ? kAccentHot : kText);

    // Close button on the right.
    if (tabs[i].closable) {
      drawText("x",
               tab.x + tabW - 12.0f, tab.y + 4.0f, 1, kTextMuted);
    }

    // Separator between tabs.
    if (!active) {
      drawRect({x + tabW - 1.0f, tab.y + 2.0f,
                1.0f, tabH - 4.0f}, kBorder, 0.0f);
    }
    x += tabW;
  }
  (void)std::max(0.0, 0.0);
}

}

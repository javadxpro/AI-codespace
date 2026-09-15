#include <kimia/DockLayoutPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
void drawDockTabs(const Rect& areaRect, DockSide side,
                  const std::vector<DockTab>& tabs, i32 activeTab) {
  using namespace theme;
  if (tabs.empty()) return;
  constexpr f32 tabH = 18.0f;
  const f32 tabW = 70.0f;
  f32 x = areaRect.x;
  const f32 y = (side == DockSide::Left || side == DockSide::Right)
      ? areaRect.y : areaRect.y;
  for (std::size_t i = 0; i < tabs.size(); ++i) {
    const Rect tab{x, y, tabW, tabH};
    const bool active = static_cast<i32>(i) == activeTab;
    drawRect(tab, active ? kAccentDim : kTitlebar, 0.0f);
    if (!tabs[i].glyph.empty()) {
      drawText(tabs[i].glyph.c_str(),
               tab.x + 4.0f, tab.y + 4.0f, 1, kText);
    }
    drawText(tabs[i].title.c_str(),
             tab.x + 14.0f, tab.y + 4.0f, 1,
             active ? kAccentHot : kText);
    x += tabW;
    if (x + tabW > areaRect.x + areaRect.w) break;
  }
  (void)side;
}
}

void drawDockLayoutPanel(const Rect& rect,
                         const std::vector<DockArea>& docks,
                         Rect& outCenter) {
  using namespace theme;
  drawRect(rect, kBg, 0.0f);

  // Compute center.
  f32 l = 0.0f, r = 0.0f, t = 0.0f, b = 0.0f;
  for (const auto& d : docks) {
    if (d.side == DockSide::Left)   l += d.size;
    if (d.side == DockSide::Right)  r += d.size;
    if (d.side == DockSide::Top)    t += d.size;
    if (d.side == DockSide::Bottom) b += d.size;
  }
  outCenter = {rect.x + l, rect.y + t, rect.w - l - r, rect.h - t - b};

  for (const auto& d : docks) {
    Rect dRect;
    switch (d.side) {
      case DockSide::Left:
        dRect = {rect.x + 0, rect.y, l, rect.h};
        break;
      case DockSide::Right:
        dRect = {rect.x + rect.w - r, rect.y, r, rect.h};
        break;
      case DockSide::Top:
        dRect = {rect.x + l, rect.y, rect.w - l - r, t};
        break;
      case DockSide::Bottom:
        dRect = {rect.x + l, rect.y + rect.h - b, rect.w - l - r, b};
        break;
      default:
        continue;
    }
    drawRect(dRect, kPanel, 0.0f);
    drawDockTabs(dRect, d.side, d.tabs, d.activeTab);
  }
}

}

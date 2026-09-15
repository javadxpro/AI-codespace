#include <kimia/GameProfilePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawGameProfilePanel(const Rect& rect,
                          const std::vector<GameProfileEntry>& profiles,
                          i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Game Profiles", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 28.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < profiles.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    if (profiles[i].isCurrent) {
      drawRect({rect.x, y, rect.w, rowH}, kAccentDim, 0.0f);
    }

    // Glyph badge.
    drawRect({rect.x + 4.0f, y + 4.0f, 20.0f, rowH - 8.0f},
             kPanelAlt, 2.0f);
    drawText(profiles[i].glyph.c_str(),
             rect.x + 10.0f, y + 8.0f, 1, kAccent);

    drawText(profiles[i].name.c_str(),
             rect.x + 30.0f, y + 4.0f, 1,
             profiles[i].isCurrent ? kAccentHot : kText);
    drawText(profiles[i].description.c_str(),
             rect.x + 30.0f, y + 16.0f, 1, kTextMuted);
  }
  popClip();
}

}

#include <kimia/LocalizationPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawLocalizationPanel(const Rect& rect,
                           const std::vector<LocaleEntry>& locales,
                           i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Locales", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 20.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  char buf[32];
  for (std::size_t i = 0; i < locales.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    if (locales[i].isCurrent) {
      drawRect({rect.x, y, rect.w, rowH}, kAccentDim, 0.0f);
    }

    drawText(locales[i].code.c_str(),
             rect.x + 4.0f, y + 4.0f, 1, kTextMuted);
    drawText(locales[i].displayName.c_str(),
             rect.x + 50.0f, y + 4.0f, 1,
             locales[i].isCurrent ? kAccentHot : kText);

    // Progress: translated / total.
    const f32 ratio = locales[i].stringCount > 0
        ? static_cast<f32>(locales[i].translatedCount) /
          static_cast<f32>(locales[i].stringCount)
        : 1.0f;
    const f32 barX = rect.x + rect.w - 60.0f;
    drawRect({barX, y + 7.0f, 40.0f, 4.0f}, kPanelAlt, 2.0f);
    drawRect({barX, y + 7.0f, 40.0f * ratio, 4.0f},
             ratio >= 1.0f ? kSuccess : kAccent, 2.0f);
    std::snprintf(buf, sizeof(buf), "%d/%d",
                  locales[i].translatedCount, locales[i].stringCount);
    drawText(buf,
             rect.x + rect.w - 18.0f, y + 4.0f, 1,
             kTextMuted);
  }
  popClip();
}

}

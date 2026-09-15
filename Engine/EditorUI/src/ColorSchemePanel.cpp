#include <kimia/ColorSchemePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
void drawSwatch(const Rect& r, const char* name, Color c) {
  using namespace theme;
  drawRect(r, kPanelAlt, 2.0f);
  drawRect({r.x + 4.0f, r.y + 4.0f, r.w - 8.0f, r.h - 14.0f},
           c, 2.0f);
  drawText(name, r.x + 4.0f, r.y + r.h - 10.0f, 1, kText);
}
}

void drawColorSchemePanel(const Rect& rect) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Color Scheme", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 swW = 70.0f;
  constexpr f32 swH = 36.0f;
  const f32 padX = 6.0f;
  const f32 padY = 20.0f;

  const struct { const char* name; Color c; } swatches[] = {
    {"Bg",          kBg},
    {"Panel",       kPanel},
    {"PanelAlt",    kPanelAlt},
    {"Titlebar",    kTitlebar},
    {"Border",      kBorder},
    {"Splitter",    kSplitter},
    {"Text",        kText},
    {"TextMuted",   kTextMuted},
    {"TextDim",     kTextDim},
    {"Accent",      kAccent},
    {"AccentHot",   kAccentHot},
    {"AccentDim",   kAccentDim},
    {"Success",     kSuccess},
    {"Warning",     kWarning},
    {"Error",       kError},
  };
  constexpr usize N = sizeof(swatches) / sizeof(swatches[0]);
  const int cols = std::max<i32>(1, static_cast<i32>((rect.w - padX * 2.0f) / (swW + 4.0f)));
  for (usize i = 0; i < N; ++i) {
    const int col = static_cast<i32>(i) % cols;
    const int row = static_cast<i32>(i) / cols;
    const f32 x = rect.x + padX + static_cast<float>(col) * (swW + 4.0f);
    const f32 y = rect.y + padY + static_cast<float>(row) * (swH + 4.0f);
    if (y + swH > rect.y + rect.h) break;
    drawSwatch({x, y, swW, swH}, swatches[i].name, swatches[i].c);
  }
}

}

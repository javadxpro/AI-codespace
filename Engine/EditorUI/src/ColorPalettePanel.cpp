#include <kimia/ColorPalettePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawColorPalettePanel(const Rect& rect,
                           const std::vector<PaletteSwatch>& swatches,
                           i32 selectedIndex) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Color Palette", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 swSize = 22.0f;
  constexpr f32 padding = 4.0f;
  const i32 cols = std::max(1, static_cast<i32>(
      (rect.w - padding) / (swSize + padding)));
  f32 x = rect.x + padding;
  f32 y = rect.y + 18.0f;

  for (std::size_t i = 0; i < swatches.size(); ++i) {
    if (y + swSize > rect.y + rect.h - 4.0f) break;
    const bool sel = (static_cast<i32>(i) == selectedIndex);
    drawRect({x, y, swSize, swSize},
             {swatches[i].r / 255.0f,
              swatches[i].g / 255.0f,
              swatches[i].b / 255.0f,
              1.0f},
             sel ? 2.0f : 1.0f);
    if (sel) {
      drawRect({x - 1.0f, y - 1.0f,
                swSize + 2.0f, swSize + 2.0f},
               kAccentHot, 2.0f);
    }

    // Advance.
    if ((static_cast<i32>(i + 1) % cols) == 0) {
      x = rect.x + padding;
      y += swSize + padding;
    } else {
      x += swSize + padding;
    }
  }

  // Selected details.
  if (selectedIndex >= 0 &&
      selectedIndex < static_cast<i32>(swatches.size())) {
    const auto& s = swatches[selectedIndex];
    const f32 infoY = rect.y + rect.h - 20.0f;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s  #%02X%02X%02X  RGB(%d,%d,%d)",
                  s.name.c_str(), s.r, s.g, s.b,
                  static_cast<int>(s.r),
                  static_cast<int>(s.g),
                  static_cast<int>(s.b));
    drawText(buf, rect.x + 4.0f, infoY, 1, kText);
  }
}

}

#pragma once
#include "EditorUI.h"
#include <vector>

namespace kimia::ui {

struct PaletteSwatch {
  std::string name;
  u8 r = 255, g = 255, b = 255;
};

void drawColorPalettePanel(const Rect& rect,
                           const std::vector<PaletteSwatch>& swatches,
                           i32 selectedIndex);

}

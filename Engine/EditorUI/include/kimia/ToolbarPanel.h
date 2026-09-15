#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct ToolbarButton {
  std::string label;
  std::string iconGlyph;
  int id = 0;
  bool toggle = false;
  bool toggledOn = false;
};

void drawToolbarPanel(const Rect& rect,
                      const std::vector<ToolbarButton>& buttons);

}

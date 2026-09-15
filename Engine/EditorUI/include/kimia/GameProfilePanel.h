#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct GameProfileEntry {
  std::string name;
  std::string description;
  std::string glyph;
  bool isCurrent = false;
};

void drawGameProfilePanel(const Rect& rect,
                          const std::vector<GameProfileEntry>& profiles,
                          i32 scrollY);

}

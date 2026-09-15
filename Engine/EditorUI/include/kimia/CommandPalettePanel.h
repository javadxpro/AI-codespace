#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct PaletteCommand {
  std::string label;
  std::string shortcut;
  std::string category;
};

void drawCommandPalettePanel(const Rect& rect,
                             const std::string& query,
                             const std::vector<PaletteCommand>& allCommands,
                             i32 selectedIndex);

}

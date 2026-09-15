#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct HelpEntry {
  std::string shortcut;
  std::string description;
  std::string category;
};

void drawHelpPanel(const Rect& rect,
                   const std::vector<HelpEntry>& entries,
                   const std::string& filter,
                   i32 scrollY);

}

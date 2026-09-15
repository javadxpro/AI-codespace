#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct CheatEntry {
  std::string trigger;     // "gizmo move"
  std::string action;      // "press Q"
  std::string description; // "toggles Move gizmo"
};

void drawCheatSheetPanel(const Rect& rect,
                         const std::vector<CheatEntry>& entries,
                         i32 scrollY,
                         const std::string& searchQuery);

}

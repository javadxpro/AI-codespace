#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct InputBinding {
  std::string action;       // "Move forward"
  std::string primaryKey;   // "W"
  std::string secondaryKey; // "Up"
};

void drawInputPanel(const Rect& rect,
                    const std::vector<InputBinding>& bindings,
                    i32 scrollY);

}

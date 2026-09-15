#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct KeyBinding {
  std::string category;     // "File", "Edit", "View", "Tools"
  std::string action;
  std::string keys;
  bool conflict = false;
};

void drawKeyBindingsPanel(const Rect& rect,
                          const std::vector<KeyBinding>& bindings,
                          i32 scrollY,
                          const std::string& category);

}

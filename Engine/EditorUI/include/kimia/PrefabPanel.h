#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct PrefabEntry {
  std::string name;
  i32 instanceCount = 0;
  f32 lastModifiedSec = 0.0f;
};

void drawPrefabPanel(const Rect& rect,
                     const std::vector<PrefabEntry>& prefabs,
                     i32 scrollY);

}

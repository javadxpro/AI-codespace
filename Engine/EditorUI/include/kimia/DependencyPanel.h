#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct Dependency {
  std::string name;
  std::string kind;       // "Material", "Texture", "Model", "Script"
  bool direct = true;     // false = transitive
};

void drawDependencyPanel(const Rect& rect,
                          const std::vector<Dependency>& deps,
                          i32 scrollY);

}

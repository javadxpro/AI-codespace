#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct HierarchyNode {
  std::string name;
  i32 depth = 0;       // nesting depth for indentation
  bool expanded = true;
  bool visible = true;
  i32 parentIndex = -1;
};

void drawHierarchyPanel(const Rect& rect,
                        const std::vector<HierarchyNode>& nodes,
                        i32 selectedIndex,
                        i32 scrollY);

}

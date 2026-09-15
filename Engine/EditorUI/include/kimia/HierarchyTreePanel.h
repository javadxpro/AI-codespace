// HierarchyTreePanel — the indented tree view that shows parent
// / child relationships between entities. Phase 4+ lands the
// rendering and the expand/collapse state; Phase 5+ wires the
// drag-to-reparent gesture.
#pragma once

#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

struct HierarchyNode {
  std::string name;
  int depth = 0;        // 0 = root
  bool expanded = true; // arrow points down
  bool hasChildren = false;
  bool selected = false;
};

void drawHierarchyTreePanel(const Rect& rect,
                            const std::vector<HierarchyNode>& nodes,
                            i32 scrollY);

}  // namespace kimia::ui

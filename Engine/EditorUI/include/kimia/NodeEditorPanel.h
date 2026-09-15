#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct NodeGraphNode {
  std::string name;
  Vec2 position{0.0, 0.0};
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
};

struct NodeGraphLink {
  i32 fromNode = -1;     // index into nodes
  i32 fromOutput = 0;
  i32 toNode = -1;
  i32 toInput = 0;
};

void drawNodeEditorPanel(const Rect& rect,
                         const std::vector<NodeGraphNode>& nodes,
                         const std::vector<NodeGraphLink>& links,
                         Vec2 pan);

}

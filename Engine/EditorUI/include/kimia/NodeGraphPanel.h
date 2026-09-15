#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct NodePin {
  std::string name;
  bool output = false;
};

struct NodeItem {
  std::string title;
  f32 x = 0;
  f32 y = 0;
  f32 w = 100;
  f32 h = 80;
  u8 r = 60, g = 100, b = 160;
  std::vector<NodePin> pins;
};

struct NodeConnection {
  i32 fromNode = -1;
  i32 fromPin  = 0;
  i32 toNode   = -1;
  i32 toPin    = 0;
};

void drawNodeGraphPanel(const Rect& rect,
                        const std::vector<NodeItem>& nodes,
                        const std::vector<NodeConnection>& conns,
                        i32 selectedNode,
                        f32 panX, f32 panY,
                        f32 zoom);

}

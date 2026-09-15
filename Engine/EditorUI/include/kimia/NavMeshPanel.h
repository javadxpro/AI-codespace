#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct NavMeshProps {
  f32 cellSize = 0.3f;
  f32 cellHeight = 0.2f;
  f32 agentHeight = 2.0f;
  f32 agentRadius = 0.5f;
  f32 climbHeight = 0.5f;
  f32 slopeDeg = 45.0f;
  i32 tileSize = 64;
  bool built = false;
  i32 polyCount = 0;
  i32 vertCount = 0;
};

void drawNavMeshPanel(const Rect& rect, const NavMeshProps& props);

}

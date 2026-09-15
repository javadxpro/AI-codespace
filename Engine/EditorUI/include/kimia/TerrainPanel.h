#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct TerrainProps {
  i32 width = 64;
  i32 depth = 64;
  f32 spacing = 1.0f;
  f32 heightScale = 8.0f;
  f32 noiseFrequency = 0.1f;
  i32 seed = 12345;
  Vec3 baseColor{0.4f, 0.6f, 0.3f};
  bool wireframe = false;
};

void drawTerrainPanel(const Rect& rect, const TerrainProps& props);

}

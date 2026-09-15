#pragma once
#include "EditorUI.h"
#include <vector>

namespace kimia::ui {

struct MiniMapItem {
  f32 worldX = 0;
  f32 worldY = 0;
  f32 worldW = 1;
  f32 worldH = 1;
  u8 r = 200;
  u8 g = 200;
  u8 b = 200;
};

void drawMiniMapPanel(const Rect& rect,
                      const std::vector<MiniMapItem>& items,
                      f32 worldMinX, f32 worldMinY,
                      f32 worldMaxX, f32 worldMaxY,
                      f32 viewportX, f32 viewportY,
                      f32 viewportW, f32 viewportH);

}

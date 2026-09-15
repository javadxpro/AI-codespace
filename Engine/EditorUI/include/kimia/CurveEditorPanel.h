#pragma once
#include "EditorUI.h"
#include <vector>

namespace kimia::ui {

struct CurvePoint {
  f32 x = 0;
  f32 y = 0;
  i32 selected = 0; // bitmask: 1=left, 2=right
};

void drawCurveEditorPanel(const Rect& rect,
                          std::vector<CurvePoint>& points,
                          f32 minX, f32 maxX,
                          f32 minY, f32 maxY,
                          i32 selectedIndex);

}

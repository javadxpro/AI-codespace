#pragma once
#include "EditorUI.h"
#include <vector>

namespace kimia::ui {

struct GraphSeries {
  std::vector<f32> values;
  u8 r = 80;
  u8 g = 200;
  u8 b = 240;
  bool fill = true;
};

void drawGraphPanel(const Rect& rect,
                    const std::vector<GraphSeries>& series,
                    f32 minY,
                    f32 maxY);

}

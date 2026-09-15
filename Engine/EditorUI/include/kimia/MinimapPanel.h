#pragma once
#include "EditorUI.h"
#include <vector>

namespace kimia::ui {

struct MinimapDot {
  Vec3 worldPos{0.0, 0.0, 0.0};
  Color color{1.0f, 1.0f, 1.0f, 1.0f};
};

void drawMinimapPanel(const Rect& rect,
                      const Vec2& viewCenter,
                      f32 viewSize,
                      const std::vector<MinimapDot>& dots);

}

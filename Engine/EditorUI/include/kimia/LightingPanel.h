#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class LightKind { Directional, Point, Spot };

struct LightProps {
  LightKind kind = LightKind::Directional;
  Vec3 direction{0.0, -1.0, 0.0};
  Vec3 position{0.0, 5.0, 0.0};
  Vec3 color{1.0, 1.0, 1.0};
  f32 intensity = 1.0f;
  f32 range = 10.0f;
  f32 spotAngleDeg = 45.0f;
  bool castsShadows = true;
};

void drawLightingPanel(const Rect& rect, const LightProps& props);

}

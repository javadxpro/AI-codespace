#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct SnapProps {
  bool snapEnabled = true;
  f32 positionStep = 0.5f;
  f32 rotationStepDeg = 15.0f;
  f32 scaleStep = 0.1f;
  bool snapToGrid = true;
  bool snapToSurface = false;
  bool snapToAngle = true;
  f32 gridSize = 1.0f;
};

void drawSnapPanel(const Rect& rect, const SnapProps& props);

}

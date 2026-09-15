#pragma once
#include "EditorUI.h"
#include <vector>

namespace kimia::ui {

enum class CurveInterp { Linear, Step, Smooth };

struct CurveKey {
  float t = 0.0f;       // time (0..1)
  float v = 0.0f;       // value
  CurveInterp interp = CurveInterp::Linear;
};

void drawCurveEditorPanel(const Rect& rect,
                          const std::vector<CurveKey>& keys);

}

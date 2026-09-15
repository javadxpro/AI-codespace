#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct PostProcessProps {
  bool bloom = true;
  f32 bloomIntensity = 0.5f;
  f32 bloomThreshold = 1.0f;
  bool tonemap = true;
  f32 exposure = 1.0f;
  bool ssao = false;
  f32 ssaoRadius = 0.5f;
  bool fxaa = true;
  bool vignette = false;
  f32 vignetteIntensity = 0.3f;
  bool motionBlur = false;
};

void drawPostProcessPanel(const Rect& rect, const PostProcessProps& props);

}

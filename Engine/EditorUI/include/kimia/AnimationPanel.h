#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class AnimationLoop { Once, Loop, PingPong };

struct AnimationProps {
  std::string clipName;
  f32 durationSec = 1.0f;
  f32 currentTime = 0.0f;
  f32 playbackRate = 1.0f;
  AnimationLoop loop = AnimationLoop::Loop;
  bool playing = false;
  bool paused = false;
};

void drawAnimationPanel(const Rect& rect, const AnimationProps& props);

}

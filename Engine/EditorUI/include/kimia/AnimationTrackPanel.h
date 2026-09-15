#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct AnimKeyframe {
  f32 time = 0;
  f32 value = 0;
  bool selected = false;
};

struct AnimTrack {
  std::string name;
  std::vector<AnimKeyframe> keys;
};

void drawAnimationTrackPanel(const Rect& rect,
                             const std::vector<AnimTrack>& tracks,
                             f32 viewStart, f32 viewEnd,
                             f32 playheadTime,
                             i32 selectedTrack);

}

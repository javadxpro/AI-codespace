#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct TimelineBlock {
  std::string label;
  f32 startTime = 0;
  f32 duration = 1;
  u8 r = 80;
  u8 g = 200;
  u8 b = 240;
  bool selected = false;
};

void drawTimelinePanel(const Rect& rect,
                       const std::vector<TimelineBlock>& blocks,
                       f32 viewStart, f32 viewEnd,
                       f32 playheadTime);

}

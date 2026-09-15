// Timeline — the bottom strip that shows the playhead, transport
// buttons (play / pause / step / stop) and the current frame /
// total frames while playing.
//
// Phase 4+ placeholder: draws the four transport buttons + a
// thin horizontal line that represents the playhead. Drag-to-scrub
// is wired (returns a UiCommand of kind ToolChanged with the new
// playhead position) but the WorldEditor doesn't yet consume it;
// that lands in Phase 5.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

struct TimelineState {
  bool playing = false;
  bool paused = false;
  f32 playhead = 0.0f;   // 0..1 normalized
  f32 totalTime = 0.0f;  // seconds since play started
};

void drawTimeline(const Rect& rect, const TimelineState& state);

}  // namespace kimia::ui

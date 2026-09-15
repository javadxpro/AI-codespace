// StatsPanel — the small overlay in a corner of the editor that
// shows the live FPS, the entity / vertex / draw-call counts,
// and the engine's resident memory.
//
// Phase 4+ is read-only: the WorldEditor populates a Stats
// struct every frame and this panel just renders it.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

struct Stats {
  f32 fps = 60.0f;
  f32 frameMs = 16.0f;
  usize entityCount = 0;
  usize vertexCount = 0;
  usize drawCalls = 0;
  usize memoryMb = 0;
};

void drawStatsPanel(const Rect& rect, const Stats& stats);

}  // namespace kimia::ui

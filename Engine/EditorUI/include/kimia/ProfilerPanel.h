// ProfilerPanel — the overlay that shows the per-system frame time
// breakdown: Update / Physics / Render / Editor / GPU. Each row
// shows the system's last frame time in milliseconds and a small
// horizontal bar visualising it relative to the longest system.
//
// Phase 4+ is read-only: the Profiler struct is populated by the
// engine each frame and this panel just renders it.
#pragma once

#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

struct ProfilerRow {
  std::string label;       // "Update" / "Physics" / "Render" / ...
  f32 lastMs = 0.0f;
  f32 maxMs = 0.0f;        // used to scale the bar (0..1 normalised)
};

void drawProfilerPanel(const Rect& rect,
                       const std::vector<ProfilerRow>& rows);

}  // namespace kimia::ui

#pragma once
#include "EditorUI.h"
#include <vector>

namespace kimia::ui {

// A sparkline (mini line graph) used for the per-frame-time
// history shown at the top of the Profiler overlay. Phase 4+
// renders one polyline across the rect; Phase 5+ wires the live
// ring buffer of frame times.
void drawProfilerGraph(const Rect& rect,
                       const std::vector<float>& samples,
                       float maxValue);

}

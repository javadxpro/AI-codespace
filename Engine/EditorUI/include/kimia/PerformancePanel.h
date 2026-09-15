#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct PerfStats {
  f32 fps = 60.0f;
  f32 frameMs = 16.0f;
  f32 cpuMs = 4.0f;
  f32 gpuMs = 8.0f;
  f32 memMb = 256.0f;
  u64 drawCalls = 200;
  u64 triangles = 100000;
  u64 verts = 50000;
};

void drawPerformancePanel(const Rect& rect, const PerfStats& stats);

}

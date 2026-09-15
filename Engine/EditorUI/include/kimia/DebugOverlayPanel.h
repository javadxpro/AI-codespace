#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct DebugReadout {
  f32 fps = 60.0f;
  f32 frameMs = 16.0f;
  f32 memMb = 256.0f;
  u32 drawCalls = 0;
  u32 verts = 0;
  std::string scene;
  std::string camera;
};

void drawDebugOverlayPanel(const Rect& rect, const DebugReadout& r);

}

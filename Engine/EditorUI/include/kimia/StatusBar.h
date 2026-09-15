// StatusBar — the single-line strip at the very bottom of the
// editor. Shows the current tool (Select / Move / Rotate / Scale),
// the entity count, the FPS, the play / pause state, and any
// transient hint ("Saved to foo.kimia", "Loading…").
//
// Phase 4+ is read-only — the Status struct is updated by the
// editor every frame and this bar just renders it.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

struct Status {
  const char* tool = "Select";     // "Select" / "Move" / "Rotate" / "Scale"
  usize entityCount = 0;
  f32 fps = 60.0f;
  bool playing = false;
  bool paused = false;
  const char* hint = "";           // transient message (empty = hidden)
};

void drawStatusBar(const Rect& rect, const Status& s);

}  // namespace kimia::ui

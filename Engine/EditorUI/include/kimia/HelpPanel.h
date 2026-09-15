// HelpPanel — the overlay that lists the editor's keyboard / touch
// shortcuts (Ctrl+Z undo, Ctrl+Shift+Z redo, F5 play, F6 pause,
// etc) and shows the engine version + build info at the bottom.
//
// Phase 4+: static render. The shortcut list is the same on every
// device so there's no per-user state.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

struct Shortcut {
  const char* keys;        // "Ctrl+Z"
  const char* description; // "Undo"
};

constexpr Shortcut kShortcuts[] = {
  {"Ctrl+Z",         "Undo"},
  {"Ctrl+Shift+Z",   "Redo"},
  {"F5",             "Play / Stop"},
  {"F6",             "Pause / Resume"},
  {"F7",             "Step one frame"},
  {"Shift+Tap",      "Toggle selection"},
  {"Drag",           "Box-select in Scene View"},
  {"Q / W / E / R",  "Select / Move / Rotate / Scale"},
  {"Ctrl+S",         "Save"},
  {"Ctrl+Shift+S",   "Save As"},
  {"Ctrl+O",         "Open"},
  {"Delete",         "Delete selected"},
  {"Ctrl+D",         "Duplicate selected"},
  {"F1",             "Toggle this help"},
};

void drawHelpPanel(const Rect& rect);

}  // namespace kimia::ui

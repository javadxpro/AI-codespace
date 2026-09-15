// DialogPanel — the modal confirmation dialog. Phase 4+ lays the
// API: title + message + up to 4 buttons. The center of the rect
// is the focus point; the buttons sit at the bottom.
//
// Phase 4+ is a passive renderer. Phase 5+ wires the button click
// to a UiCommand of kind DialogClosed with the chosen index.
#pragma once

#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

struct DialogButton {
  std::string label;
  int id = 0;     // user-defined id returned via UiCommand
};

struct DialogState {
  std::string title;
  std::string message;
  std::vector<DialogButton> buttons;
  bool open = false;
};

void drawDialogPanel(const Rect& rect, const DialogState& state);

}  // namespace kimia::ui

// ContextMenuPanel — the right-click / long-press popup menu. The
// editor uses it everywhere (Object Tree row → Rename / Delete /
// Duplicate; Scene View entity → Move Here / Create Cube / etc;
// empty Scene View → Create Cube / Sphere / Plane / Player).
//
// Phase 4+ is a passive renderer — the menu items are passed in
// as a flat list of (label, id) pairs and the click handler
// lands in Phase 5+.
#pragma once

#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

struct ContextMenuItem {
  std::string label;
  int id = 0;
  bool separator = false;  // render as a horizontal divider
  bool disabled = false;
};

struct ContextMenuState {
  std::vector<ContextMenuItem> items;
  bool open = false;
};

void drawContextMenu(f32 x, f32 y, const ContextMenuState& state);

}  // namespace kimia::ui

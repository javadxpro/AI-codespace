// ObjectTree — the left-hand dock that lists every entity in the
// current scene. Each entry shows its name + a small colour
// swatch. Selected entries are drawn in the accent colour.
//
// Phase 4+: pure render. The WorldEditor owns the canonical
// entity list and pushes it through SceneSnapshot.entities; this
// panel just iterates that list. Click handling (select / shift-
// select) lands in Phase 5.
#pragma once

#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct ObjectTreeEntry {
  std::string name;
  bool selected = false;
  Color swatch{0.7f, 0.7f, 0.7f, 1.0f};
};

void drawObjectTree(const Rect& rect,
                    const std::vector<ObjectTreeEntry>& entries,
                    i32 scrollY);

}  // namespace kimia::ui

// PropertySheet — the right-hand dock that exposes the editable
// properties of the currently selected entity (Position / Rotation
// / Scale / Color / Material / Locked). Phase 4+ is a read-only
// inspector: it draws the current values but the inline inputs /
// colour picker / numeric draggers land in Phase 5.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

struct EntityProps {
  std::string name;
  Vec3 position{0.0, 0.0, 0.0};
  Vec3 rotationEuler{0.0, 0.0, 0.0};  // degrees
  Vec3 scale{1.0, 1.0, 1.0};
  Vec3 color{0.7f, 0.7f, 0.7f};
  f32 roughness = 0.5f;
  f32 metalness = 0.0f;
  bool locked = false;
  bool isPlayer = false;
  bool isBall = false;
};

void drawPropertySheet(const Rect& rect, const EntityProps& props);

}  // namespace kimia::ui

// PhysicsPanel — the right-hand dock that exposes the collider /
// rigidbody settings of the currently selected entity. Mirrors the
// Property Sheet but only shows the physics-relevant rows:
// shape (Box / Sphere / Plane), mass, friction, restitution, and
// whether the body is dynamic or static.
//
// Phase 4+ placeholder: reads from a flat struct (PhysicsProps) the
// editor snapshots from the WorldEditor. The full integration with
// the engine's collider system lands in Phase 5.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

enum class ColliderShape {
  Box,
  Sphere,
  Plane,
  Capsule,
};

struct PhysicsProps {
  ColliderShape shape = ColliderShape::Box;
  f32 mass = 1.0f;            // 0 = static (infinite mass)
  f32 friction = 0.5f;
  f32 restitution = 0.1f;     // bounciness
  Vec3 extents{1.0, 1.0, 1.0};  // box half-extents OR sphere radius (x)
  bool isTrigger = false;
  bool isKinematic = false;
};

// Render the physics panel. Returns true if any property was edited
// (the caller is expected to push the resulting UiCommand onto the
// UndoStack so the user can Ctrl+Z the change).
bool drawPhysicsPanel(const Rect& rect, const PhysicsProps& current,
                      PhysicsProps& edited);

}  // namespace kimia::ui

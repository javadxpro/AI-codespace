// PhysicsPanel tests — see Engine/EditorUI/include/kimia/PhysicsPanel.h.

#include <kimia_test.h>
#include <kimia/PhysicsPanel.h>

KIMIA_TEST(PhysicsPanel_DrawDefaultDoesNotCrash) {
  kimia::ui::PhysicsProps p;
  kimia::ui::PhysicsProps e;
  const bool changed = kimia::ui::drawPhysicsPanel(
      {0, 0, 200, 200}, p, e);
  // Phase 4+: read-only — never reports a change.
  KIMIA_REQUIRE(!changed);
}

KIMIA_TEST(PhysicsPanel_DrawBoxDoesNotCrash) {
  kimia::ui::PhysicsProps p;
  p.shape = kimia::ui::ColliderShape::Box;
  p.mass = 5.0f;
  p.friction = 0.7f;
  p.restitution = 0.2f;
  p.extents = {1.0, 2.0, 3.0};
  kimia::ui::PhysicsProps e = p;
  KIMIA_REQUIRE(!kimia::ui::drawPhysicsPanel({0, 0, 240, 220}, p, e));
}

KIMIA_TEST(PhysicsPanel_DrawSphereDoesNotCrash) {
  kimia::ui::PhysicsProps p;
  p.shape = kimia::ui::ColliderShape::Sphere;
  p.extents = {0.5, 0.0, 0.0};  // sphere uses x as radius
  kimia::ui::PhysicsProps e = p;
  KIMIA_REQUIRE(!kimia::ui::drawPhysicsPanel({0, 0, 240, 220}, p, e));
}

KIMIA_TEST(PhysicsPanel_DrawPlaneStaticDoesNotCrash) {
  kimia::ui::PhysicsProps p;
  p.shape = kimia::ui::ColliderShape::Plane;
  p.mass = 0.0f;  // static
  p.friction = 1.0f;
  p.restitution = 0.0f;
  kimia::ui::PhysicsProps e = p;
  KIMIA_REQUIRE(!kimia::ui::drawPhysicsPanel({0, 0, 240, 220}, p, e));
}

KIMIA_TEST(PhysicsPanel_DrawKinematicCapsuleDoesNotCrash) {
  kimia::ui::PhysicsProps p;
  p.shape = kimia::ui::ColliderShape::Capsule;
  p.isKinematic = true;
  p.mass = 2.5f;
  kimia::ui::PhysicsProps e = p;
  KIMIA_REQUIRE(!kimia::ui::drawPhysicsPanel({0, 0, 240, 220}, p, e));
}

KIMIA_TEST(PhysicsPanel_DrawWithTriggerFlagDoesNotCrash) {
  kimia::ui::PhysicsProps p;
  p.isTrigger = true;
  kimia::ui::PhysicsProps e;
  e.isTrigger = true;
  KIMIA_REQUIRE(!kimia::ui::drawPhysicsPanel({0, 0, 240, 220}, p, e));
}

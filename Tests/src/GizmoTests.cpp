// Gizmo tests — see Engine/EditorUI/include/kimia/Gizmo.h.
//
// Phase 4 places only the API surface and the "yellow marker"
// stub renderer. These tests pin the API shape:
//   * gizmoHitTest compiles, returns no-hit (we're not yet
//     ray-testing).
//   * drawGizmo compiles and never crashes for any reasonable
//     projection / origin pair.

#include <kimia_test.h>
#include <kimia/Gizmo.h>
#include <kimia/Mat4.h>

namespace {

kimia::Mat4 lookingAtOrigin() {
  kimia::Mat4 m{};
  for (kimia::i32 i = 0; i < 16; ++i) m.m_[i] = 0.0;
  m.at(0,0) = 1.0; m.at(1,1) = 1.0; m.at(2,2) = 1.0; m.at(3,3) = 1.0;
  return m;
}

kimia::Mat4 identityProjection() { return kimia::Mat4{}; }

}  // namespace

KIMIA_TEST(Gizmo_HitTestMoveNoHit) {
  const auto hit = kimia::ui::gizmoHitTest(
      {0.0, 0.0, 0.0}, lookingAtOrigin(),
      1024, 720, kimia::ui::GizmoMode::Move,
      512.0, 360.0);
  KIMIA_REQUIRE(!hit.hit);
  KIMIA_REQUIRE(hit.axis == -1);
}

KIMIA_TEST(Gizmo_HitTestRotateNoHit) {
  const auto hit = kimia::ui::gizmoHitTest(
      {0.0, 0.0, 0.0}, lookingAtOrigin(),
      1024, 720, kimia::ui::GizmoMode::Rotate,
      100.0, 100.0);
  KIMIA_REQUIRE(!hit.hit);
}

KIMIA_TEST(Gizmo_HitTestScaleNoHit) {
  const auto hit = kimia::ui::gizmoHitTest(
      {0.0, 0.0, 0.0}, lookingAtOrigin(),
      1024, 720, kimia::ui::GizmoMode::Scale,
      999.0, 999.0);
  KIMIA_REQUIRE(!hit.hit);
}

KIMIA_TEST(Gizmo_DrawGizmoOriginBehindCameraDoesNotCrash) {
  kimia::ui::drawGizmo({-1000.0, -1000.0, -1000.0},
                       kimia::ui::GizmoMode::Rotate,
                       identityProjection(), 1024, 720);
}

KIMIA_TEST(Gizmo_DrawGizmoOriginInFrontDoesNotCrash) {
  kimia::ui::drawGizmo({0.0, 0.0, 0.0},
                       kimia::ui::GizmoMode::Move,
                       lookingAtOrigin(), 1024, 720);
}

KIMIA_TEST(Gizmo_DrawGizmoAllModesCompile) {
  const kimia::Mat4 m = lookingAtOrigin();
  kimia::ui::drawGizmo({1.0, 2.0, 3.0}, kimia::ui::GizmoMode::Select,
                       m, 800, 600);
  kimia::ui::drawGizmo({1.0, 2.0, 3.0}, kimia::ui::GizmoMode::Move,
                       m, 800, 600);
  kimia::ui::drawGizmo({1.0, 2.0, 3.0}, kimia::ui::GizmoMode::Rotate,
                       m, 800, 600);
  kimia::ui::drawGizmo({1.0, 2.0, 3.0}, kimia::ui::GizmoMode::Scale,
                       m, 800, 600);
}

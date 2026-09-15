// Gizmo implementation — see Gizmo.h.
//
// Phase 4 placeholder: the API is wired so the Scene View can ask
// "did the user grab an axis?" and draw a small marker. The real
// picking math (ray-vs-arrow, ray-vs-ring, ray-vs-cube) and the
// line / ring renderers land in Phase 5.
#include <kimia/Gizmo.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>

namespace kimia::ui {

GizmoHit gizmoHitTest(const Vec3&, const Mat4&,
                      i32, i32, GizmoMode,
                      f64, f64) {
  GizmoHit hit;
  return hit;
}

void drawGizmo(const Vec3& worldOrigin, GizmoMode /*mode*/,
               const Mat4& viewProjection,
               i32 viewportW, i32 viewportH) {
  // Convert the world-space origin into screen-space via the matrix
  // stack the editor already uses for the Scene View.
  const f64 px = viewProjection.at(0,0)*worldOrigin.x
               + viewProjection.at(1,0)*worldOrigin.y
               + viewProjection.at(2,0)*worldOrigin.z
               + viewProjection.at(3,0);
  const f64 py = viewProjection.at(0,1)*worldOrigin.x
               + viewProjection.at(1,1)*worldOrigin.y
               + viewProjection.at(2,1)*worldOrigin.z
               + viewProjection.at(3,1);
  const f64 pw = viewProjection.at(0,3)*worldOrigin.x
               + viewProjection.at(1,3)*worldOrigin.y
               + viewProjection.at(2,3)*worldOrigin.z
               + viewProjection.at(3,3);
  if (pw <= 1e-9) return;
  const f64 ndcX = px / pw;
  const f64 ndcY = py / pw;
  const f64 sx = (ndcX * 0.5 + 0.5) * static_cast<f64>(viewportW);
  const f64 sy = (1.0 - (ndcY * 0.5 + 0.5)) * static_cast<f64>(viewportH);

  // 12x12 marker so the user sees the gizmo anchor even on a phone
  // screen.
  const f64 marker = 6.0;
  drawRect({sx - marker, sy - marker, marker * 2.0, marker * 2.0},
           {1.0, 0.85, 0.15, 1.0});
}

}  // namespace kimia::ui

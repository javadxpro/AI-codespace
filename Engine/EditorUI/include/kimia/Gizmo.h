// Gizmo — the 3D translation / rotation / scale handles that the
// Scene View draws on top of the selected entity.
//
// Phase 4 placeholder: the API is wired so the Scene View can ask
// "did the user grab an axis?" and draw a small marker. The real
// picking math (ray-vs-arrow, ray-vs-ring, ray-vs-cube) and the
// line / ring renderers land in Phase 5. Until then drawGizmo
// emits a single yellow square at the projected screen position
// of the world-space origin passed in, and gizmoHitTest returns
// no-hit so the Scene View never accidentally drags anything.
#pragma once

#include "EditorUI.h"
#include <kimia/Vec.h>
#include <kimia/Mat4.h>

namespace kimia::ui {

enum class GizmoMode {
  Select,  // no handle, just bounding box highlight
  Move,    // 3 axis arrows + 3 plane squares
  Rotate,  // 3 axis rings
  Scale,   // 3 axis cubes + uniform-scale cube
};

struct GizmoHit {
  bool hit = false;
  // Which axis (-1 = none / uniform, 0=x, 1=y, 2=z) and whether it's
  // the positive or negative direction along that axis.
  i32 axis = -1;
  bool negative = false;
};

// Project a screen-space touch into the gizmo and return which axis
// the user grabbed. The editor calls this from the Scene View region
// in pointer-down events; once a hit is registered the gizmo drags
// the selected entity along that axis until pointer-up.
//
// Phase 4: returns no-hit. Phase 5 will ray-test against the
// arrow / ring / cube geometry.
GizmoHit gizmoHitTest(const Vec3& worldOrigin,
                      const Mat4& viewProjection,
                      i32 viewportW, i32 viewportH,
                      GizmoMode mode,
                      f64 mouseX, f64 mouseY);

// Render the gizmo into the DrawCmd list. The world-space origin is
// where the handles are anchored; the matrices are camera view * proj.
//
// Phase 4: emits a 6x6 yellow square at the projected screen position.
// Phase 5 will emit three axis arrows + three plane squares (Move),
// three axis rings (Rotate) or three axis cubes (Scale).
void drawGizmo(const Vec3& worldOrigin,
               GizmoMode mode,
               const Mat4& viewProjection,
               i32 viewportW, i32 viewportH);

}  // namespace kimia::ui

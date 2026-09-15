// CameraPanel — the dock that exposes the editor camera's
// parameters: orthographic vs perspective, FOV, near/far plane,
// and the orbit target. Phase 4+ is read-only; Phase 5+ will
// wire the inline inputs.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

enum class CameraProjection {
  Perspective,
  Orthographic,
};

struct CameraProps {
  CameraProjection projection = CameraProjection::Perspective;
  f32 fovYDeg = 60.0f;        // vertical field of view (perspective)
  f32 nearPlane = 0.1f;
  f32 farPlane = 1000.0f;
  Vec3 eye{0.0, 2.0, 5.0};    // camera position
  Vec3 target{0.0, 0.0, 0.0}; // orbit target
  Vec3 up{0.0, 1.0, 0.0};
  f32 orthoSize = 5.0f;       // half-height of ortho frustum
};

void drawCameraPanel(const Rect& rect, const CameraProps& props);

}  // namespace kimia::ui

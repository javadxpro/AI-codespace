#pragma once

#include <kimia/Mat4.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

namespace kimia {

// Perspective camera: right-handed, +Y up, -Z forward.
struct Camera {
  f64 fovYRadians = 1.0471975511965976;  // 60 degrees
  f64 aspect = 16.0 / 9.0;
  f64 nearPlane = 0.1;
  f64 farPlane = 100.0;
  Vec3 position{0.0, 0.0, 0.0};
  Vec3 target{0.0, 0.0, -1.0};
  Vec3 up{0.0, 1.0, 0.0};

  Mat4 projectionMatrix() const { return Mat4::perspective(fovYRadians, aspect, nearPlane, farPlane); }
  Mat4 viewMatrix() const { return Mat4::lookAt(position, target, up); }
  Mat4 viewProjectionMatrix() const { return projectionMatrix() * viewMatrix(); }
};

}  // namespace kimia

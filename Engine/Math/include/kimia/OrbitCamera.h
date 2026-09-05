#pragma once

#include <algorithm>
#include <cmath>

#include <kimia/Types.h>
#include <kimia/Vec.h>

namespace kimia {

// Orbit camera around a center point: yaw around +Y, pitch above the
// horizon, and a distance. All angles are radians. This is the editor
// camera model (arrow keys orbit, q/e zoom, c resets) — deterministic and
// fully CPU-side, so the software rasterizer on the phone shows the same
// view as the GL path.
struct OrbitCamera {
  // The classic overview: eye 6.0 in front of the target and 3.6 above it.
  static constexpr f64 kDefaultPitch = 0.5404195002705842;      // atan2(3.6, 6.0)
  static constexpr f64 kDefaultDistance = 6.997142273803751;    // sqrt(3.6^2 + 6.0^2)

  Vec3 center{0.0, 0.2, 0.0};
  f64 yaw = 0.0;
  f64 pitch = kDefaultPitch;
  f64 distance = kDefaultDistance;

  static constexpr f64 kMinPitch = 0.08726646259971647;  // 5 degrees
  static constexpr f64 kMaxPitch = 1.3613568165555772;   // 78 degrees
  static constexpr f64 kMinDistance = 2.5;
  static constexpr f64 kMaxDistance = 32.0;

  // Camera eye on the orbit sphere around the center.
  Vec3 eye() const {
    const f64 horizontal = std::cos(pitch) * distance;
    return center +
           Vec3{std::sin(yaw) * horizontal, std::sin(pitch) * distance, std::cos(yaw) * horizontal};
  }

  Vec3 target() const { return center; }

  // Positive dYaw orbits the camera to the left (eye moves toward -X when
  // looking along -Z); positive dPitch raises the camera above the scene.
  void orbit(f64 dYaw, f64 dPitch) {
    yaw += dYaw;
    pitch = std::clamp(pitch + dPitch, kMinPitch, kMaxPitch);
  }

  // factor < 1 gets closer, factor > 1 pulls away.
  void zoom(f64 factor) { distance = std::clamp(distance * factor, kMinDistance, kMaxDistance); }

  void reset() {
    yaw = 0.0;
    pitch = kDefaultPitch;
    distance = kDefaultDistance;
  }
};

}  // namespace kimia

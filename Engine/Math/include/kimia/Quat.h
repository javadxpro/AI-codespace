#pragma once

#include <kimia/Mat4.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <cmath>

namespace kimia {

// Unit quaternion rotation (x, y, z, w).
struct Quat {
  f64 x = 0.0;
  f64 y = 0.0;
  f64 z = 0.0;
  f64 w = 1.0;

  constexpr Quat() = default;
  constexpr Quat(f64 x_, f64 y_, f64 z_, f64 w_) : x(x_), y(y_), z(z_), w(w_) {}

  static Quat fromAxisAngle(const Vec3& axis, f64 angleRadians) {
    const Vec3 n = axis.normalized();
    const f64 half = angleRadians * 0.5;
    const f64 s = std::sin(half);
    return Quat{n.x * s, n.y * s, n.z * s, std::cos(half)};
  }

  static Quat fromMat4(const Mat4& m) {
    const f64 trace = m.at(0, 0) + m.at(1, 1) + m.at(2, 2);
    Quat q{};
    if (trace > 0.0) {
      f64 s = std::sqrt(trace + 1.0) * 2.0;  // 4w
      q.w = 0.25 * s;
      q.x = (m.at(1, 2) - m.at(2, 1)) / s;
      q.y = (m.at(2, 0) - m.at(0, 2)) / s;
      q.z = (m.at(0, 1) - m.at(1, 0)) / s;
    } else if (m.at(0, 0) > m.at(1, 1) && m.at(0, 0) > m.at(2, 2)) {
      f64 s = std::sqrt(1.0 + m.at(0, 0) - m.at(1, 1) - m.at(2, 2)) * 2.0;  // 4x
      q.w = (m.at(1, 2) - m.at(2, 1)) / s;
      q.x = 0.25 * s;
      q.y = (m.at(0, 1) + m.at(1, 0)) / s;
      q.z = (m.at(0, 2) + m.at(2, 0)) / s;
    } else if (m.at(1, 1) > m.at(2, 2)) {
      f64 s = std::sqrt(1.0 + m.at(1, 1) - m.at(0, 0) - m.at(2, 2)) * 2.0;  // 4y
      q.w = (m.at(2, 0) - m.at(0, 2)) / s;
      q.x = (m.at(0, 1) + m.at(1, 0)) / s;
      q.y = 0.25 * s;
      q.z = (m.at(1, 2) + m.at(2, 1)) / s;
    } else {
      f64 s = std::sqrt(1.0 + m.at(2, 2) - m.at(0, 0) - m.at(1, 1)) * 2.0;  // 4z
      q.w = (m.at(0, 1) - m.at(1, 0)) / s;
      q.x = (m.at(0, 2) + m.at(2, 0)) / s;
      q.y = (m.at(1, 2) + m.at(2, 1)) / s;
      q.z = 0.25 * s;
    }
    return q.normalized();
  }

  Quat normalized() const {
    const f64 len = std::sqrt(x * x + y * y + z * z + w * w);
    if (len < 1e-12) return Quat{};
    return Quat{x / len, y / len, z / len, w / len};
  }

  Quat conjugated() const { return Quat{-x, -y, -z, w}; }

  Quat operator*(const Quat& o) const {
    return Quat{
        w * o.x + x * o.w + y * o.z - z * o.y,
        w * o.y - x * o.z + y * o.w + z * o.x,
        w * o.z + x * o.y - y * o.x + z * o.w,
        w * o.w - x * o.x - y * o.y - z * o.z,
    };
  }

  Vec3 rotate(const Vec3& v) const {
    const Vec3 qv{x, y, z};
    const Vec3 t = cross(qv, v) * 2.0;
    return v + t * w + cross(qv, t);
  }

  Mat4 toMat4() const {
    Mat4 m{};
    m.at(0, 0) = 1.0 - 2.0 * (y * y + z * z);
    m.at(1, 0) = 2.0 * (x * y - w * z);
    m.at(2, 0) = 2.0 * (x * z + w * y);
    m.at(0, 1) = 2.0 * (x * y + w * z);
    m.at(1, 1) = 1.0 - 2.0 * (x * x + z * z);
    m.at(2, 1) = 2.0 * (y * z - w * x);
    m.at(0, 2) = 2.0 * (x * z - w * y);
    m.at(1, 2) = 2.0 * (y * z + w * x);
    m.at(2, 2) = 1.0 - 2.0 * (x * x + y * y);
    return m;
  }
};

}  // namespace kimia

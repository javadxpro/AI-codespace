#pragma once

#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <cmath>

namespace kimia {

// Column-major 4x4 matrix (right-handed, +Y up, -Z forward).
// Storage: m_[column * 4 + row].
struct Mat4 {
  f64 m_[16] = {
      1.0, 0.0, 0.0, 0.0,  //
      0.0, 1.0, 0.0, 0.0,  //
      0.0, 0.0, 1.0, 0.0,  //
      0.0, 0.0, 0.0, 1.0,  //
  };

  constexpr f64 at(i32 column, i32 row) const { return m_[static_cast<usize>(column * 4 + row)]; }
  constexpr f64& at(i32 column, i32 row) { return m_[static_cast<usize>(column * 4 + row)]; }

  constexpr Mat4 operator*(const Mat4& o) const {
    Mat4 result{};
    for (i32 c = 0; c < 4; ++c) {
      for (i32 r = 0; r < 4; ++r) {
        f64 sum = 0.0;
        for (i32 k = 0; k < 4; ++k) sum += at(k, r) * o.at(c, k);
        result.at(c, r) = sum;
      }
    }
    return result;
  }

  constexpr Vec4 operator*(const Vec4& v) const {
    return Vec4{
        at(0, 0) * v.x + at(1, 0) * v.y + at(2, 0) * v.z + at(3, 0) * v.w,
        at(0, 1) * v.x + at(1, 1) * v.y + at(2, 1) * v.z + at(3, 1) * v.w,
        at(0, 2) * v.x + at(1, 2) * v.y + at(2, 2) * v.z + at(3, 2) * v.w,
        at(0, 3) * v.x + at(1, 3) * v.y + at(2, 3) * v.z + at(3, 3) * v.w,
    };
  }

  // Affine transform of a point (w = 1, no perspective divide).
  constexpr Vec3 operator*(const Vec3& v) const {
    const Vec4 result = *this * Vec4{v.x, v.y, v.z, 1.0};
    return result.xyz();
  }

  // Transform of a direction (w = 0: translation is ignored).
  constexpr Vec3 transformDirection(const Vec3& v) const {
    const Vec4 result = *this * Vec4{v.x, v.y, v.z, 0.0};
    return result.xyz();
  }

  constexpr Mat4 transposed() const {
    Mat4 result{};
    for (i32 c = 0; c < 4; ++c) {
      for (i32 r = 0; r < 4; ++r) result.at(c, r) = at(r, c);
    }
    return result;
  }

  constexpr f64 determinant() const {
    // Cofactor expansion along the first row.
    f64 det = 0.0;
    for (i32 c = 0; c < 4; ++c) {
      const f64 cofactor = (c % 2 == 0 ? 1.0 : -1.0) * at(c, 0) * minor3(c, 0);
      det += cofactor;
    }
    return det;
  }

  constexpr Mat4 inverse() const {
    Mat4 result{};
    const f64 det = determinant();
    if (det == 0.0) return Mat4{};  // Singular: fall back to identity.
    const f64 invDet = 1.0 / det;
    for (i32 c = 0; c < 4; ++c) {
      for (i32 r = 0; r < 4; ++r) {
        const f64 cofactor = ((c + r) % 2 == 0 ? 1.0 : -1.0) * minor3(r, c);
        result.at(c, r) = cofactor * invDet;  // transposed cofactor matrix
      }
    }
    return result;
  }

  // Normal matrix: inverse-transpose (correct under non-uniform scale).
  Mat4 inverseTranspose() const { return inverse().transposed(); }

  static constexpr Mat4 translation(const Vec3& t) {
    Mat4 result{};
    result.at(3, 0) = t.x;
    result.at(3, 1) = t.y;
    result.at(3, 2) = t.z;
    return result;
  }

  static constexpr Mat4 scaling(const Vec3& s) {
    Mat4 result{};
    result.at(0, 0) = s.x;
    result.at(1, 1) = s.y;
    result.at(2, 2) = s.z;
    return result;
  }

  static Mat4 rotationX(f64 radians) {
    Mat4 result{};
    const f64 c = std::cos(radians);
    const f64 s = std::sin(radians);
    result.at(1, 1) = c;
    result.at(1, 2) = s;
    result.at(2, 1) = -s;
    result.at(2, 2) = c;
    return result;
  }

  static Mat4 rotationY(f64 radians) {
    Mat4 result{};
    const f64 c = std::cos(radians);
    const f64 s = std::sin(radians);
    result.at(0, 0) = c;
    result.at(2, 0) = s;
    result.at(0, 2) = -s;
    result.at(2, 2) = c;
    return result;
  }

  static Mat4 rotationZ(f64 radians) {
    Mat4 result{};
    const f64 c = std::cos(radians);
    const f64 s = std::sin(radians);
    result.at(0, 0) = c;
    result.at(0, 1) = s;
    result.at(1, 0) = -s;
    result.at(1, 1) = c;
    return result;
  }

  static constexpr Mat4 orthographic(f64 left, f64 right, f64 bottom, f64 top, f64 nearPlane, f64 farPlane) {
    Mat4 result{};
    result.at(0, 0) = 2.0 / (right - left);
    result.at(1, 1) = 2.0 / (top - bottom);
    result.at(2, 2) = -2.0 / (farPlane - nearPlane);
    result.at(3, 0) = -(right + left) / (right - left);
    result.at(3, 1) = -(top + bottom) / (top - bottom);
    result.at(3, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
    return result;
  }

  // Right-handed perspective projection (OpenGL-style, clip depth [-1, 1]).
  static Mat4 perspective(f64 fovYRadians, f64 aspect, f64 nearPlane, f64 farPlane) {
    const f64 tanHalfFovY = std::tan(fovYRadians * 0.5);
    Mat4 result{};
    result.at(0, 0) = 1.0 / (aspect * tanHalfFovY);
    result.at(1, 1) = 1.0 / tanHalfFovY;
    result.at(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
    result.at(2, 3) = -1.0;
    result.at(3, 2) = -(2.0 * farPlane * nearPlane) / (farPlane - nearPlane);
    result.at(3, 3) = 0.0;
    return result;
  }

  // Right-handed look-at view matrix (+Y up, -Z forward).
  static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    const Vec3 f = (target - eye).normalized();
    const Vec3 s = cross(f, up).normalized();
    const Vec3 u = cross(s, f);
    Mat4 result{};
    result.at(0, 0) = s.x;
    result.at(1, 0) = s.y;
    result.at(2, 0) = s.z;
    result.at(0, 1) = u.x;
    result.at(1, 1) = u.y;
    result.at(2, 1) = u.z;
    result.at(0, 2) = -f.x;
    result.at(1, 2) = -f.y;
    result.at(2, 2) = -f.z;
    result.at(3, 0) = -dot(s, eye);
    result.at(3, 1) = -dot(u, eye);
    result.at(3, 2) = dot(f, eye);
    return result;
  }

private:
  // Determinant of the 3x3 matrix obtained by removing (excludeCol, excludeRow).
  constexpr f64 minor3(i32 excludeCol, i32 excludeRow) const {
    f64 sub[9] = {};
    i32 out = 0;
    for (i32 c = 0; c < 4; ++c) {
      if (c == excludeCol) continue;
      for (i32 r = 0; r < 4; ++r) {
        if (r == excludeRow) continue;
        sub[out] = at(c, r);
        ++out;
      }
    }
    return sub[0] * (sub[4] * sub[8] - sub[5] * sub[7]) - sub[1] * (sub[3] * sub[8] - sub[5] * sub[6]) +
           sub[2] * (sub[3] * sub[7] - sub[4] * sub[6]);
  }
};

}  // namespace kimia

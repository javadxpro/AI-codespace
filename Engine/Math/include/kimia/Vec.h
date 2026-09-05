#pragma once

#include <kimia/Types.h>

#include <cmath>

namespace kimia {

// 2-component vector.
struct Vec2 {
  f64 x = 0.0;
  f64 y = 0.0;

  constexpr Vec2() = default;
  constexpr Vec2(f64 x_, f64 y_) : x(x_), y(y_) {}

  constexpr Vec2 operator+(const Vec2& o) const { return Vec2{x + o.x, y + o.y}; }
  constexpr Vec2 operator-(const Vec2& o) const { return Vec2{x - o.x, y - o.y}; }
  constexpr Vec2 operator-() const { return Vec2{-x, -y}; }
  constexpr Vec2 operator*(f64 s) const { return Vec2{x * s, y * s}; }
  constexpr Vec2 operator/(f64 s) const { return Vec2{x / s, y / s}; }
  Vec2& operator+=(const Vec2& o) {
    x += o.x;
    y += o.y;
    return *this;
  }
  Vec2& operator-=(const Vec2& o) {
    x -= o.x;
    y -= o.y;
    return *this;
  }
  Vec2& operator*=(f64 s) {
    x *= s;
    y *= s;
    return *this;
  }

  f64 lengthSquared() const { return x * x + y * y; }
  f64 length() const { return std::sqrt(lengthSquared()); }
  Vec2 normalized() const {
    const f64 len = length();
    if (len < 1e-12) return Vec2{};
    return Vec2{x / len, y / len};
  }
};

inline constexpr Vec2 operator*(f64 s, const Vec2& v) { return v * s; }
inline constexpr f64 dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

// 3-component vector.
struct Vec3 {
  f64 x = 0.0;
  f64 y = 0.0;
  f64 z = 0.0;

  constexpr Vec3() = default;
  constexpr Vec3(f64 x_, f64 y_, f64 z_) : x(x_), y(y_), z(z_) {}

  constexpr Vec3 operator+(const Vec3& o) const { return Vec3{x + o.x, y + o.y, z + o.z}; }
  constexpr Vec3 operator-(const Vec3& o) const { return Vec3{x - o.x, y - o.y, z - o.z}; }
  constexpr Vec3 operator-() const { return Vec3{-x, -y, -z}; }
  constexpr Vec3 operator*(f64 s) const { return Vec3{x * s, y * s, z * s}; }
  constexpr Vec3 operator/(f64 s) const { return Vec3{x / s, y / s, z / s}; }
  Vec3& operator+=(const Vec3& o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
  }
  Vec3& operator-=(const Vec3& o) {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
  }
  Vec3& operator*=(f64 s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  f64 lengthSquared() const { return x * x + y * y + z * z; }
  f64 length() const { return std::sqrt(lengthSquared()); }
  Vec3 normalized() const {
    const f64 len = length();
    if (len < 1e-12) return Vec3{};
    return Vec3{x / len, y / len, z / len};
  }
};

inline constexpr Vec3 operator*(f64 s, const Vec3& v) { return v * s; }
inline constexpr f64 dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr Vec3 cross(const Vec3& a, const Vec3& b) {
  return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// 4-component vector (homogeneous).
struct Vec4 {
  f64 x = 0.0;
  f64 y = 0.0;
  f64 z = 0.0;
  f64 w = 0.0;

  constexpr Vec4() = default;
  constexpr Vec4(f64 x_, f64 y_, f64 z_, f64 w_) : x(x_), y(y_), z(z_), w(w_) {}
  explicit constexpr Vec4(const Vec3& v, f64 w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

  constexpr Vec4 operator+(const Vec4& o) const { return Vec4{x + o.x, y + o.y, z + o.z, w + o.w}; }
  constexpr Vec4 operator-(const Vec4& o) const { return Vec4{x - o.x, y - o.y, z - o.z, w - o.w}; }
  constexpr Vec4 operator*(f64 s) const { return Vec4{x * s, y * s, z * s, w * s}; }

  constexpr Vec3 xyz() const { return Vec3{x, y, z}; }
};

inline constexpr Vec4 operator*(f64 s, const Vec4& v) { return v * s; }
inline constexpr f64 dot(const Vec4& a, const Vec4& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

}  // namespace kimia

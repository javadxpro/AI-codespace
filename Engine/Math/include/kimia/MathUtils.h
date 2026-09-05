#pragma once

#include <kimia/Types.h>

#include <algorithm>

namespace kimia {

inline constexpr f64 kPi = 3.14159265358979323846;
inline constexpr f64 kHalfPi = kPi * 0.5;
inline constexpr f64 kTwoPi = kPi * 2.0;

inline constexpr f64 radians(f64 degrees) { return degrees * kPi / 180.0; }
inline constexpr f64 degrees(f64 radiansValue) { return radiansValue * 180.0 / kPi; }

inline constexpr f64 clamp(f64 value, f64 lo, f64 hi) {
  return value < lo ? lo : (value > hi ? hi : value);
}

inline constexpr f64 lerp(f64 a, f64 b, f64 t) { return a + (b - a) * t; }

inline constexpr bool approxEqual(f64 a, f64 b, f64 epsilon = 1e-9) {
  const f64 diff = a - b;
  return diff >= -epsilon && diff <= epsilon;
}

}  // namespace kimia

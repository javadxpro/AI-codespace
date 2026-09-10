#pragma once

#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <cmath>

namespace kimia {

// --- Display colour pipeline (the foundation of the modern-graphics track) ---
//
// Shading happens in LINEAR light. The last step turns that into what a
// screen stores: a filmic tone map (highlights roll off gracefully instead
// of clipping to white) followed by the sRGB transfer function. The GL
// shader and the software rasteriser run the SAME math, so a frame looks
// identical on every backend — hardware GPU, software CPU, headless server.

// The exact sRGB transfer function (the real curve, not the gamma-2.2
// shortcut). Input and output are clamped to 0..1, so the endpoints are
// exact (encode(1.0) == 1.0) rather than one ULP off from float noise.
inline f64 srgbEncode(f64 linear) {
  const f64 v = linear < 0.0 ? 0.0 : (linear > 1.0 ? 1.0 : linear);
  if (v <= 0.0) return 0.0;
  if (v >= 1.0) return 1.0;
  const f64 out = v <= 0.0031308 ? 12.92 * v : 1.055 * std::pow(v, 1.0 / 2.4) - 0.055;
  return out < 0.0 ? 0.0 : (out > 1.0 ? 1.0 : out);
}

inline f64 srgbDecode(f64 encoded) {
  const f64 v = encoded < 0.0 ? 0.0 : (encoded > 1.0 ? 1.0 : encoded);
  if (v <= 0.0) return 0.0;
  if (v >= 1.0) return 1.0;
  const f64 out = v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
  return out < 0.0 ? 0.0 : (out > 1.0 ? 1.0 : out);
}

// Narkowicz's ACES-filmic fit — a cheap, stable approximation of the
// Academy Colour Encoding System tone mapper. Linear in, linear out;
// mid-tones are preserved, highlights roll off toward white.
inline f64 acesToneMap(f64 x) {
  const f64 a = 2.51;
  const f64 b = 0.03;
  const f64 c = 2.43;
  const f64 d = 0.59;
  const f64 e = 0.14;
  const f64 v = x < 0.0 ? 0.0 : x;
  const f64 mapped = (v * (a * v + b)) / (v * (c * v + d) + e);
  return mapped < 0.0 ? 0.0 : (mapped > 1.0 ? 1.0 : mapped);
}

inline Vec3 acesToneMap(const Vec3& color) {
  return Vec3{acesToneMap(color.x), acesToneMap(color.y), acesToneMap(color.z)};
}

inline Vec3 srgbEncode(const Vec3& linear) {
  return Vec3{srgbEncode(linear.x), srgbEncode(linear.y), srgbEncode(linear.z)};
}

inline Vec3 srgbDecode(const Vec3& encoded) {
  return Vec3{srgbDecode(encoded.x), srgbDecode(encoded.y), srgbDecode(encoded.z)};
}

// Linear (possibly HDR) colour -> the value a pixel should store:
// filmic tone map, then the sRGB transfer function.
inline Vec3 displayEncode(const Vec3& linear) { return srgbEncode(acesToneMap(linear)); }

}  // namespace kimia

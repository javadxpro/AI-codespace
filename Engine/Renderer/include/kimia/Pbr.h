#pragma once

#include <kimia/MathUtils.h>
#include <kimia/Vec.h>

#include <algorithm>
#include <cmath>

namespace kimia {

// Physically-based shading (Cook-Torrance micro-facet model). This header is
// the single source of truth for the software rasteriser; the GL fragment
// shader mirrors the same formulas line for line, so every backend renders
// an identical frame. The photon raytracer implements the same BRDF.
//
// Light colour convention: a light's `color` is the LINEAR irradiance a
// perfectly white, Lambertian surface facing the light receives — colour 1.0
// means "full light", exactly like the engine's previous Lambert model, so
// existing scenes and authoring habits keep working. Internally the light's
// radiance is color * PI, which cancels the 1/PI carried by the diffuse
// BRDF; the specular lobe gets the same PI so metal and dielectric stay in
// balance.

// Perceptually-linear roughness to GGX alpha, clamped so a mirror-smooth
// surface cannot produce a NaN hotspot.
inline f64 roughnessAlpha(f64 roughness) { return std::max(roughness * roughness, 0.02); }

// Fresnel reflectance at normal incidence: 4% for a dielectric, the tinted
// albedo for a full metal, blended by `metallic` in 0..1.
inline Vec3 fresnelF0(const Vec3& albedo, f64 metallic) {
  return Vec3{0.04 + (albedo.x - 0.04) * metallic, 0.04 + (albedo.y - 0.04) * metallic,
              0.04 + (albedo.z - 0.04) * metallic};
}

// Schlick's Fresnel approximation.
inline Vec3 fresnelSchlick(f64 vDotH, const Vec3& f0) {
  const f64 c = std::max(1.0 - vDotH, 0.0);
  const f64 k = c * c * c * c * c;
  return Vec3{f0.x + (1.0 - f0.x) * k, f0.y + (1.0 - f0.y) * k, f0.z + (1.0 - f0.z) * k};
}

// GGX (Trowbridge-Reitz) normal distribution.
inline f64 ggxDistribution(f64 nDotH, f64 alpha) {
  const f64 a2 = alpha * alpha;
  const f64 denom = nDotH * nDotH * (a2 - 1.0) + 1.0;
  return a2 / (kPi * denom * denom);
}

// One Smith masking/shadowing factor.
inline f64 smithG1(f64 nDotV, f64 alpha) {
  const f64 a2 = alpha * alpha;
  const f64 denom = nDotV + std::sqrt(a2 + (1.0 - a2) * nDotV * nDotV);
  return (2.0 * nDotV) / denom;
}

// The LINEAR outgoing radiance of a Cook-Torrance surface for one light
// (HDR — the caller tone-maps). `normal`, `viewDir` (surface -> camera) and
// `lightDir` (surface -> light) are unit vectors.
inline Vec3 cookTorrance(const Vec3& albedo, f64 roughness, f64 metallic, const Vec3& normal,
                         const Vec3& viewDir, const Vec3& lightDir, const Vec3& lightColor) {
  const f64 nDotL = std::max(dot(normal, lightDir), 0.0);
  if (nDotL <= 0.0) return Vec3{0.0, 0.0, 0.0};
  const f64 nDotV = std::max(dot(normal, viewDir), 0.0);
  const Vec3 halfVec = (viewDir + lightDir).normalized();
  const f64 nDotH = std::max(dot(normal, halfVec), 0.0);
  const f64 vDotH = std::max(dot(viewDir, halfVec), 0.0);
  const f64 alpha = roughnessAlpha(roughness);

  const Vec3 f0 = fresnelF0(albedo, metallic);
  const Vec3 f = fresnelSchlick(vDotH, f0);
  const Vec3 kd{(1.0 - f.x) * (1.0 - metallic), (1.0 - f.y) * (1.0 - metallic),
                (1.0 - f.z) * (1.0 - metallic)};

  const f64 d = ggxDistribution(nDotH, alpha);
  const f64 g = smithG1(nDotL, alpha) * smithG1(nDotV, alpha);
  const f64 specScale = d * g / std::max(4.0 * nDotL * nDotV, 1e-4);
  const Vec3 specular{f.x * specScale, f.y * specScale, f.z * specScale};

  // radiance = color * PI cancels the diffuse 1/PI (see the convention note).
  const Vec3 radiance{lightColor.x * kPi, lightColor.y * kPi, lightColor.z * kPi};
  return Vec3{(kd.x * albedo.x * (1.0 / kPi) + specular.x) * radiance.x * nDotL,
              (kd.y * albedo.y * (1.0 / kPi) + specular.y) * radiance.y * nDotL,
              (kd.z * albedo.z * (1.0 / kPi) + specular.z) * radiance.z * nDotL};
}

}  // namespace kimia

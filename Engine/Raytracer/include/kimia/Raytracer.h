#pragma once

#include <kimia/Image.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <string>
#include <vector>

namespace kimia {

struct WorldData;

namespace raytrace {

// Offline path tracer (the Cycles-style render preview): PBR direct light
// (Cook-Torrance GGX) + cosine-sampled indirect bounces + sky environment,
// soft sun shadows via cone sampling, BVH-accelerated triangle traversal.
// Pure CPU, fully deterministic (fixed per-pixel RNG seeds): the same scene
// and settings produce byte-identical images on any platform.
struct RaytraceSettings {
  i32 width = 480;
  i32 height = 360;
  i32 samplesPerPixel = 32;
  i32 maxBounces = 3;
  Vec3 sunDirection{0.4, 0.8, 0.4};  // the direction the light TRAVELS; normalized on use
  f64 sunIntensity = 2.2;
  f64 sunAngularRadius = 0.04;  // soft shadow cone (radians)
  f64 skyIntensity = 0.6;
  f64 exposure = 1.0;
};

struct RaytraceScene {
  std::vector<Vec3> vertices;  // 3 per triangle
  std::vector<Vec3> albedo;    // per triangle
  std::vector<f64> roughness;  // per triangle
  std::vector<f64> metallic;   // per triangle

  usize triangleCount() const { return albedo.size(); }
  void addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& color, f64 rough, f64 metal);
};

// Builds a ray-trace scene from a world: every entity becomes its primitive
// mesh (or its placed OBJ/FBX model), transformed to world space. The ball
// is included at its rest position when the scene has no Ball entity.
bool buildFromWorld(const WorldData& world, RaytraceScene& out, std::string& error);

// Weather and the clock (stage 24): fills in the sun direction, its
// strength and the sky brightness from the world's profile, so a render of
// a night match really is lit like one and a rainy day is flat and grey.
// The rest of `settings` (size, samples, exposure) is left untouched.
void applyWorldSky(const WorldData& world, RaytraceSettings& settings);

struct RaytraceCamera {
  Vec3 eye{0.0, 3.6, 6.2};
  Vec3 target{0.0, 0.2, 0.0};
  Vec3 up{0.0, 1.0, 0.0};
  f64 fovYDegrees = 60.0;
};

// Renders into `out` (RGB8). `bruteForce` skips the BVH (test hook: results
// are bit-identical either way).
bool render(const RaytraceScene& scene, const RaytraceSettings& settings, const RaytraceCamera& camera, Image& out);
bool render(const RaytraceScene& scene, const RaytraceSettings& settings, const RaytraceCamera& camera,
            bool bruteForce, Image& out);

}  // namespace raytrace
}  // namespace kimia

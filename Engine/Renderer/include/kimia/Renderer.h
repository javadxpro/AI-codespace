#pragma once

#include <kimia/GpuMesh.h>
#include <kimia/GraphicsTypes.h>
#include <kimia/Image.h>
#include <kimia/Mat4.h>
#include <kimia/Shader.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <map>
#include <string>
#include <vector>

namespace kimia {

// One drawable object: a mesh plus its model matrix and material.
struct RenderObject {
  const MeshData* mesh = nullptr;
  Mat4 model;
  Vec3 color{1.0, 1.0, 1.0};
  f64 roughness = 0.5;
  // Cook-Torrance metalness, 0..1: 0 = dielectric (4% Fresnel), 1 = full
  // metal (no diffuse, Fresnel tinted by `color`). Defaults to 0 so every
  // scene built before PBR keeps its Lambert look.
  f64 metallic = 0.0;
  // Optional texture (stage 34). When set, the mesh's UVs choose a pixel
  // from this image and `color` tints it. Null means a plain colour, which
  // is what everything drawn before this existed still gets.
  //
  // Not owned: the caller keeps the image alive for the frame.
  const Image* texture = nullptr;
  // Self-illumination, LINEAR and HDR: added to the lit colour before the
  // tone mapper, independent of every light. A bright emissive (say 3.0)
  // reads as a light itself after tone mapping.
  Vec3 emissive{0.0, 0.0, 0.0};
  // Opacity: 1.0 is opaque; anything below blends over the frame (drawn
  // after opaque geometry, back-to-front, without writing depth).
  f64 alpha = 1.0;
};

// A local point light: a finite lamp in the world. `color` is LINEAR light
// colour (it may exceed 1.0 — the filmic tone mapper rolls the excess off)
// and `radius` is the distance at which the light has fully fallen off.
struct PointLight {
  Vec3 position{0.0, 0.0, 0.0};
  Vec3 color{1.0, 1.0, 1.0};
  f64 radius = 5.0;
};

// The renderer's fixed point-light budget. The GL shader declares the same
// number of array uniforms and the software rasteriser caps to it too, so a
// scene can hand over as many lights as it likes without breaking a frame.
inline constexpr usize kMaxPointLights = 8;

// Everything the renderer needs to draw a frame.
struct RenderScene {
  std::vector<RenderObject> objects;
  Mat4 view;
  Mat4 projection;
  Vec3 lightDirection{-0.4, -0.8, -0.4};  // directional key light (normalized on use)
  // Key-light colour: LINEAR irradiance at a facing surface (1.0 = full
  // light). See the convention note in Pbr.h.
  Vec3 lightColor{1.0, 1.0, 1.0};
  Vec3 cameraPosition{0.0, 0.0, 0.0};
  f64 ambient = 0.25;
  // Local lights on top of the key light, added additively (no shadows in
  // this pass). Empty for every scene built before they existed.
  std::vector<PointLight> pointLights;
  // Distance fog (exponential-squared), linear-space: final = lerp(fogColor,
  // colour, exp(-(density * dist)^2)). density 0 disables it, which is the
  // default so every scene built before fog keeps its look.
  Vec3 fogColor{0.55, 0.6, 0.68};
  f64 fogDensity = 0.0;
};

// OpenGL 3.3 renderer: Cook-Torrance PBR + filmic tone mapping + key-light
// shadow map pass, PNG capture. Matches the software rasteriser pixel for
// pixel (same BRDF and display pipeline).
// Requires a current GL context (EGL pbuffer or window); initialize() reports
// failure otherwise and every call becomes a no-op.
class Renderer {
public:
  Renderer() = default;
  ~Renderer();
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  bool initialize(std::string& error);
  void shutdown();
  bool ready() const { return ready_; }

  void setShadowEnabled(bool enabled) { shadowEnabled_ = enabled; }
  bool shadowEnabled() const { return shadowEnabled_; }

  void render(const RenderScene& scene, i32 width, i32 height);
  // Reads the framebuffer back as a top-down RGBA image (so a HUD can be
  // drawn on it before encoding); false when the renderer is not ready.
  bool captureImage(i32 width, i32 height, Image& outImage) const;
  bool capturePNG(i32 width, i32 height, std::vector<u8>& outPng) const;

  static Mat4 shadowViewProjection(const RenderScene& scene);

private:
  struct TextureGpu {
    GLuint texture = 0;
    u64 signature = 0U;
  };

  const GpuMesh& meshFor(const MeshData* mesh);
  GLuint textureFor(const Image* image);

  Shader phong_;
  Shader depth_;
  std::map<const MeshData*, GpuMesh> gpuMeshes_;
  std::map<const Image*, TextureGpu> gpuTextures_;
  GLuint shadowFbo_ = 0;
  GLuint shadowTexture_ = 0;
  bool ready_ = false;
  bool shadowEnabled_ = true;
};

// CPU rasterizer: flat-shaded, z-buffered, perspective-correct. Runs anywhere
// (no GL needed) — this is the guaranteed display path on devices without a
// GPU driver (Termux software rendering).
bool renderSoftware(const RenderScene& scene, i32 width, i32 height, const Vec3& clearColor, Image& out);

}  // namespace kimia

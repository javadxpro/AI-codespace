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
  // Optional texture (stage 34). When set, the mesh's UVs choose a pixel
  // from this image and `color` tints it. Null means a plain colour, which
  // is what everything drawn before this existed still gets.
  //
  // Not owned: the caller keeps the image alive for the frame.
  const Image* texture = nullptr;
};

// Everything the renderer needs to draw a frame.
struct RenderScene {
  std::vector<RenderObject> objects;
  Mat4 view;
  Mat4 projection;
  Vec3 lightDirection{-0.4, -0.8, -0.4};  // directional key light (normalized on use)
  Vec3 cameraPosition{0.0, 0.0, 0.0};
  f64 ambient = 0.25;
};

// OpenGL 3.3 renderer: Phong + gamma + key-light shadow map pass, PNG capture.
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
  const GpuMesh& meshFor(const MeshData* mesh);

  Shader phong_;
  Shader depth_;
  std::map<const MeshData*, GpuMesh> gpuMeshes_;
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

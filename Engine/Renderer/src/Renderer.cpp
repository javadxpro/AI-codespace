#include <kimia/Renderer.h>

#include <kimia/Image.h>
#include <kimia/Shaders.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

namespace kimia {

namespace {

constexpr i32 kShadowSize = 1024;

struct Bounds {
  Vec3 lo{1e30, 1e30, 1e30};
  Vec3 hi{-1e30, -1e30, -1e30};
};

Bounds sceneBounds(const RenderScene& scene) {
  Bounds bounds;
  for (const RenderObject& object : scene.objects) {
    if (object.mesh == nullptr) continue;
    for (const Vec3& vertex : object.mesh->positions) {
      const Vec3 world = object.model * vertex;
      bounds.lo.x = std::min(bounds.lo.x, world.x);
      bounds.lo.y = std::min(bounds.lo.y, world.y);
      bounds.lo.z = std::min(bounds.lo.z, world.z);
      bounds.hi.x = std::max(bounds.hi.x, world.x);
      bounds.hi.y = std::max(bounds.hi.y, world.y);
      bounds.hi.z = std::max(bounds.hi.z, world.z);
    }
  }
  return bounds;
}

u64 textureSignature(const Image& image) {
  // Workbench images are mutable. Hash their content so an edited image is
  // uploaded again even when the caller keeps the same Image object.
  u64 hash = 1469598103934665603ULL;
  const auto mix = [&hash](u8 value) {
    hash ^= static_cast<u64>(value);
    hash *= 1099511628211ULL;
  };
  mix(static_cast<u8>(image.width & 0xff));
  mix(static_cast<u8>((image.width >> 8) & 0xff));
  mix(static_cast<u8>(image.height & 0xff));
  mix(static_cast<u8>((image.height >> 8) & 0xff));
  mix(static_cast<u8>(image.channels));
  for (const u8 value : image.pixels) mix(value);
  return hash;
}

}  // namespace

Renderer::~Renderer() { shutdown(); }

Mat4 Renderer::shadowViewProjection(const RenderScene& scene) {
  const Bounds bounds = sceneBounds(scene);
  const Vec3 center = (bounds.lo + bounds.hi) * 0.5;
  const Vec3 extent = bounds.hi - bounds.lo;
  const f64 radius = extent.length() * 0.5 + 0.1;
  const Vec3 light = scene.lightDirection.normalized();
  const Vec3 up = std::abs(light.y) < 0.9 ? Vec3{0.0, 1.0, 0.0} : Vec3{1.0, 0.0, 0.0};
  const Mat4 lightView = Mat4::lookAt(center - light * radius, center, up);
  f64 minX = 1e30, maxX = -1e30, minY = 1e30, maxY = -1e30, minZ = 1e30, maxZ = -1e30;
  for (i32 i = 0; i < 8; ++i) {
    const Vec3 corner{bounds.lo.x + (i & 1 ? extent.x : 0.0), bounds.lo.y + (i & 2 ? extent.y : 0.0),
                      bounds.lo.z + (i & 4 ? extent.z : 0.0)};
    const Vec3 view = lightView * corner;
    minX = std::min(minX, view.x);
    maxX = std::max(maxX, view.x);
    minY = std::min(minY, view.y);
    maxY = std::max(maxY, view.y);
    minZ = std::min(minZ, view.z);
    maxZ = std::max(maxZ, view.z);
  }
  const f64 margin = radius * 0.1 + 0.01;
  const f64 nearPlane = std::max(0.01, -maxZ - margin);
  const f64 farPlane = -minZ + margin;
  return Mat4::orthographic(minX - margin, maxX + margin, minY - margin, maxY + margin, nearPlane, farPlane) *
         lightView;
}

bool Renderer::initialize(std::string& error) {
  shutdown();
  GLFunctions& gl = GLFunctions::instance();
  if (!gl.loaded()) {
    error = "no OpenGL available (GLFunctions not loaded)";
    return false;
  }
  if (!phong_.compile(shaders::phongVertex, shaders::phongFragment, error)) return false;
  if (!depth_.compile(shaders::depthVertex, shaders::depthFragment, error)) return false;

  gl.genTextures(1, &shadowTexture_);
  gl.bindTexture(GL_TEXTURE_2D, shadowTexture_);
  gl.texImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, kShadowSize, kShadowSize, 0, GL_DEPTH_COMPONENT,
                GL_UNSIGNED_INT, nullptr);
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  gl.bindTexture(GL_TEXTURE_2D, 0);

  gl.genFramebuffers(1, &shadowFbo_);
  gl.bindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
  gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture_, 0);
  const GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
  gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    error = "shadow framebuffer incomplete (status " + std::to_string(status) + ")";
    shutdown();
    return false;
  }
  ready_ = true;
  return true;
}

void Renderer::shutdown() {
  GLFunctions& gl = GLFunctions::instance();
  for (const auto& entry : gpuTextures_) {
    if (entry.second.texture != 0U) gl.deleteTextures(1, &entry.second.texture);
  }
  gpuTextures_.clear();
  gpuMeshes_.clear();
  phong_.destroy();
  depth_.destroy();
  if (shadowFbo_ != 0) {
    gl.deleteFramebuffers(1, &shadowFbo_);
    shadowFbo_ = 0;
  }
  if (shadowTexture_ != 0) {
    gl.deleteTextures(1, &shadowTexture_);
    shadowTexture_ = 0;
  }
  ready_ = false;
}

void Renderer::render(const RenderScene& scene, i32 width, i32 height) {
  if (!ready_ || width <= 0 || height <= 0) return;
  GLFunctions& gl = GLFunctions::instance();
  const Mat4 viewProjection = scene.projection * scene.view;
  const Mat4 lightViewProjection = shadowViewProjection(scene);
  const Vec3 lightDirection = scene.lightDirection.normalized();

  if (shadowEnabled_) {
    gl.bindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
    gl.viewport(0, 0, kShadowSize, kShadowSize);
    gl.clear(GL_DEPTH_BUFFER_BIT);
    depth_.use();
    for (const RenderObject& object : scene.objects) {
      if (object.mesh == nullptr) continue;
      depth_.setMat4("uModel", object.model);
      depth_.setMat4("uLightViewProj", lightViewProjection);
      meshFor(object.mesh).draw();
    }
    gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  gl.viewport(0, 0, width, height);
  gl.clearColor(0.05f, 0.05f, 0.06f, 1.0f);
  gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl.enable(GL_DEPTH_TEST);
  gl.depthFunc(GL_LEQUAL);
  gl.enable(GL_CULL_FACE);
  gl.cullFace(GL_BACK);
  gl.frontFace(GL_CCW);

  phong_.use();
  phong_.setMat4("uViewProj", viewProjection);
  phong_.setMat4("uLightViewProj", lightViewProjection);
  phong_.setVec3("uLightDir", lightDirection);
  phong_.setVec3("uLightColor", scene.lightColor);
  phong_.setVec3("uAmbient", Vec3{scene.ambient, scene.ambient, scene.ambient});
  phong_.setVec3("uCameraPos", scene.cameraPosition);
  phong_.setVec3("uFogColor", scene.fogColor);
  phong_.setFloat("uFogDensity", scene.fogDensity);
  gl.activeTexture(GL_TEXTURE0 + 1);
  gl.bindTexture(GL_TEXTURE_2D, shadowTexture_);
  phong_.setInt("uShadowMap", 1);
  phong_.setInt("uBaseTexture", 0);
  const usize lightCount =
      scene.pointLights.size() < kMaxPointLights ? scene.pointLights.size() : kMaxPointLights;
  phong_.setInt("uPointLightCount", static_cast<i32>(lightCount));
  for (usize i = 0; i < lightCount; ++i) {
    const PointLight& lamp = scene.pointLights[i];
    char name[40];
    std::snprintf(name, sizeof(name), "uPointLightPos[%zu]", i);
    phong_.setVec3(name, lamp.position);
    std::snprintf(name, sizeof(name), "uPointLightColor[%zu]", i);
    phong_.setVec3(name, lamp.color);
    std::snprintf(name, sizeof(name), "uPointLightRadius[%zu]", i);
    phong_.setFloat(name, lamp.radius);
  }
  auto drawObject = [&](const RenderObject& object) {
    phong_.setMat4("uModel", object.model);
    phong_.setMat4("uNormalMat", object.model.inverseTranspose());
    phong_.setVec3("uColor", object.color);
    phong_.setFloat("uRoughness", object.roughness);
    phong_.setFloat("uMetallic", object.metallic);
    phong_.setVec3("uEmissive", object.emissive);
    phong_.setFloat("uAlpha", object.alpha);
    const GLuint texture = textureFor(object.texture);
    gl.activeTexture(GL_TEXTURE0);
    gl.bindTexture(GL_TEXTURE_2D, texture);
    phong_.setFloat("uHasTexture", texture != 0U ? 1.0 : 0.0);
    meshFor(object.mesh).draw();
  };

  // Opaque first (they own the depth buffer), then translucent surfaces
  // far-to-near with alpha blending and no depth write, exactly like the
  // software rasteriser.
  for (const RenderObject& object : scene.objects) {
    if (object.mesh == nullptr || object.alpha < 1.0) continue;
    drawObject(object);
  }
  std::vector<const RenderObject*> translucent;
  for (const RenderObject& object : scene.objects) {
    if (object.mesh != nullptr && object.alpha < 1.0) translucent.push_back(&object);
  }
  if (!translucent.empty()) {
    std::stable_sort(translucent.begin(), translucent.end(),
                     [&](const RenderObject* a, const RenderObject* b) {
                       const Vec3 pa = a->model * Vec3{0.0, 0.0, 0.0};
                       const Vec3 pb = b->model * Vec3{0.0, 0.0, 0.0};
                       return (pa - scene.cameraPosition).lengthSquared() >
                              (pb - scene.cameraPosition).lengthSquared();
                     });
    gl.enable(GL_BLEND);
    gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.depthMask(false);
    for (const RenderObject* object : translucent) drawObject(*object);
    gl.depthMask(true);
    gl.disable(GL_BLEND);
  }

  gl.activeTexture(GL_TEXTURE0);
  gl.bindTexture(GL_TEXTURE_2D, 0);
  gl.activeTexture(GL_TEXTURE0 + 1);
  gl.bindTexture(GL_TEXTURE_2D, 0);
  gl.disable(GL_CULL_FACE);
  gl.disable(GL_DEPTH_TEST);
}

const GpuMesh& Renderer::meshFor(const MeshData* mesh) {
  auto found = gpuMeshes_.find(mesh);
  if (found != gpuMeshes_.end()) return found->second;
  auto inserted = gpuMeshes_.emplace(mesh, GpuMesh{});
  inserted.first->second.upload(*mesh);
  return inserted.first->second;
}

GLuint Renderer::textureFor(const Image* image) {
  if (image == nullptr || image->isEmpty() || image->width <= 0 || image->height <= 0 ||
      (image->channels != 3 && image->channels != 4)) {
    return 0U;
  }
  const usize pixelCount = static_cast<usize>(image->width) * static_cast<usize>(image->height);
  const usize required = pixelCount * static_cast<usize>(image->channels);
  if (image->pixels.size() < required) return 0U;

  GLFunctions& gl = GLFunctions::instance();
  const u64 signature = textureSignature(*image);
  auto found = gpuTextures_.find(image);
  if (found != gpuTextures_.end() && found->second.signature == signature) return found->second.texture;
  if (found != gpuTextures_.end() && found->second.texture != 0U) {
    gl.deleteTextures(1, &found->second.texture);
    found->second.texture = 0U;
  }

  std::vector<u8> rgba(pixelCount * 4U);
  for (usize i = 0; i < pixelCount; ++i) {
    const usize source = i * static_cast<usize>(image->channels);
    const usize destination = i * 4U;
    rgba[destination] = image->pixels[source];
    rgba[destination + 1U] = image->pixels[source + 1U];
    rgba[destination + 2U] = image->pixels[source + 2U];
    rgba[destination + 3U] = image->channels == 4 ? image->pixels[source + 3U] : 255U;
  }

  GLuint texture = 0U;
  gl.genTextures(1, &texture);
  if (texture == 0U) return 0U;
  gl.activeTexture(GL_TEXTURE0);
  gl.bindTexture(GL_TEXTURE_2D, texture);
  gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
  gl.texImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA), static_cast<GLsizei>(image->width),
                static_cast<GLsizei>(image->height), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR_MIPMAP_LINEAR));
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_REPEAT));
  gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_REPEAT));
  gl.generateMipmap(GL_TEXTURE_2D);
  gl.bindTexture(GL_TEXTURE_2D, 0U);

  TextureGpu& stored = gpuTextures_[image];
  stored.texture = texture;
  stored.signature = signature;
  return texture;
}

bool Renderer::captureImage(i32 width, i32 height, Image& outImage) const {
  if (!ready_ || width <= 0 || height <= 0) return false;
  GLFunctions& gl = GLFunctions::instance();
  std::vector<u8> rgba(static_cast<usize>(width) * static_cast<usize>(height) * 4U);
  gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
  gl.readPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  outImage.width = width;
  outImage.height = height;
  outImage.channels = 4;
  outImage.pixels.resize(rgba.size());
  const usize rowBytes = static_cast<usize>(width) * 4U;
  for (i32 y = 0; y < height; ++y) {
    const usize src = static_cast<usize>(height - 1 - y) * rowBytes;
    const usize dst = static_cast<usize>(y) * rowBytes;
    for (usize i = 0; i < rowBytes; ++i) outImage.pixels[dst + i] = rgba[src + i];
  }
  return true;
}

bool Renderer::capturePNG(i32 width, i32 height, std::vector<u8>& outPng) const {
  Image image;
  if (!captureImage(width, height, image)) return false;
  outPng = image.encodePNG();
  return !outPng.empty();
}

}  // namespace kimia

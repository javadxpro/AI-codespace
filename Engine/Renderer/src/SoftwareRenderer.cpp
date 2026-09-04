#include <kimia/Renderer.h>

#include <cmath>
#include <vector>

namespace kimia {

namespace {

f64 clampUnit(f64 value) { return value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value); }

u8 toByte(f64 value) { return static_cast<u8>(std::lround(clampUnit(value) * 255.0)); }

f64 gammaEncode(f64 linear) { return std::pow(clampUnit(linear), 1.0 / 2.2); }

// Signed edge function in screen space.
f64 edge(f64 ax, f64 ay, f64 bx, f64 by, f64 px, f64 py) { return (px - ax) * (by - ay) - (py - ay) * (bx - ax); }

struct Vertex {
  f64 x = 0.0;   // screen x (pixels)
  f64 y = 0.0;   // screen y (pixels)
  f64 depth = 0.0;
  f64 invW = 1.0;
};

}  // namespace

bool renderSoftware(const RenderScene& scene, i32 width, i32 height, const Vec3& clearColor, Image& out) {
  if (width <= 0 || height <= 0 || width > 8192 || height > 8192) return false;
  const Mat4 viewProjection = scene.projection * scene.view;

  out.width = width;
  out.height = height;
  out.channels = 3;
  out.pixels.assign(static_cast<usize>(width) * static_cast<usize>(height) * 3U, 0);
  std::vector<f64> zBuffer(static_cast<usize>(width) * static_cast<usize>(height), 1.0);

  // Background (gamma-encoded clear color).
  const u8 clearR = toByte(gammaEncode(clearColor.x));
  const u8 clearG = toByte(gammaEncode(clearColor.y));
  const u8 clearB = toByte(gammaEncode(clearColor.z));
  for (i32 y = 0; y < height; ++y) {
    for (i32 x = 0; x < width; ++x) {
      u8* pixel = out.at(x, y);
      pixel[0] = clearR;
      pixel[1] = clearG;
      pixel[2] = clearB;
    }
  }

  const Vec3 light = scene.lightDirection.normalized();

  for (const RenderObject& object : scene.objects) {
    if (object.mesh == nullptr || !object.mesh->isValid()) continue;
    const MeshData& mesh = *object.mesh;
    for (usize t = 0; t + 2U < mesh.indices.size(); t += 3U) {
      const u32 i0 = mesh.indices[t];
      const u32 i1 = mesh.indices[t + 1U];
      const u32 i2 = mesh.indices[t + 2U];
      const Vec3 w0 = object.model * mesh.positions[i0];
      const Vec3 w1 = object.model * mesh.positions[i1];
      const Vec3 w2 = object.model * mesh.positions[i2];

      // Flat shading with the world-space face normal.
      Vec3 normal = kimia::cross(w1 - w0, w2 - w0);
      if (normal.lengthSquared() < 1e-24) continue;
      normal = normal.normalized();
      if (kimia::dot(normal, w0 - scene.cameraPosition) > 0.0) continue;  // backface

      const Vec4 c0 = viewProjection * Vec4{w0.x, w0.y, w0.z, 1.0};
      const Vec4 c1 = viewProjection * Vec4{w1.x, w1.y, w1.z, 1.0};
      const Vec4 c2 = viewProjection * Vec4{w2.x, w2.y, w2.z, 1.0};
      if (c0.w <= 1e-6 || c1.w <= 1e-6 || c2.w <= 1e-6) continue;  // crosses the near plane

      const Vertex v0{(c0.x / c0.w * 0.5 + 0.5) * static_cast<f64>(width),
                      (1.0 - (c0.y / c0.w * 0.5 + 0.5)) * static_cast<f64>(height), c0.z / c0.w * 0.5 + 0.5, 1.0 / c0.w};
      const Vertex v1{(c1.x / c1.w * 0.5 + 0.5) * static_cast<f64>(width),
                      (1.0 - (c1.y / c1.w * 0.5 + 0.5)) * static_cast<f64>(height), c1.z / c1.w * 0.5 + 0.5, 1.0 / c1.w};
      const Vertex v2{(c2.x / c2.w * 0.5 + 0.5) * static_cast<f64>(width),
                      (1.0 - (c2.y / c2.w * 0.5 + 0.5)) * static_cast<f64>(height), c2.z / c2.w * 0.5 + 0.5, 1.0 / c2.w};

      const f64 area = edge(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
      if (std::abs(area) < 1e-12) continue;
      const f64 invArea = 1.0 / area;

      const f64 minXf = std::min(std::min(v0.x, v1.x), v2.x);
      const f64 maxXf = std::max(std::max(v0.x, v1.x), v2.x);
      const f64 minYf = std::min(std::min(v0.y, v1.y), v2.y);
      const f64 maxYf = std::max(std::max(v0.y, v1.y), v2.y);
      const i32 minX = std::max(0, static_cast<i32>(std::floor(minXf)));
      const i32 maxX = std::min(width - 1, static_cast<i32>(std::ceil(maxXf)));
      const i32 minY = std::max(0, static_cast<i32>(std::floor(minYf)));
      const i32 maxY = std::min(height - 1, static_cast<i32>(std::ceil(maxYf)));

      // Shading (Lambert + ambient, gamma-encoded once per triangle).
      const f64 diffuse = std::max(0.0, kimia::dot(normal, -light));
      const f64 shade = scene.ambient + diffuse * (1.0 - scene.ambient);
      const u8 r = toByte(gammaEncode(object.color.x * shade));
      const u8 g = toByte(gammaEncode(object.color.y * shade));
      const u8 b = toByte(gammaEncode(object.color.z * shade));

      for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
          const f64 px = static_cast<f64>(x) + 0.5;
          const f64 py = static_cast<f64>(y) + 0.5;
          const f64 w0b = edge(v1.x, v1.y, v2.x, v2.y, px, py) * invArea;
          const f64 w1b = edge(v2.x, v2.y, v0.x, v0.y, px, py) * invArea;
          const f64 w2b = edge(v0.x, v0.y, v1.x, v1.y, px, py) * invArea;
          if (w0b < -1e-9 || w1b < -1e-9 || w2b < -1e-9) continue;
          // Perspective-correct depth interpolation.
          const f64 depth = (w0b * v0.depth * v0.invW + w1b * v1.depth * v1.invW + w2b * v2.depth * v2.invW) /
                            (w0b * v0.invW + w1b * v1.invW + w2b * v2.invW);
          const usize index = static_cast<usize>(y) * static_cast<usize>(width) + static_cast<usize>(x);
          if (depth >= zBuffer[index]) continue;
          zBuffer[index] = depth;
          u8* pixel = out.at(x, y);
          pixel[0] = r;
          pixel[1] = g;
          pixel[2] = b;
        }
      }
    }
  }
  return true;
}

}  // namespace kimia

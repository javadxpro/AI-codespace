#include <kimia/ColorPipeline.h>
#include <kimia/Pbr.h>
#include <kimia/Renderer.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace kimia {

namespace {

f64 clampUnit(f64 value) { return value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value); }

u8 toByte(f64 value) { return static_cast<u8>(std::lround(clampUnit(value) * 255.0)); }

// Signed edge function in screen space.
f64 edge(f64 ax, f64 ay, f64 bx, f64 by, f64 px, f64 py) { return (px - ax) * (by - ay) - (py - ay) * (bx - ax); }

struct Vertex {
  f64 x = 0.0;   // screen x (pixels)
  f64 y = 0.0;   // screen y (pixels)
  f64 clipZ = 0.0;
  f64 clipW = 1.0;
};

// A view-space corner plus the texture coordinate that belongs to it. The
// UV has to travel with the position through clipping, or a triangle that
// crosses the near plane gets the wrong part of the image (stage 34).
struct Corner {
  Vec4 at;
  Vec2 uv;
};

// Sutherland-Hodgman clip of a view-space triangle against z <= -near
// (the near plane, looking down -Z). Returns 0, 3 or 4 view-space corners.
std::vector<Corner> clipNear(const Corner& a, const Corner& b, const Corner& c, f64 nearValue) {
  std::vector<Corner> in = {a, b, c};
  std::vector<Corner> out;
  out.reserve(4);
  for (usize i = 0; i < in.size(); ++i) {
    const Corner cur = in[i];
    const Corner nxt = in[(i + 1U) % in.size()];
    const bool curIn = cur.at.z <= -nearValue;
    const bool nxtIn = nxt.at.z <= -nearValue;
    if (curIn) out.push_back(cur);
    if (curIn != nxtIn) {
      const f64 denominator = cur.at.z - nxt.at.z;
      f64 t = 0.5;
      if (std::abs(denominator) > 1e-12) t = (cur.at.z + nearValue) / denominator;
      Corner cut;
      cut.at = cur.at + (nxt.at - cur.at) * t;
      cut.uv = Vec2{cur.uv.x + (nxt.uv.x - cur.uv.x) * t, cur.uv.y + (nxt.uv.y - cur.uv.y) * t};
      out.push_back(cut);
    }
  }
  return out;
}

// Nearest-neighbour sample, wrapping so a UV outside 0..1 tiles rather
// than smearing the edge pixel. Nearest rather than bilinear on purpose:
// this rasteriser runs on a phone CPU, and the art is pixel-art scale.
void sampleTexture(const Image& image, f64 u, f64 v, f64& r, f64& g, f64& b) {
  r = g = b = 1.0;
  if (image.width <= 0 || image.height <= 0 || image.channels < 3) return;
  f64 wrappedU = u - std::floor(u);
  f64 wrappedV = v - std::floor(v);
  i32 x = static_cast<i32>(wrappedU * static_cast<f64>(image.width));
  i32 y = static_cast<i32>(wrappedV * static_cast<f64>(image.height));
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x >= image.width) x = image.width - 1;
  if (y >= image.height) y = image.height - 1;
  const usize index = (static_cast<usize>(y) * static_cast<usize>(image.width) + static_cast<usize>(x)) *
                      static_cast<usize>(image.channels);
  if (index + 2U >= image.pixels.size()) return;
  // The image is sRGB-encoded; shading is linear, so decode first.
  r = srgbDecode(static_cast<f64>(image.pixels[index]) / 255.0);
  g = srgbDecode(static_cast<f64>(image.pixels[index + 1U]) / 255.0);
  b = srgbDecode(static_cast<f64>(image.pixels[index + 2U]) / 255.0);
}

}  // namespace

bool renderSoftware(const RenderScene& scene, i32 width, i32 height, const Vec3& clearColor, Image& out) {
  if (width <= 0 || height <= 0 || width > 8192 || height > 8192) return false;

  out.width = width;
  out.height = height;
  out.channels = 3;
  out.pixels.assign(static_cast<usize>(width) * static_cast<usize>(height) * 3U, 0);
  std::vector<f64> zBuffer(static_cast<usize>(width) * static_cast<usize>(height), 1.0);

  // Background (sRGB-encoded clear color).
  const u8 clearR = toByte(srgbEncode(clearColor.x));
  const u8 clearG = toByte(srgbEncode(clearColor.y));
  const u8 clearB = toByte(srgbEncode(clearColor.z));
  for (i32 y = 0; y < height; ++y) {
    for (i32 x = 0; x < width; ++x) {
      u8* pixel = out.at(x, y);
      pixel[0] = clearR;
      pixel[1] = clearG;
      pixel[2] = clearB;
    }
  }

  // Near plane for clipping, recovered from a standard perspective matrix.
  f64 nearValue = 1e-3;
  bool useNearClip = false;
  if (std::abs(scene.projection.at(2, 3) + 1.0) < 1e-9) {
    const f64 denominator = scene.projection.at(2, 2) - 1.0;
    if (std::abs(denominator) > 1e-12) {
      const f64 nearPlane = scene.projection.at(3, 2) / denominator;
      if (nearPlane > 1e-4) {
        nearValue = nearPlane;
        useNearClip = true;
      }
    }
  }

  const Vec3 light = scene.lightDirection.normalized();
  const bool fogEnabled = scene.fogDensity > 0.0;

  // Rasterize one object. `blend` draws a translucent surface over the
  // pixels already there (no depth write) so glass and smoke stack.
  auto rasterizeObject = [&](const RenderObject& object, bool blend) {
    if (object.mesh == nullptr || !object.mesh->isValid()) return;
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

      // UVs travel with the corners so texturing survives near-plane
      // clipping. A mesh without UVs textures as a flat colour.
      const bool textured = object.texture != nullptr && mesh.uvs.size() == mesh.positions.size();
      const Vec2 uv0 = textured ? mesh.uvs[i0] : Vec2{0.0, 0.0};
      const Vec2 uv1 = textured ? mesh.uvs[i1] : Vec2{0.0, 0.0};
      const Vec2 uv2 = textured ? mesh.uvs[i2] : Vec2{0.0, 0.0};

      std::vector<Corner> clipped;
      const Corner view0{scene.view * Vec4{w0.x, w0.y, w0.z, 1.0}, uv0};
      const Corner view1{scene.view * Vec4{w1.x, w1.y, w1.z, 1.0}, uv1};
      const Corner view2{scene.view * Vec4{w2.x, w2.y, w2.z, 1.0}, uv2};
      if (useNearClip) {
        clipped = clipNear(view0, view1, view2, nearValue);
      } else {
        clipped = {view0, view1, view2};
      }
      if (clipped.size() < 3U) continue;  // entirely behind the camera

      // Cook-Torrance PBR. Flat shading samples the BRDF once per triangle
      // at its centroid (the flat-shaded stand-in for a per-pixel position);
      // the textured path re-runs the same lambda per pixel so the texture
      // pixel is the albedo the BRDF sees. Direct lighting is additive and
      // HDR: the tone mapper rolls the excess off. This mirrors the GL
      // shader's per-pixel version of the exact same BRDF.
      const Vec3 centroid = (w0 + w1 + w2) * (1.0 / 3.0);
      const Vec3 viewDir = (scene.cameraPosition - centroid).normalized();
      const usize lightCount =
          scene.pointLights.size() < kMaxPointLights ? scene.pointLights.size() : kMaxPointLights;
      auto lightSurface = [&](const Vec3& albedo) {
        Vec3 lit = kimia::cookTorrance(albedo, object.roughness, object.metallic, normal, viewDir,
                                       light * -1.0, scene.lightColor);
        lit.x += albedo.x * scene.ambient;
        lit.y += albedo.y * scene.ambient;
        lit.z += albedo.z * scene.ambient;
        for (usize i = 0; i < lightCount; ++i) {
          const PointLight& lamp = scene.pointLights[i];
          const Vec3 toLight = lamp.position - centroid;
          const f64 distSq = toLight.lengthSquared();
          if (distSq <= 1e-24) continue;
          const f64 radius = lamp.radius > 0.0 ? lamp.radius : 1.0;
          if (distSq > radius * radius) continue;
          const f64 dist = std::sqrt(distSq);
          const f64 falloff = 1.0 - dist / radius;
          const f64 attenuation = falloff * falloff;
          const Vec3 direction = toLight * (1.0 / dist);
          const Vec3 lampColor{lamp.color.x * attenuation, lamp.color.y * attenuation,
                               lamp.color.z * attenuation};
          const Vec3 contribution =
              kimia::cookTorrance(albedo, object.roughness, object.metallic, normal, viewDir,
                                  direction, lampColor);
          lit.x += contribution.x;
          lit.y += contribution.y;
          lit.z += contribution.z;
        }
        return lit;
      };
      const Vec3 flatRadiance = lightSurface(object.color);
      const Vec3 emissive = object.emissive;
      const Vec3 flatLinear{flatRadiance.x + emissive.x, flatRadiance.y + emissive.y,
                            flatRadiance.z + emissive.z};
      const u8 r = toByte(srgbEncode(acesToneMap(flatLinear.x)));
      const u8 g = toByte(srgbEncode(acesToneMap(flatLinear.y)));
      const u8 b = toByte(srgbEncode(acesToneMap(flatLinear.z)));

      // Fan-triangulate the clipped polygon (3 or 4 vertices).
      for (usize k = 1; k + 1U < clipped.size(); ++k) {
        const Vec4 c0 = scene.projection * clipped[0].at;
        const Vec4 c1 = scene.projection * clipped[k].at;
        const Vec4 c2 = scene.projection * clipped[k + 1U].at;
        const Vec2 t0 = clipped[0].uv;
        const Vec2 t1 = clipped[k].uv;
        const Vec2 t2 = clipped[k + 1U].uv;
        if (c0.w <= 1e-9 || c1.w <= 1e-9 || c2.w <= 1e-9) continue;

        const Vertex v0{(c0.x / c0.w * 0.5 + 0.5) * static_cast<f64>(width),
                        (1.0 - (c0.y / c0.w * 0.5 + 0.5)) * static_cast<f64>(height), c0.z, c0.w};
        const Vertex v1{(c1.x / c1.w * 0.5 + 0.5) * static_cast<f64>(width),
                        (1.0 - (c1.y / c1.w * 0.5 + 0.5)) * static_cast<f64>(height), c1.z, c1.w};
        const Vertex v2{(c2.x / c2.w * 0.5 + 0.5) * static_cast<f64>(width),
                        (1.0 - (c2.y / c2.w * 0.5 + 0.5)) * static_cast<f64>(height), c2.z, c2.w};

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

        for (i32 y = minY; y <= maxY; ++y) {
          for (i32 x = minX; x <= maxX; ++x) {
            const f64 px = static_cast<f64>(x) + 0.5;
            const f64 py = static_cast<f64>(y) + 0.5;
            const f64 w0b = edge(v1.x, v1.y, v2.x, v2.y, px, py) * invArea;
            const f64 w1b = edge(v2.x, v2.y, v0.x, v0.y, px, py) * invArea;
            const f64 w2b = edge(v0.x, v0.y, v1.x, v1.y, px, py) * invArea;
            if (w0b < -1e-9 || w1b < -1e-9 || w2b < -1e-9) continue;
            // Depth: clip z and w are affine in screen space for planar
            // surfaces, so interpolate both and divide (NDC depth = z/w).
            const f64 depth = (w0b * v0.clipZ + w1b * v1.clipZ + w2b * v2.clipZ) /
                              (w0b * v0.clipW + w1b * v1.clipW + w2b * v2.clipW) * 0.5 + 0.5;
            const usize index = static_cast<usize>(y) * static_cast<usize>(width) + static_cast<usize>(x);
            if (depth >= zBuffer[index]) continue;
            u8* pixel = out.at(x, y);

            // Fast path: an opaque, untextured triangle under no fog has
            // the same LINEAR colour everywhere, so it was tone-mapped once
            // above (r/g/b).
            if (!blend && !fogEnabled && !textured) {
              pixel[0] = r;
              pixel[1] = g;
              pixel[2] = b;
              zBuffer[index] = depth;
              continue;
            }

            // Perspective-correct interpolation denominator, shared by
            // texturing and fog. 1/clipW is linear in screen space, so
            // interpolating u/w (and world/w) then dividing is correct.
            f64 iw0 = 0.0;
            f64 iw1 = 0.0;
            f64 iw2 = 0.0;
            f64 invW = 0.0;
            if (textured || fogEnabled) {
              iw0 = 1.0 / v0.clipW;
              iw1 = 1.0 / v1.clipW;
              iw2 = 1.0 / v2.clipW;
              invW = w0b * iw0 + w1b * iw1 + w2b * iw2;
              if (std::abs(invW) < 1e-12) continue;
            }

            // LINEAR colour before tone mapping: PBR lighting + emissive,
            // then distance fog. Mirrors the GL fragment shader.
            Vec3 linear;
            if (textured) {
              const f64 u = (w0b * t0.x * iw0 + w1b * t1.x * iw1 + w2b * t2.x * iw2) / invW;
              const f64 v = (w0b * t0.y * iw0 + w1b * t1.y * iw1 + w2b * t2.y * iw2) / invW;
              f64 tr = 1.0;
              f64 tg = 1.0;
              f64 tb = 1.0;
              sampleTexture(*object.texture, u, v, tr, tg, tb);
              // The object colour tints the texture, so a white object
              // shows the image unchanged; the texel is the PBR albedo.
              const Vec3 albedo{object.color.x * tr, object.color.y * tg, object.color.z * tb};
              linear = lightSurface(albedo);
            } else {
              linear = flatRadiance;
            }
            linear.x += emissive.x;
            linear.y += emissive.y;
            linear.z += emissive.z;
            if (fogEnabled) {
              const Vec3 world{(w0b * w0.x * iw0 + w1b * w1.x * iw1 + w2b * w2.x * iw2) / invW,
                               (w0b * w0.y * iw0 + w1b * w1.y * iw1 + w2b * w2.y * iw2) / invW,
                               (w0b * w0.z * iw0 + w1b * w1.z * iw1 + w2b * w2.z * iw2) / invW};
              const f64 fogK = scene.fogDensity * (world - scene.cameraPosition).length();
              const f64 fogFactor = std::exp(-fogK * fogK);
              linear.x = scene.fogColor.x + (linear.x - scene.fogColor.x) * fogFactor;
              linear.y = scene.fogColor.y + (linear.y - scene.fogColor.y) * fogFactor;
              linear.z = scene.fogColor.z + (linear.z - scene.fogColor.z) * fogFactor;
            }
            const u8 sr = toByte(srgbEncode(acesToneMap(linear.x)));
            const u8 sg = toByte(srgbEncode(acesToneMap(linear.y)));
            const u8 sb = toByte(srgbEncode(acesToneMap(linear.z)));
            if (!blend) {
              pixel[0] = sr;
              pixel[1] = sg;
              pixel[2] = sb;
              zBuffer[index] = depth;
              continue;
            }
            // Translucent: "over" blend, no depth write so later glass
            // stacks on top.
            const f64 a = object.alpha;
            pixel[0] = static_cast<u8>(std::lround(static_cast<f64>(sr) * a + static_cast<f64>(pixel[0]) * (1.0 - a)));
            pixel[1] = static_cast<u8>(std::lround(static_cast<f64>(sg) * a + static_cast<f64>(pixel[1]) * (1.0 - a)));
            pixel[2] = static_cast<u8>(std::lround(static_cast<f64>(sb) * a + static_cast<f64>(pixel[2]) * (1.0 - a)));
          }
        }
      }
    }
  };

  // Opaque pass first (those own the depth buffer), then translucent
  // surfaces far-to-near so nearer glass blends over farther glass.
  for (const RenderObject& object : scene.objects) {
    if (object.alpha < 1.0) continue;
    rasterizeObject(object, false);
  }
  std::vector<const RenderObject*> translucent;
  for (const RenderObject& object : scene.objects) {
    if (object.alpha < 1.0 && object.mesh != nullptr) translucent.push_back(&object);
  }
  std::stable_sort(translucent.begin(), translucent.end(),
                   [&](const RenderObject* a, const RenderObject* b) {
                     const Vec3 pa = a->model * Vec3{0.0, 0.0, 0.0};
                     const Vec3 pb = b->model * Vec3{0.0, 0.0, 0.0};
                     return (pa - scene.cameraPosition).lengthSquared() >
                            (pb - scene.cameraPosition).lengthSquared();
                   });
  for (const RenderObject* object : translucent) rasterizeObject(*object, true);

  return true;
}

}  // namespace kimia

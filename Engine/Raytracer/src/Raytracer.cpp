#include <kimia/Raytracer.h>

#include <kimia/AssetPipeline.h>
#include <kimia/Mesh.h>
#include <kimia/World.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace kimia {
namespace raytrace {

namespace {

constexpr f64 kEpsilon = 1e-12;
constexpr f64 kRayOffset = 1e-4;
constexpr f64 kPi = 3.14159265358979323846;

f64 clamp01(f64 v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }

Vec3 lerp(const Vec3& a, const Vec3& b, f64 t) { return a + (b - a) * t; }

Vec3 componentMul(const Vec3& a, const Vec3& b) { return Vec3{a.x * b.x, a.y * b.y, a.z * b.z}; }

// Deterministic splitmix64-style RNG. The state advances with unsigned
// overflow (defined behavior); unit() maps 53 bits to [0, 1).
struct Rng {
  u64 state_;
  explicit Rng(u64 seed) : state_(seed == 0U ? 0x9E3779B97F4A7C15ULL : seed) {}
  f64 unit() {
    state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<f64>(state_ >> 11) * (1.0 / 9007199254740992.0);
  }
};

struct Ray {
  Vec3 origin{0.0, 0.0, 0.0};
  Vec3 direction{0.0, 0.0, -1.0};
};

struct Hit {
  f64 t = 0.0;
  i32 triangle = -1;
  bool hit() const { return triangle >= 0; }
};

// --- BVH over triangles ---

struct BvhNode {
  Vec3 minV{0.0, 0.0, 0.0};
  Vec3 maxV{0.0, 0.0, 0.0};
  i32 left = -1;
  i32 right = -1;
  i32 triStart = 0;
  i32 triCount = 0;
  bool leaf() const { return left < 0; }
};

struct Bvh {
  std::vector<BvhNode> nodes;
  std::vector<i32> indices;  // triangle permutation the leaves refer to
};

Vec3 triangleCentroid(const std::vector<Vec3>& vertices, i32 tri) {
  const Vec3& a = vertices[static_cast<usize>(tri) * 3U];
  const Vec3& b = vertices[static_cast<usize>(tri) * 3U + 1U];
  const Vec3& c = vertices[static_cast<usize>(tri) * 3U + 2U];
  return (a + b + c) * (1.0 / 3.0);
}

BvhNode boundsOf(const std::vector<Vec3>& vertices, const std::vector<i32>& indices, i32 begin, i32 end) {
  BvhNode node;
  node.minV = Vec3{1e18, 1e18, 1e18};
  node.maxV = Vec3{-1e18, -1e18, -1e18};
  for (i32 i = begin; i < end; ++i) {
    const Vec3& a = vertices[static_cast<usize>(indices[static_cast<usize>(i)]) * 3U];
    const Vec3& b = vertices[static_cast<usize>(indices[static_cast<usize>(i)]) * 3U + 1U];
    const Vec3& c = vertices[static_cast<usize>(indices[static_cast<usize>(i)]) * 3U + 2U];
    node.minV.x = std::min(node.minV.x, std::min(a.x, std::min(b.x, c.x)));
    node.minV.y = std::min(node.minV.y, std::min(a.y, std::min(b.y, c.y)));
    node.minV.z = std::min(node.minV.z, std::min(a.z, std::min(b.z, c.z)));
    node.maxV.x = std::max(node.maxV.x, std::max(a.x, std::max(b.x, c.x)));
    node.maxV.y = std::max(node.maxV.y, std::max(a.y, std::max(b.y, c.y)));
    node.maxV.z = std::max(node.maxV.z, std::max(a.z, std::max(b.z, c.z)));
  }
  return node;
}

i32 buildBvh(Bvh& bvh, const std::vector<Vec3>& vertices, const std::vector<Vec3>& centroids,
             std::vector<i32>& indices, i32 begin, i32 end) {
  // Note: the recursion below reallocates bvh.nodes, so this node must only
  // be touched through its index — never through a reference that a deeper
  // push_back can invalidate.
  const i32 nodeIndex = static_cast<i32>(bvh.nodes.size());
  bvh.nodes.push_back(boundsOf(vertices, indices, begin, end));

  const i32 count = end - begin;
  if (count <= 4) {
    bvh.nodes[static_cast<usize>(nodeIndex)].triStart = begin;
    bvh.nodes[static_cast<usize>(nodeIndex)].triCount = count;
    return nodeIndex;
  }

  // Median split on the centroid along the node's largest axis.
  const BvhNode node = bvh.nodes[static_cast<usize>(nodeIndex)];
  const Vec3 extent = node.maxV - node.minV;
  const i32 axis = extent.x >= extent.y && extent.x >= extent.z ? 0 : (extent.y >= extent.z ? 1 : 2);
  const i32 mid = begin + count / 2;
  std::nth_element(indices.begin() + begin, indices.begin() + mid, indices.begin() + end,
                   [&centroids, axis](i32 lhs, i32 rhs) {
                     const Vec3& a = centroids[static_cast<usize>(lhs)];
                     const Vec3& b = centroids[static_cast<usize>(rhs)];
                     if (axis == 0) return a.x < b.x;
                     if (axis == 1) return a.y < b.y;
                     return a.z < b.z;
                   });

  bvh.nodes[static_cast<usize>(nodeIndex)].left = buildBvh(bvh, vertices, centroids, indices, begin, mid);
  bvh.nodes[static_cast<usize>(nodeIndex)].right = buildBvh(bvh, vertices, centroids, indices, mid, end);
  return nodeIndex;
}

Bvh buildBvh(const std::vector<Vec3>& vertices) {
  Bvh bvh;
  const usize triCount = vertices.size() / 3U;
  if (triCount == 0U) return bvh;
  bvh.indices.reserve(triCount);
  for (usize i = 0; i < triCount; ++i) bvh.indices.push_back(static_cast<i32>(i));
  std::vector<Vec3> centroids;
  centroids.reserve(triCount);
  for (usize i = 0; i < triCount; ++i) centroids.push_back(triangleCentroid(vertices, static_cast<i32>(i)));
  buildBvh(bvh, vertices, centroids, bvh.indices, 0, static_cast<i32>(triCount));
  return bvh;
}

// Deterministic, traversal-order-independent hit choice: the winner is the
// min of the total order (quantized t, triangle index). Shared edges then
// resolve to the same triangle for the BVH and brute force alike, so both
// produce bit-identical images.
constexpr f64 kRayFar = 1e9;
constexpr f64 kQuantize = 1073741824.0;  // 2^30

void considerHit(Hit& best, f64 t, i32 triangle) {
  const i64 key = static_cast<i64>(t * kQuantize);
  const i64 bestKey = best.triangle < 0 ? 0 : static_cast<i64>(best.t * kQuantize);
  if (best.triangle < 0 || key < bestKey || (key == bestKey && triangle < best.triangle)) {
    best.t = t;
    best.triangle = triangle;
  }
}

bool intersectTriangle(const Ray& ray, const Vec3& a, const Vec3& b, const Vec3& c, f64& outT) {
  const Vec3 edge1 = b - a;
  const Vec3 edge2 = c - a;
  const Vec3 pvec = cross(ray.direction, edge2);
  const f64 det = dot(edge1, pvec);
  if (std::abs(det) < kEpsilon) return false;
  const f64 invDet = 1.0 / det;
  const Vec3 tvec = ray.origin - a;
  const f64 u = dot(tvec, pvec) * invDet;
  if (u < 0.0 || u > 1.0) return false;
  const Vec3 qvec = cross(tvec, edge1);
  const f64 v = dot(ray.direction, qvec) * invDet;
  if (v < 0.0 || u + v > 1.0) return false;
  const f64 t = dot(edge2, qvec) * invDet;
  if (t <= kRayOffset) return false;
  outT = t;
  return true;
}

bool intersectAabb(const Ray& ray, const Vec3& minV, const Vec3& maxV, f64 tMax) {
  f64 tMin = kRayOffset;
  for (i32 axis = 0; axis < 3; ++axis) {
    const f64 origin = axis == 0 ? ray.origin.x : (axis == 1 ? ray.origin.y : ray.origin.z);
    const f64 direction = axis == 0 ? ray.direction.x : (axis == 1 ? ray.direction.y : ray.direction.z);
    const f64 lo = axis == 0 ? minV.x : (axis == 1 ? minV.y : minV.z);
    const f64 hi = axis == 0 ? maxV.x : (axis == 1 ? maxV.y : maxV.z);
    if (std::abs(direction) < kEpsilon) {
      if (origin < lo || origin > hi) return false;
      continue;
    }
    const f64 inv = 1.0 / direction;
    f64 t0 = (lo - origin) * inv;
    f64 t1 = (hi - origin) * inv;
    if (t0 > t1) std::swap(t0, t1);
    tMin = std::max(tMin, t0);
    tMax = std::min(tMax, t1);
    if (tMin > tMax) return false;
  }
  return true;
}

bool intersectBvh(const Ray& ray, const Bvh& bvh, const std::vector<Vec3>& vertices, Hit& best) {
  best.triangle = -1;
  best.t = kRayFar;
  if (bvh.nodes.empty()) return false;

  i32 stack[64];
  i32 top = 0;
  stack[top++] = 0;
  while (top > 0) {
    const i32 index = stack[--top];
    const BvhNode& node = bvh.nodes[static_cast<usize>(index)];
    // Prune with one quanta of slack: a triangle at t <= best.t + 1/2^30 can
    // still win on the index tie-break, so its node must be visited.
    if (!intersectAabb(ray, node.minV, node.maxV, best.t + 1.0 / kQuantize)) continue;
    if (node.leaf()) {
      for (i32 i = node.triStart; i < node.triStart + node.triCount; ++i) {
        const i32 tri = bvh.indices[static_cast<usize>(i)];
        const usize base = static_cast<usize>(tri) * 3U;
        f64 t = 0.0;
        if (intersectTriangle(ray, vertices[base], vertices[base + 1U], vertices[base + 2U], t)) {
          considerHit(best, t, tri);
        }
      }
    } else {
      stack[top++] = node.left;
      stack[top++] = node.right;
    }
  }
  return best.hit();
}

bool intersectBrute(const Ray& ray, const std::vector<Vec3>& vertices, Hit& best) {
  best.triangle = -1;
  best.t = kRayFar;
  for (usize tri = 0; tri < vertices.size() / 3U; ++tri) {
    const usize base = tri * 3U;
    f64 t = 0.0;
    if (intersectTriangle(ray, vertices[base], vertices[base + 1U], vertices[base + 2U], t)) {
      considerHit(best, t, static_cast<i32>(tri));
    }
  }
  return best.hit();
}

// --- Shading ---

Vec3 normalAt(const std::vector<Vec3>& vertices, i32 tri) {
  const usize base = static_cast<usize>(tri) * 3U;
  const Vec3 n = cross(vertices[base + 1U] - vertices[base], vertices[base + 2U] - vertices[base]);
  const f64 length = n.length();
  return length > kEpsilon ? n / length : Vec3{0.0, 1.0, 0.0};
}

// Orthonormal basis around a (non-parallel) direction for cone sampling.
void basisAround(const Vec3& dir, Vec3& tangent, Vec3& bitangent) {
  const Vec3 helper = std::abs(dir.y) < 0.9 ? Vec3{0.0, 1.0, 0.0} : Vec3{1.0, 0.0, 0.0};
  tangent = cross(dir, helper).normalized();
  bitangent = cross(dir, tangent);
}

// Cosine-weighted hemisphere sample around `normal`.
Vec3 sampleCosineHemisphere(Rng& rng, const Vec3& normal) {
  const f64 u1 = rng.unit();
  const f64 u2 = rng.unit();
  const f64 radius = std::sqrt(u1);
  const f64 angle = 2.0 * kPi * u2;
  Vec3 tangent;
  Vec3 bitangent;
  basisAround(normal, tangent, bitangent);
  return (tangent * (radius * std::cos(angle)) + bitangent * (radius * std::sin(angle)) + normal * std::sqrt(1.0 - u1))
      .normalized();
}

// Schlick fresnel; F0 mixes 4% dielectric with the albedo by metallic.
Vec3 fresnelF0(const Vec3& albedo, f64 metallic) {
  return lerp(Vec3{0.04, 0.04, 0.04}, albedo, metallic);
}

Vec3 fresnelSchlick(f64 cosine, const Vec3& f0) { return f0 + (Vec3{1.0, 1.0, 1.0} - f0) * std::pow(1.0 - cosine, 5.0); }

// GGX normal distribution with roughness alpha.
f64 ggxDistribution(f64 nDotH, f64 alpha) {
  const f64 a2 = alpha * alpha;
  const f64 denom = nDotH * nDotH * (a2 - 1.0) + 1.0;
  return a2 / (kPi * denom * denom);
}

f64 smithG1(f64 nDotV, f64 alpha) {
  const f64 a2 = alpha * alpha;
  const f64 denom = nDotV + std::sqrt(a2 + (1.0 - a2) * nDotV * nDotV);
  return (2.0 * nDotV) / denom;
}

Vec3 skyColor(const Vec3& dir, const Vec3& sunDir, f64 sunIntensity, f64 skyIntensity) {
  const Vec3 zenith{0.20, 0.40, 0.85};
  const Vec3 horizon{0.85, 0.88, 0.95};
  const f64 height = clamp01(dir.y * 0.5 + 0.5);
  Vec3 color = lerp(horizon, zenith, std::pow(height, 0.6)) * skyIntensity;
  const f64 sunDot = dot(dir, sunDir);
  if (sunDot > 0.9993) {
    const f64 core = (sunDot - 0.9993) / 0.0007;
    color += Vec3{1.0, 0.98, 0.9} * (sunIntensity * 8.0 * core);
  }
  return color;
}

struct ShadeContext {
  const RaytraceScene& scene;
  const RaytraceSettings& settings;
  const Bvh* bvh;
  Vec3 sunDir{0.0, 0.0, 0.0};
  Vec3 sunTangent{1.0, 0.0, 0.0};
  Vec3 sunBitangent{0.0, 1.0, 0.0};
};

bool traceAny(const ShadeContext& ctx, const Ray& ray) {
  Hit hit;
  if (ctx.bvh != nullptr) return intersectBvh(ray, *ctx.bvh, ctx.scene.vertices, hit);
  return intersectBrute(ray, ctx.scene.vertices, hit);
}

Vec3 radiance(const ShadeContext& ctx, const Ray& ray, Rng& rng, i32 depth) {
  Hit hit;
  const bool found = ctx.bvh != nullptr ? intersectBvh(ray, *ctx.bvh, ctx.scene.vertices, hit)
                                         : intersectBrute(ray, ctx.scene.vertices, hit);
  if (!found) return skyColor(ray.direction, ctx.sunDir, ctx.settings.sunIntensity, ctx.settings.skyIntensity);

  const usize tri = static_cast<usize>(hit.triangle);
  const Vec3 position = ray.origin + ray.direction * hit.t;
  Vec3 normal = normalAt(ctx.scene.vertices, hit.triangle);
  if (dot(normal, ray.direction) > 0.0) normal = normal * -1.0;  // two-sided

  const Vec3& albedo = ctx.scene.albedo[tri];
  const f64 roughness = ctx.scene.roughness[tri];
  const f64 alpha = std::max(roughness * roughness, 0.02);
  const Vec3 f0 = fresnelF0(albedo, ctx.scene.metallic[tri]);

  Vec3 color{0.0, 0.0, 0.0};

  // Direct sun with a soft shadow cone: the shadow ray jitters the sun
  // direction within its angular radius, so the penumbra accumulates over
  // the samples per pixel.
  {
    Vec3 sunSample = ctx.sunDir;
    if (ctx.settings.sunAngularRadius > 0.0) {
      const f64 u1 = rng.unit();
      const f64 u2 = rng.unit();
      const f64 radius = std::sqrt(u1) * ctx.settings.sunAngularRadius;
      const f64 angle = 2.0 * kPi * u2;
      sunSample = (ctx.sunDir + ctx.sunTangent * (radius * std::cos(angle)) +
                   ctx.sunBitangent * (radius * std::sin(angle)))
                      .normalized();
    }
    const f64 nDotL = dot(normal, sunSample);
    if (nDotL > 0.0) {
      const Ray shadowRay{position + normal * kRayOffset, sunSample};
      if (!traceAny(ctx, shadowRay)) {
        const Vec3 viewDir = ray.direction * -1.0;
        const Vec3 halfVec = (viewDir + sunSample).normalized();
        const f64 nDotH = std::max(dot(normal, halfVec), 0.0);
        const f64 nDotV = std::max(dot(normal, viewDir), 0.0);
        const f64 vDotH = std::max(dot(viewDir, halfVec), 0.0);
        const f64 d = ggxDistribution(nDotH, alpha);
        const f64 g = smithG1(nDotL, alpha) * smithG1(nDotV, alpha);
        const Vec3 f = fresnelSchlick(vDotH, f0);
        const Vec3 specular = f * ((d * g) / std::max(4.0 * nDotL * nDotV, 1e-4));
        const Vec3 diffuse = componentMul(Vec3{1.0, 1.0, 1.0} - f, albedo * (1.0 / kPi));
        color += (diffuse + specular) * (nDotL * ctx.settings.sunIntensity);
      }
    }
  }

  // Indirect: split between a fresnel-weighted reflection and a cosine
  // hemisphere bounce (path-traced global illumination).
  if (depth < ctx.settings.maxBounces) {
    const f64 fresnel = (f0.x + f0.y + f0.z) / 3.0;
    const f64 specularChance = clamp01(fresnel * 0.5 + 0.1);  // keep some diffuse everywhere
    if (rng.unit() < specularChance) {
      Vec3 tangent;
      Vec3 bitangent;
      basisAround(ray.direction * -1.0, tangent, bitangent);
      const Vec3 reflected = (ray.direction - normal * (2.0 * dot(ray.direction, normal))).normalized();
      const f64 spread = roughness * 0.6;
      const Vec3 jitter = tangent * ((rng.unit() * 2.0 - 1.0) * spread) +
                          bitangent * ((rng.unit() * 2.0 - 1.0) * spread);
      const Vec3 bounceDir = (reflected + jitter).normalized();
      const Ray bounce{position + normal * kRayOffset, bounceDir};
      const Vec3 incoming = radiance(ctx, bounce, rng, depth + 1);
      const Vec3 f = fresnelSchlick(std::max(dot(normal, bounceDir), 0.0), f0);
      color += componentMul(f, incoming) * (1.0 / specularChance);
    } else {
      const Vec3 bounceDir = sampleCosineHemisphere(rng, normal);
      const Ray bounce{position + normal * kRayOffset, bounceDir};
      const Vec3 incoming = radiance(ctx, bounce, rng, depth + 1);
      color += componentMul(albedo, incoming) * (1.0 / (1.0 - specularChance));
    }
  }

  return color;
}

u8 toByte(f64 value) {
  const f64 clamped = clamp01(value);
  const f64 gamma = std::pow(clamped, 1.0 / 2.2) * 255.0;
  return static_cast<u8>(gamma + 0.5);
}

}  // namespace

void RaytraceScene::addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& color, f64 rough,
                                f64 metal) {
  vertices.push_back(a);
  vertices.push_back(b);
  vertices.push_back(c);
  albedo.push_back(color);
  roughness.push_back(rough);
  metallic.push_back(metal);
}

namespace {

void addMeshTriangles(RaytraceScene& out, const MeshData& mesh, const Mat4& model, const Vec3& color, f64 rough,
                      f64 metal) {
  for (usize i = 0; i + 2U < mesh.indices.size(); i += 3U) {
    const Vec3 a = model * mesh.positions[static_cast<usize>(mesh.indices[i])];
    const Vec3 b = model * mesh.positions[static_cast<usize>(mesh.indices[i + 1U])];
    const Vec3 c = model * mesh.positions[static_cast<usize>(mesh.indices[i + 2U])];
    out.addTriangle(a, b, c, color, rough, metal);
  }
}

}  // namespace

bool buildFromWorld(const WorldData& world, RaytraceScene& out, std::string& error) {
  out = RaytraceScene{};
  bool sawProblem = false;

  world.scene.forEach([&out, &sawProblem](EntityHandle, const EntityData& entity) {
    const ObjectKind kind = objectKindForName(entity.name);
    Vec3 scale = entity.transform.scale;
    // Sphere entities store their DIAMETER as the scale, but the reference
    // sphere mesh has radius 1 (diameter 2) — so spheres take half scale.
    if (entity.mesh == MeshKind::sphere && entity.meshFile.empty()) scale = scale * 0.5;
    const Mat4 model = Mat4::translation(entity.transform.position) * entity.transform.rotation.toMat4() *
                       Mat4::scaling(scale);
    const f64 metal = kind == ObjectKind::Ball ? 0.15 : 0.0;
    MeshData mesh;

    if (!entity.meshFile.empty()) {
      std::string loadError;
      auto loaded = kimia::assets::loadMesh(entity.meshFile, loadError);
      if (loaded.has_value()) {
        mesh = std::move(loaded->mesh);
      } else {
        mesh = makeCube(1.0);  // unreadable model: fall back to the cube
        sawProblem = true;
      }
    } else if (entity.mesh == MeshKind::plane) {
      mesh = makePlane(1.0, 1.0);
    } else if (entity.mesh == MeshKind::sphere) {
      mesh = makeSphere(16, 8);
    } else {
      mesh = makeCube(1.0);
    }
    addMeshTriangles(out, mesh, model, entity.color, entity.roughness, metal);
  });

  // The ball follows its physics rest position when the scene has no Ball
  // entity (fresh worlds start without one).
  if (world.scene.find("Ball") == kNullEntity) {
    const f64 radius = world.ball.radius;
    const MeshData sphere = makeSphere(16, 8);
    const Mat4 model = Mat4::translation(Vec3{0.0, radius, 0.0}) * Mat4::scaling(Vec3{radius, radius, radius});
    addMeshTriangles(out, sphere, model, world.ball.color, 0.3, 0.15);
  }

  if (sawProblem) error = "one or more model files could not be read (rendered as cubes)";
  return true;
}

void applyWorldSky(const WorldData& world, RaytraceSettings& settings) {
  const f64 hour = world.profile.hour;
  const f64 rain = world.profile.rain;
  // The sun tracks a simple arc: up at noon, below the horizon at night,
  // and swinging east to west across the day.
  const f64 dayAngle = hour / 24.0 * 2.0 * kPi;
  const f64 height = -std::cos(dayAngle);   // -1 midnight ... +1 midday
  const f64 across = std::sin(dayAngle);    // east in the morning, west by evening
  // sunDirection is the direction the light TRAVELS, so it points down when
  // the sun is up: negate the height.
  Vec3 direction{across * 0.6, -height, -0.35};
  const f64 length = direction.length();
  if (length > 1e-9) direction = direction * (1.0 / length);
  settings.sunDirection = direction;

  // Below the horizon there is no sun at all — only the floodlights, which
  // the sky term stands in for. Rain puts cloud in the way.
  const f64 above = height <= 0.0 ? 0.0 : height;
  const f64 cloud = 1.0 - 0.65 * rain;
  settings.sunIntensity = 2.2 * above * cloud;
  // The sky never goes fully black: a night pitch is lit, just dimly, and
  // an overcast sky is flat rather than dark.
  const f64 skyDay = 0.6 * cloud;
  const f64 skyNight = 0.10;
  settings.skyIntensity = skyNight + (skyDay - skyNight) * above;
}

bool render(const RaytraceScene& scene, const RaytraceSettings& settings, const RaytraceCamera& camera, Image& out) {
  return render(scene, settings, camera, false, out);
}

bool render(const RaytraceScene& scene, const RaytraceSettings& settings, const RaytraceCamera& camera,
            bool bruteForce, Image& out) {
  if (settings.width <= 0 || settings.height <= 0 || settings.samplesPerPixel <= 0 || scene.triangleCount() == 0U) {
    return false;
  }
  const i32 width = settings.width;
  const i32 height = settings.height;

  Bvh bvh = buildBvh(scene.vertices);
  static_cast<void>(bruteForce);  // traversal switch below

  ShadeContext ctx{scene, settings, bruteForce ? nullptr : &bvh, Vec3{0.0, 0.0, 0.0}, Vec3{1.0, 0.0, 0.0},
                   Vec3{0.0, 1.0, 0.0}};
  ctx.sunDir = settings.sunDirection.normalized();
  basisAround(ctx.sunDir, ctx.sunTangent, ctx.sunBitangent);

  const Vec3 forward = (camera.target - camera.eye).normalized();
  const Vec3 right = cross(forward, camera.up).normalized();
  const Vec3 upv = cross(right, forward);
  const f64 aspect = static_cast<f64>(width) / static_cast<f64>(height);
  const f64 tanHalf = std::tan((camera.fovYDegrees * kPi / 180.0) * 0.5);

  out = Image{};
  out.width = width;
  out.height = height;
  out.channels = 3;
  out.pixels.assign(static_cast<usize>(width) * static_cast<usize>(height) * 3U, 0U);

  for (i32 y = 0; y < height; ++y) {
    for (i32 x = 0; x < width; ++x) {
      const f64 ndcX = ((static_cast<f64>(x) + 0.5) / static_cast<f64>(width) * 2.0 - 1.0) * aspect * tanHalf;
      const f64 ndcY = (1.0 - (static_cast<f64>(y) + 0.5) / static_cast<f64>(height) * 2.0) * tanHalf;
      const Vec3 direction = (forward + right * ndcX + upv * ndcY).normalized();
      const Ray primary{camera.eye, direction};

      Vec3 accumulated{0.0, 0.0, 0.0};
      for (i32 sample = 0; sample < settings.samplesPerPixel; ++sample) {
        // The mixing constants are u64 rather than ULL literals: u64 is
        // `unsigned long` here, and mixing it with `unsigned long long`
        // trips -Wsign-conversion on Clang.
        const u64 pixelSeed =
            (static_cast<u64>(y) * u64{73856093} ^ static_cast<u64>(x) * u64{19349663}) + u64{1};
        Rng rng(pixelSeed + static_cast<u64>(sample) * u64{83492791});
        accumulated += radiance(ctx, primary, rng, 0);
      }
      const f64 inv = settings.exposure / static_cast<f64>(settings.samplesPerPixel);
      u8* pixel = out.at(x, y);
      pixel[0] = toByte(accumulated.x * inv);
      pixel[1] = toByte(accumulated.y * inv);
      pixel[2] = toByte(accumulated.z * inv);
    }
  }
  return true;
}

}  // namespace raytrace
}  // namespace kimia

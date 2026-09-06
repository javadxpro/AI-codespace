#include <kimia/Picking.h>

#include <algorithm>
#include <cmath>

namespace kimia {
namespace pick {

namespace {

constexpr f64 kEpsilon = 1e-12;

// Nothing smaller than this can be tapped comfortably with a fingertip,
// so a thin or tiny object is given a bit of padding to catch touches.
constexpr f64 kMinimumHalfExtent = 0.15;

// Slab test: where does a ray enter an axis-aligned box? Returns false on
// a miss, or when the box is entirely behind the ray's origin.
bool rayBox(const Ray& ray, const Vec3& center, const Vec3& half, f64& outDistance) {
  f64 nearest = 0.0;
  f64 farthest = 1e30;
  const f64 o[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
  const f64 d[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
  const f64 c[3] = {center.x, center.y, center.z};
  const f64 h[3] = {half.x, half.y, half.z};

  for (i32 axis = 0; axis < 3; ++axis) {
    if (std::abs(d[axis]) < kEpsilon) {
      // Parallel to this pair of faces: a miss unless we start between them.
      if (o[axis] < c[axis] - h[axis] || o[axis] > c[axis] + h[axis]) return false;
      continue;
    }
    const f64 inverse = 1.0 / d[axis];
    f64 t1 = (c[axis] - h[axis] - o[axis]) * inverse;
    f64 t2 = (c[axis] + h[axis] - o[axis]) * inverse;
    if (t1 > t2) std::swap(t1, t2);
    nearest = std::max(nearest, t1);
    farthest = std::min(farthest, t2);
    if (nearest > farthest) return false;
  }
  outDistance = nearest;
  return true;
}

}  // namespace

Ray rayThroughPixel(const Viewport& viewport, f64 pixelX, f64 pixelY) {
  Ray ray;
  ray.origin = viewport.eye;
  if (viewport.width <= 0 || viewport.height <= 0) return ray;

  // Pixel -> normalised device coordinates. The Y flip is because a touch
  // is reported from the top-left and NDC counts up from the bottom.
  const f64 ndcX = (pixelX / static_cast<f64>(viewport.width)) * 2.0 - 1.0;
  const f64 ndcY = 1.0 - (pixelY / static_cast<f64>(viewport.height)) * 2.0;

  // Undo the projection and the view to get a direction in world space.
  const Mat4 inverseProjection = viewport.projection.inverse();
  const Vec4 nearPoint = inverseProjection * Vec4{ndcX, ndcY, -1.0, 1.0};
  if (std::abs(nearPoint.w) < kEpsilon) return ray;
  const Vec3 inView{nearPoint.x / nearPoint.w, nearPoint.y / nearPoint.w, nearPoint.z / nearPoint.w};

  const Mat4 inverseView = viewport.view.inverse();
  const Vec4 world = inverseView * Vec4{inView.x, inView.y, inView.z, 1.0};
  Vec3 direction{world.x - viewport.eye.x, world.y - viewport.eye.y, world.z - viewport.eye.z};
  const f64 length = direction.length();
  if (length < kEpsilon) return ray;
  ray.direction = direction * (1.0 / length);
  return ray;
}

Hit pickAt(const Viewport& viewport, const std::vector<Target>& targets, f64 pixelX, f64 pixelY) {
  const Ray ray = rayThroughPixel(viewport, pixelX, pixelY);
  Hit best;
  for (const Target& target : targets) {
    f64 distance = 0.0;
    if (!rayBox(ray, target.center, target.halfExtents, distance)) continue;
    if (best.hit && distance >= best.distance) continue;
    best.hit = true;
    best.name = target.name;
    best.distance = distance;
    best.point = ray.origin + ray.direction * distance;
  }
  return best;
}

std::vector<Target> targetsFromScene(const Scene& scene) {
  std::vector<Target> targets;
  scene.forEach([&targets](EntityHandle, const EntityData& entity) {
    // The ground is the backdrop, not a thing you select by tapping the
    // middle of the screen — it would swallow every miss.
    if (entity.name == "Ground") return;
    Target target;
    target.name = entity.name;
    target.center = entity.transform.position;
    const Vec3& scale = entity.transform.scale;
    target.halfExtents = Vec3{std::max(std::abs(scale.x) * 0.5, kMinimumHalfExtent),
                              std::max(std::abs(scale.y) * 0.5, kMinimumHalfExtent),
                              std::max(std::abs(scale.z) * 0.5, kMinimumHalfExtent)};
    targets.push_back(target);
  });
  return targets;
}

bool pixelOnPlane(const Viewport& viewport, f64 pixelX, f64 pixelY, f64 planeY, Vec3& out) {
  const Ray ray = rayThroughPixel(viewport, pixelX, pixelY);
  if (std::abs(ray.direction.y) < 1e-6) return false;  // parallel to the floor
  const f64 distance = (planeY - ray.origin.y) / ray.direction.y;
  if (distance < 0.0) return false;  // the plane is behind the camera
  // A ray near the horizon still technically meets the floor, but hundreds
  // of metres out. Dragging to such a point would throw the object off the
  // map, so a grazing hit counts as a miss.
  if (distance > kMaxPlaneReach) return false;
  out = ray.origin + ray.direction * distance;
  return true;
}

bool dragDelta(const Viewport& viewport, f64 fromX, f64 fromY, f64 toX, f64 toY, f64 planeY, Vec3& out) {
  Vec3 start;
  Vec3 end;
  if (!pixelOnPlane(viewport, fromX, fromY, planeY, start)) return false;
  if (!pixelOnPlane(viewport, toX, toY, planeY, end)) return false;
  // Only the horizontal part: a drag slides something along the floor, it
  // does not lift it toward the camera.
  out = Vec3{end.x - start.x, 0.0, end.z - start.z};
  return true;
}

Vec3 snapTo(const Vec3& position, f64 step) {
  if (step <= 0.0) return position;
  const auto round = [step](f64 value) { return std::round(value / step) * step; };
  return Vec3{round(position.x), position.y, round(position.z)};
}

}  // namespace pick
}  // namespace kimia

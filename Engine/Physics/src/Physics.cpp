#include <kimia/Physics.h>

#include <algorithm>
#include <cmath>

namespace kimia {

namespace {

constexpr f64 kEpsilon = 1e-12;

f64 clampValue(f64 value, f64 lo, f64 hi) { return value < lo ? lo : (value > hi ? hi : value); }

}  // namespace

PhysicsWorld::PhysicsWorld(f64 fixedDt, u32 maxStepsPerFrame)
    : fixedDt_(fixedDt), accumulator_(fixedDt, maxStepsPerFrame) {}

u32 PhysicsWorld::addSphere(const SphereBody& body) {
  const u32 id = nextId_;
  ++nextId_;
  spheres_.emplace(id, body);
  return id;
}

bool PhysicsWorld::removeSphere(u32 id) { return spheres_.erase(id) > 0U; }

SphereBody* PhysicsWorld::sphere(u32 id) {
  const auto found = spheres_.find(id);
  return found == spheres_.end() ? nullptr : &found->second;
}

const SphereBody* PhysicsWorld::sphere(u32 id) const {
  const auto found = spheres_.find(id);
  return found == spheres_.end() ? nullptr : &found->second;
}

u32 PhysicsWorld::addPlane(f64 y) {
  const u32 id = nextId_;
  ++nextId_;
  planes_.emplace(id, StaticPlane{y});
  return id;
}

u32 PhysicsWorld::addBox(const Vec3& center, const Vec3& halfExtents) {
  const u32 id = nextId_;
  ++nextId_;
  boxes_.emplace(id, StaticBox{center, halfExtents});
  return id;
}

void PhysicsWorld::clear() {
  spheres_.clear();
  planes_.clear();
  boxes_.clear();
  time_ = 0.0;
  steps_ = 0U;
}

void PhysicsWorld::resolvePlane(SphereBody& body, f64 planeY) {
  const f64 distance = body.position.y - body.radius - planeY;
  if (distance >= 0.0) return;
  ++body.collisionCount;
  body.position.y = planeY + body.radius;
  if (body.velocity.y < 0.0) body.velocity.y = -body.restitution * body.velocity.y;
  // Tangential friction and rolling friction on the contact plane (XZ).
  const f64 decay = std::max(0.0, (1.0 - body.friction * fixedDt_) * (1.0 - body.rollingFriction * fixedDt_));
  body.velocity.x *= decay;
  body.velocity.z *= decay;
}

void PhysicsWorld::resolveBox(SphereBody& body, const StaticBox& box) {
  const Vec3 closest{
      clampValue(body.position.x, box.center.x - box.halfExtents.x, box.center.x + box.halfExtents.x),
      clampValue(body.position.y, box.center.y - box.halfExtents.y, box.center.y + box.halfExtents.y),
      clampValue(body.position.z, box.center.z - box.halfExtents.z, box.center.z + box.halfExtents.z),
  };
  const Vec3 delta = body.position - closest;
  const f64 distanceSquared = delta.lengthSquared();
  if (distanceSquared >= body.radius * body.radius) return;
  ++body.collisionCount;

  Vec3 normal;
  f64 penetration = 0.0;
  if (distanceSquared > kEpsilon) {
    const f64 distance = std::sqrt(distanceSquared);
    normal = delta / distance;
    penetration = body.radius - distance;
  } else {
    // Center inside the box: push out along the axis of least penetration.
    const f64 dx = box.halfExtents.x - std::abs(body.position.x - box.center.x);
    const f64 dy = box.halfExtents.y - std::abs(body.position.y - box.center.y);
    const f64 dz = box.halfExtents.z - std::abs(body.position.z - box.center.z);
    if (dx <= dy && dx <= dz) {
      normal = Vec3{body.position.x >= box.center.x ? 1.0 : -1.0, 0.0, 0.0};
      penetration = dx + body.radius;
    } else if (dy <= dz) {
      normal = Vec3{0.0, body.position.y >= box.center.y ? 1.0 : -1.0, 0.0};
      penetration = dy + body.radius;
    } else {
      normal = Vec3{0.0, 0.0, body.position.z >= box.center.z ? 1.0 : -1.0};
      penetration = dz + body.radius;
    }
  }

  body.position += normal * penetration;
  const f64 velocityNormal = kimia::dot(body.velocity, normal);
  if (velocityNormal < 0.0) {
    body.velocity -= normal * ((1.0 + body.restitution) * velocityNormal);
    // Friction decays the tangential component while in contact.
    const Vec3 tangential = body.velocity - normal * kimia::dot(body.velocity, normal);
    const f64 decay = std::max(0.0, 1.0 - body.friction * fixedDt_);
    body.velocity = normal * kimia::dot(body.velocity, normal) + tangential * decay;
  }
}

void PhysicsWorld::step() {
  for (auto& spherePair : spheres_) {
    SphereBody& body = spherePair.second;
    body.collisionCount = 0U;
    body.velocity.y -= kGravity * fixedDt_;
    body.position += body.velocity * fixedDt_;
    for (const auto& planePair : planes_) resolvePlane(body, planePair.second.y);
    for (const auto& boxPair : boxes_) resolveBox(body, boxPair.second);
  }
  time_ += fixedDt_;
  ++steps_;
}

u32 PhysicsWorld::advance(f64 hostSeconds) {
  return accumulator_.advance(hostSeconds, [this](f64) { step(); });
}

}  // namespace kimia

#include <kimia/Physics.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace kimia {

namespace {

constexpr f64 kEpsilon = 1e-12;

// Sequential-impulse solver tuning: 20 iterations of fresh contact
// resolution per step. Positional correction removes a fraction of the
// penetration per iteration, so stacked bodies converge without jitter;
// 20 iterations propagate the weight of a stack through the chain (each
// iteration halves the transferred residual, ~2^-20 of g*dt at the end).
constexpr u32 kSolverIterations = 20U;
constexpr f64 kPositionCorrection = 0.8;

f64 clampValue(f64 value, f64 lo, f64 hi) { return value < lo ? lo : (value > hi ? hi : value); }

// Closest point of `point` to the AABB(center, halfExtents).
Vec3 closestPointOnBox(const Vec3& point, const Vec3& center, const Vec3& halfExtents) {
  return Vec3{
      clampValue(point.x, center.x - halfExtents.x, center.x + halfExtents.x),
      clampValue(point.y, center.y - halfExtents.y, center.y + halfExtents.y),
      clampValue(point.z, center.z - halfExtents.z, center.z + halfExtents.z),
  };
}

// Sphere-vs-box contact. On success `normal` points from the sphere toward
// the box and `penetration` is the overlap depth. `embedded` is true when the
// sphere CENTER is inside the box (deep overlap): such contacts only correct
// position (exit through the top), never apply a velocity impulse.
bool sphereBoxContact(const Vec3& spherePos, f64 radius, const Vec3& boxPos, const Vec3& half,
                      Vec3& normal, f64& penetration, bool& embedded) {
  const Vec3 closest = closestPointOnBox(spherePos, boxPos, half);
  const Vec3 delta = spherePos - closest;  // points from the box toward the sphere
  const f64 distanceSquared = delta.lengthSquared();
  if (distanceSquared > radius * radius) return false;  // touching (== r^2) counts as contact
  embedded = false;
  if (distanceSquared > kEpsilon) {
    const f64 distance = std::sqrt(distanceSquared);
    normal = (delta / distance) * -1.0;  // flip: from the sphere toward the box
    penetration = radius - distance;
  } else {
    // Sphere center inside the box: exit UP through the top face. Exiting
    // along the nearest face would push the sphere down through the floor
    // plane when a box sits on the ground (the plane and the box then fight
    // and the sphere sinks), so the center-inside case always pops the
    // sphere out on top of the box. The solver moves the sphere along
    // -normal, so normal points back down into the box here.
    normal = Vec3{0.0, -1.0, 0.0};
    penetration = (boxPos.y + half.y - spherePos.y) + radius;
    embedded = true;
  }
  return true;
}

// Axis-aligned box-vs-box contact. On success `normal` points from A toward B.
bool boxBoxContact(const Vec3& posA, const Vec3& halfA, const Vec3& posB, const Vec3& halfB,
                   Vec3& normal, f64& penetration) {
  const f64 dx = halfA.x + halfB.x - std::abs(posB.x - posA.x);
  const f64 dy = halfA.y + halfB.y - std::abs(posB.y - posA.y);
  const f64 dz = halfA.z + halfB.z - std::abs(posB.z - posA.z);
  if (dx <= 0.0 || dy <= 0.0 || dz <= 0.0) return false;
  const Vec3 delta = posB - posA;
  if (dx <= dy && dx <= dz) {
    normal = Vec3{delta.x >= 0.0 ? 1.0 : -1.0, 0.0, 0.0};
    penetration = dx;
  } else if (dy <= dz) {
    normal = Vec3{0.0, delta.y >= 0.0 ? 1.0 : -1.0, 0.0};
    penetration = dy;
  } else {
    normal = Vec3{0.0, 0.0, delta.z >= 0.0 ? 1.0 : -1.0};
    penetration = dz;
  }
  return true;
}

// Constant-force tangential damping: decelerates by factor * g per second,
// never overshooting past zero (the ball/crate friction model).
void dampTangential(Vec3& velocity, const Vec3& normal, f64 factor, f64 dt) {
  const Vec3 tangential = velocity - normal * kimia::dot(velocity, normal);
  const f64 speed = tangential.length();
  if (speed <= 0.0) return;
  const f64 deceleration = factor * kGravity * dt;
  const f64 scale = speed > deceleration ? 1.0 - deceleration / speed : 0.0;
  velocity -= tangential * (1.0 - scale);
}

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

u32 PhysicsWorld::addDynamicBox(const DynamicBox& body) {
  const u32 id = nextId_;
  ++nextId_;
  dynamicBoxes_.emplace(id, body);
  return id;
}

bool PhysicsWorld::removeDynamicBox(u32 id) { return dynamicBoxes_.erase(id) > 0U; }

DynamicBox* PhysicsWorld::dynamicBox(u32 id) {
  const auto found = dynamicBoxes_.find(id);
  return found == dynamicBoxes_.end() ? nullptr : &found->second;
}

const DynamicBox* PhysicsWorld::dynamicBox(u32 id) const {
  const auto found = dynamicBoxes_.find(id);
  return found == dynamicBoxes_.end() ? nullptr : &found->second;
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
  dynamicBoxes_.clear();
  planes_.clear();
  boxes_.clear();
  time_ = 0.0;
  steps_ = 0U;
}

void PhysicsWorld::collectContacts(std::vector<Contact>& contacts) const {
  contacts.clear();

  // Sphere vs plane.
  for (const auto& spherePair : spheres_) {
    const SphereBody& sphere = spherePair.second;
    for (const auto& planePair : planes_) {
      const f64 distance = sphere.position.y - sphere.radius - planePair.second.y;
      if (distance <= 0.0) {  // touching counts: resting bodies stay damped
        contacts.push_back(
            Contact{true, false, true, false, spherePair.first, planePair.first, Vec3{0.0, -1.0, 0.0},
                    -distance, sphere.restitution});
      }
    }
  }

  // Sphere vs static box.
  for (const auto& spherePair : spheres_) {
    const SphereBody& sphere = spherePair.second;
    for (const auto& boxPair : boxes_) {
      Vec3 normal;
      f64 penetration = 0.0;
      bool embedded = false;
      if (sphereBoxContact(sphere.position, sphere.radius, boxPair.second.center, boxPair.second.halfExtents,
                           normal, penetration, embedded)) {
        contacts.push_back(Contact{true, false, true, embedded, spherePair.first, boxPair.first, normal,
                                   penetration, sphere.restitution});
      }
    }
  }

  // Sphere vs sphere.
  for (auto a = spheres_.begin(); a != spheres_.end(); ++a) {
    for (auto b = std::next(a); b != spheres_.end(); ++b) {
      const Vec3 delta = b->second.position - a->second.position;
      const f64 radii = a->second.radius + b->second.radius;
      const f64 distanceSquared = delta.lengthSquared();
      if (distanceSquared >= radii * radii) continue;
      Vec3 normal{1.0, 0.0, 0.0};
      f64 penetration = radii;
      if (distanceSquared > kEpsilon) {
        const f64 distance = std::sqrt(distanceSquared);
        normal = delta / distance;
        penetration = radii - distance;
      }
      const f64 restitution = std::max(a->second.restitution, b->second.restitution);
      contacts.push_back(
          Contact{true, true, false, false, a->first, b->first, normal, penetration, restitution});
    }
  }

  // Dynamic box vs plane.
  for (const auto& boxPair : dynamicBoxes_) {
    const DynamicBox& box = boxPair.second;
    for (const auto& planePair : planes_) {
      const f64 distance = box.position.y - box.halfExtents.y - planePair.second.y;
      if (distance <= 0.0) {  // touching counts: resting bodies stay damped
        contacts.push_back(
            Contact{false, false, true, false, boxPair.first, planePair.first, Vec3{0.0, -1.0, 0.0},
                    -distance, box.restitution});
      }
    }
  }

  // Dynamic box vs static box.
  for (const auto& boxPair : dynamicBoxes_) {
    const DynamicBox& box = boxPair.second;
    for (const auto& staticPair : boxes_) {
      Vec3 normal;
      f64 penetration = 0.0;
      if (boxBoxContact(box.position, box.halfExtents, staticPair.second.center, staticPair.second.halfExtents,
                        normal, penetration)) {
        contacts.push_back(
            Contact{false, false, true, false, boxPair.first, staticPair.first, normal, penetration,
                    box.restitution});
      }
    }
  }

  // Dynamic box vs dynamic box.
  for (auto a = dynamicBoxes_.begin(); a != dynamicBoxes_.end(); ++a) {
    for (auto b = std::next(a); b != dynamicBoxes_.end(); ++b) {
      Vec3 normal;
      f64 penetration = 0.0;
      if (boxBoxContact(a->second.position, a->second.halfExtents, b->second.position, b->second.halfExtents,
                        normal, penetration)) {
        const f64 restitution = std::max(a->second.restitution, b->second.restitution);
        contacts.push_back(
            Contact{false, false, false, false, a->first, b->first, normal, penetration, restitution});
      }
    }
  }

  // Sphere vs dynamic box.
  for (const auto& spherePair : spheres_) {
    const SphereBody& sphere = spherePair.second;
    for (const auto& boxPair : dynamicBoxes_) {
      Vec3 normal;
      f64 penetration = 0.0;
      bool embedded = false;
      if (sphereBoxContact(sphere.position, sphere.radius, boxPair.second.position, boxPair.second.halfExtents,
                           normal, penetration, embedded)) {
        const f64 restitution = std::max(sphere.restitution, boxPair.second.restitution);
        contacts.push_back(Contact{true, false, false, embedded, spherePair.first, boxPair.first, normal,
                                   penetration, restitution});
      }
    }
  }
}

void PhysicsWorld::resolvePair(const Contact& contact, bool countContacts) {
  Vec3* posA = nullptr;
  Vec3* velA = nullptr;
  f64 invA = 0.0;
  u32* countA = nullptr;
  if (contact.sphereA) {
    SphereBody* body = sphere(contact.idA);
    if (body == nullptr) return;
    posA = &body->position;
    velA = &body->velocity;
    invA = body->mass > kEpsilon ? 1.0 / body->mass : 0.0;
    countA = &body->collisionCount;
  } else {
    DynamicBox* body = dynamicBox(contact.idA);
    if (body == nullptr) return;
    posA = &body->position;
    velA = &body->velocity;
    invA = body->mass > kEpsilon ? 1.0 / body->mass : 0.0;
    countA = &body->collisionCount;
  }

  Vec3* posB = nullptr;
  Vec3* velB = nullptr;
  f64 invB = 0.0;
  u32* countB = nullptr;
  if (contact.staticB) {
    // Static: the id is a plane (planes_) or a static box (boxes_).
    if (planes_.find(contact.idB) == planes_.end() && boxes_.find(contact.idB) == boxes_.end()) return;
  } else if (contact.sphereB) {
    SphereBody* body = sphere(contact.idB);
    if (body == nullptr) return;
    posB = &body->position;
    velB = &body->velocity;
    invB = body->mass > kEpsilon ? 1.0 / body->mass : 0.0;
    countB = &body->collisionCount;
  } else {
    DynamicBox* body = dynamicBox(contact.idB);
    if (body == nullptr) return;
    posB = &body->position;
    velB = &body->velocity;
    invB = body->mass > kEpsilon ? 1.0 / body->mass : 0.0;
    countB = &body->collisionCount;
  }

  const f64 total = invA + invB;
  if (total <= kEpsilon) return;  // both effectively static: nothing to resolve

  // Positional correction: separate the bodies along the contact normal.
  const f64 correction = contact.penetration * kPositionCorrection;
  if (posA != nullptr) *posA -= contact.normal * (correction * (invA / total));
  if (posB != nullptr) *posB += contact.normal * (correction * (invB / total));

  // Normal impulse: reflect approaching motion with restitution; impacts
  // slower than the threshold settle (no micro-bouncing).
  const Vec3 velocityA = velA != nullptr ? *velA : Vec3{0.0, 0.0, 0.0};
  const Vec3 velocityB = velB != nullptr ? *velB : Vec3{0.0, 0.0, 0.0};
  const f64 approach = kimia::dot(velocityB - velocityA, contact.normal);
  if (approach < 0.0 && !contact.embedded) {
    const f64 impulse =
        (-approach >= kContactRestitutionThreshold ? -(1.0 + contact.restitution) * approach : -approach) / total;
    if (velA != nullptr) *velA -= contact.normal * (impulse * invA);
    if (velB != nullptr) *velB += contact.normal * (impulse * invB);
  }

  if (countContacts) {
    if (countA != nullptr) ++*countA;
    if (countB != nullptr) ++*countB;
  }
}

void PhysicsWorld::applyPairFriction(const Contact& contact) {
  if (contact.sphereA) {
    SphereBody* body = sphere(contact.idA);
    if (body != nullptr) {
      dampTangential(body->velocity, contact.normal, body->friction + body->rollingFriction, fixedDt_);
    }
  } else {
    DynamicBox* body = dynamicBox(contact.idA);
    if (body != nullptr) {
      dampTangential(body->velocity, contact.normal, body->friction + body->rollingFriction, fixedDt_);
    }
  }
  if (!contact.staticB) {
    if (contact.sphereB) {
      SphereBody* body = sphere(contact.idB);
      if (body != nullptr) {
        dampTangential(body->velocity, contact.normal, body->friction + body->rollingFriction, fixedDt_);
      }
    } else {
      DynamicBox* body = dynamicBox(contact.idB);
      if (body != nullptr) {
        dampTangential(body->velocity, contact.normal, body->friction + body->rollingFriction, fixedDt_);
      }
    }
  }
}

f64 PhysicsWorld::resolveSpawnHeight(const Vec3& center, f64 radius, f64 maxHeight) const {
  f64 y = center.y;
  for (int iteration = 0; iteration < 8; ++iteration) {
    f64 raiseTo = y;
    for (const auto& boxPair : boxes_) {
      const StaticBox& box = boxPair.second;
      if (std::abs(center.x - box.center.x) >= box.halfExtents.x + radius) continue;
      if (std::abs(center.z - box.center.z) >= box.halfExtents.z + radius) continue;
      const f64 top = box.center.y + box.halfExtents.y;
      const f64 bottom = box.center.y - box.halfExtents.y;
      if (y + radius > bottom + 1e-9 && y - radius < top - 1e-9) {
        raiseTo = std::max(raiseTo, top + radius + 1e-4);
      }
    }
    for (const auto& boxPair : dynamicBoxes_) {
      const DynamicBox& box = boxPair.second;
      if (std::abs(center.x - box.position.x) >= box.halfExtents.x + radius) continue;
      if (std::abs(center.z - box.position.z) >= box.halfExtents.z + radius) continue;
      const f64 top = box.position.y + box.halfExtents.y;
      const f64 bottom = box.position.y - box.halfExtents.y;
      if (y + radius > bottom + 1e-9 && y - radius < top - 1e-9) {
        raiseTo = std::max(raiseTo, top + radius + 1e-4);
      }
    }
    if (raiseTo <= y) return y;
    if (raiseTo > maxHeight) return maxHeight;
    y = raiseTo;
  }
  return y;
}

void PhysicsWorld::step() {
  for (auto& spherePair : spheres_) {
    SphereBody& body = spherePair.second;
    body.collisionCount = 0U;
    body.velocity.y -= kGravity * fixedDt_;
    body.position += body.velocity * fixedDt_;
  }
  for (auto& boxPair : dynamicBoxes_) {
    DynamicBox& body = boxPair.second;
    body.collisionCount = 0U;
    body.velocity.y -= kGravity * fixedDt_;
    body.position += body.velocity * fixedDt_;
  }

  std::vector<Contact> contacts;
  for (u32 iteration = 0U; iteration < kSolverIterations; ++iteration) {
    collectContacts(contacts);
    for (const Contact& contact : contacts) resolvePair(contact, iteration == 0U);
  }

  // One friction pass per contact pair, after the impulses converge.
  collectContacts(contacts);
  for (const Contact& contact : contacts) applyPairFriction(contact);

  time_ += fixedDt_;
  ++steps_;
}

u32 PhysicsWorld::advance(f64 hostSeconds) {
  return accumulator_.advance(hostSeconds, [this](f64) { step(); });
}

}  // namespace kimia

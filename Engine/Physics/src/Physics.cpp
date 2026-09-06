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
    : fixedDt_(fixedDt), accumulator_(fixedDt, maxStepsPerFrame) {
  addCharacter(CharacterBody{});  // character 1: the player, always present
}

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
  // Characters: the extra players go away, but player 1 is KEPT as it is.
  // clear() rebuilds the level, not the player — the game positions the
  // player itself (resetCharacter) and used to rely on it surviving here.
  const CharacterBody primary = characters_.count(kPrimaryCharacter) > 0U
                                    ? characters_.at(kPrimaryCharacter)
                                    : CharacterBody{};
  characters_.clear();
  nextCharacterId_ = kPrimaryCharacter;
  addCharacter(primary);
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

void PhysicsWorld::setWetness(f64 wetness) {
  wetness_ = wetness < 0.0 ? 0.0 : (wetness > 1.0 ? 1.0 : wetness);
}

// Dry returns exactly 1.0, so a dry world is bit-identical to one with no
// notion of weather at all.
f64 PhysicsWorld::gripFactor() const {
  if (wetness_ == 0.0) return 1.0;
  return 1.0 - (1.0 - kWetMinGrip) * wetness_;
}

void PhysicsWorld::applyPairFriction(const Contact& contact) {
  const f64 grip = gripFactor();
  if (contact.sphereA) {
    SphereBody* body = sphere(contact.idA);
    if (body != nullptr) {
      dampTangential(body->velocity, contact.normal, (body->friction + body->rollingFriction) * grip, fixedDt_);
    }
  } else {
    DynamicBox* body = dynamicBox(contact.idA);
    if (body != nullptr) {
      dampTangential(body->velocity, contact.normal, (body->friction + body->rollingFriction) * grip, fixedDt_);
    }
  }
  if (!contact.staticB) {
    if (contact.sphereB) {
      SphereBody* body = sphere(contact.idB);
      if (body != nullptr) {
        dampTangential(body->velocity, contact.normal, (body->friction + body->rollingFriction) * grip, fixedDt_);
      }
    } else {
      DynamicBox* body = dynamicBox(contact.idB);
      if (body != nullptr) {
        dampTangential(body->velocity, contact.normal, (body->friction + body->rollingFriction) * grip, fixedDt_);
      }
    }
  }
}

// --- Wind ---

f64 Wind::speed() const { return std::sqrt(acceleration.x * acceleration.x + acceleration.z * acceleration.z); }

f64 Wind::direction() const {
  // Mirror of WorldEditor::aimDirection: yaw 0 points toward -Z, and yaw
  // grows toward -X, so (x, z) = (-sin, -cos) * speed.
  if (!active()) return 0.0;
  return std::atan2(-acceleration.x, -acceleration.z);
}

Wind makeWind(f64 speed, f64 direction) {
  const f64 clamped = std::max(0.0, std::min(speed, kMaxWindAcceleration));
  Wind wind;
  wind.acceleration = Vec3{-std::sin(direction) * clamped, 0.0, -std::cos(direction) * clamped};
  // Kill the denormal residue of sin/cos at the cardinal angles so an
  // "off" wind (speed 0) is exactly inactive and byte-stable.
  if (clamped == 0.0) wind.acceleration = Vec3{0.0, 0.0, 0.0};
  return wind;
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
    // Wind pushes a MOVING ball, never a resting one (see Wind). The
    // collisionCount still holds the PREVIOUS step's contact count here (it
    // is cleared just below), which is how we know we were on the ground.
    const bool grounded = body.collisionCount != 0U;
    body.collisionCount = 0U;
    // The speed the body ARRIVED with, before this step's gravity: a ball
    // parked on the ground reads exactly zero here (the contact solver
    // zeroed it last step), which is what makes "at rest" detectable.
    const f64 arrivedHorizontal = std::sqrt(body.velocity.x * body.velocity.x + body.velocity.z * body.velocity.z);
    body.velocity.y -= kGravity * fixedDt_;
    if (body.windFactor != 0.0 && wind_.active()) {
      // A ball at rest is held by friction: the breeze can never start it
      // moving, which is what stops a still ball creeping across the course
      // forever. On the ground only the HORIZONTAL speed counts (a ball
      // settling under gravity is not "rolling").
      // Only the ground can hold a ball still — in the air there is nothing
      // to grip it, so anything airborne always catches the breeze.
      const bool resting = grounded && arrivedHorizontal <= kWindRestSpeed;
      if (!resting) {
        f64 accel = wind_.speed() * body.windFactor;
        if (grounded) {
          // The turf takes most of the breeze, and it can never supply more
          // push than the friction it is fighting — otherwise a strong
          // enough gale would accelerate a rolling ball for ever.
          // ... and never more than kWindGroundGrip of the friction the turf
          // is already supplying, so a rolling ball always still slows down
          // and stops: wind bends a putt, it never drives it for ever.
          const f64 frictionBudget = (body.friction + body.rollingFriction) * kGravity;
          accel = std::min(accel * kWindGroundFactor, frictionBudget * kWindGroundGrip);
        }
        const f64 windSpeed = wind_.speed();
        if (windSpeed > kEpsilon) {
          const f64 scale = accel * fixedDt_ / windSpeed;
          body.velocity.x += wind_.acceleration.x * scale;
          body.velocity.z += wind_.acceleration.z * scale;
        }
      }
    }
    // Magnus (stage 23): a spinning ball is pushed sideways by the air,
    // which is what bends a free kick. Only in the air — on the turf the
    // contact solver owns the ball. Spin decays as the air drags on it.
    if (body.magnusFactor != 0.0) {
      const f64 spinLength = body.spin.length();
      if (spinLength > kEpsilon) {
        if (!grounded) {
          const Vec3 magnus = cross(body.spin, body.velocity) * (kMagnusCoefficient * body.magnusFactor);
          body.velocity += magnus * fixedDt_;
          body.spin -= body.spin * (kSpinAirDecay * fixedDt_);
        } else {
          // Touching down scrubs the spin off against the ground.
          body.spin = body.spin * kSpinGroundKeep;
        }
      }
    }
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

// --- Character controller ---

namespace {

// True when the two AABBs strictly overlap on all three axes.
bool characterBoxOverlaps(const Vec3& aPosition, const Vec3& aHalf, const Vec3& bPosition,
                          const Vec3& bHalf) {
  return std::abs(aPosition.x - bPosition.x) < aHalf.x + bHalf.x &&
         std::abs(aPosition.y - bPosition.y) < aHalf.y + bHalf.y &&
         std::abs(aPosition.z - bPosition.z) < aHalf.z + bHalf.z;
}

// After an axis move, push the character out of a box along that axis and
// kill the velocity on it. axis: 0 = X, 1 = Y, 2 = Z.
void characterResolveAxis(CharacterBody& character, const Vec3& boxCenter, const Vec3& boxHalf,
                          i32 axis) {
  if (!characterBoxOverlaps(character.position, character.halfExtents, boxCenter, boxHalf)) return;
  f64* position = nullptr;
  f64 half = 0.0;
  f64 center = 0.0;
  f64 otherHalf = 0.0;
  f64* velocity = nullptr;
  if (axis == 0) {
    position = &character.position.x;
    half = character.halfExtents.x;
    center = boxCenter.x;
    otherHalf = boxHalf.x;
    velocity = &character.velocity.x;
  } else if (axis == 1) {
    position = &character.position.y;
    half = character.halfExtents.y;
    center = boxCenter.y;
    otherHalf = boxHalf.y;
    velocity = &character.velocity.y;
  } else {
    position = &character.position.z;
    half = character.halfExtents.z;
    center = boxCenter.z;
    otherHalf = boxHalf.z;
    velocity = &character.velocity.z;
  }
  *position = *position < center ? center - otherHalf - half : center + otherHalf + half;
  *velocity = 0.0;
  ++character.collisionCount;
}

}  // namespace

// Feet still resting on a surface? (a plane at the feet height, a box whose
// top face is under the character's center, or ANOTHER character's head —
// standing on a team-mate is legal, it just has to be stable.)
bool PhysicsWorld::characterSupported(const CharacterBody& character, u32 selfId) const {
  const f64 feet = character.position.y - character.halfExtents.y;
  for (const auto& pair : planes_) {
    if (std::abs(feet - pair.second.y) <= 1e-6) return true;
  }
  constexpr f64 kSupportTolerance = 1e-4;
  const auto supportedByBox = [&character, feet](const Vec3& center, const Vec3& half) {
    if (std::abs(feet - (center.y + half.y)) > kSupportTolerance) return false;
    if (std::abs(character.position.x - center.x) > half.x - kSupportTolerance) return false;
    if (std::abs(character.position.z - center.z) > half.z - kSupportTolerance) return false;
    return true;
  };
  for (const auto& pair : boxes_) {
    if (supportedByBox(pair.second.center, pair.second.halfExtents)) return true;
  }
  for (const auto& pair : dynamicBoxes_) {
    if (supportedByBox(pair.second.position, pair.second.halfExtents)) return true;
  }
  for (const auto& pair : characters_) {
    if (pair.first == selfId) continue;
    if (supportedByBox(pair.second.position, pair.second.halfExtents)) return true;
  }
  return false;
}

// --- Characters ---

u32 PhysicsWorld::addCharacter(const CharacterBody& body) {
  const u32 id = nextCharacterId_++;
  characters_[id] = body;
  return id;
}

bool PhysicsWorld::removeCharacter(u32 id) { return characters_.erase(id) > 0U; }

CharacterBody* PhysicsWorld::characterById(u32 id) {
  const auto it = characters_.find(id);
  return it == characters_.end() ? nullptr : &it->second;
}

const CharacterBody* PhysicsWorld::characterById(u32 id) const {
  const auto it = characters_.find(id);
  return it == characters_.end() ? nullptr : &it->second;
}

std::vector<u32> PhysicsWorld::characterIds() const {
  std::vector<u32> ids;
  ids.reserve(characters_.size());
  for (const auto& pair : characters_) ids.push_back(pair.first);  // std::map: ascending
  return ids;
}

void PhysicsWorld::resetCharacter(const Vec3& position) { resetCharacter(kPrimaryCharacter, position); }

void PhysicsWorld::resetCharacter(u32 id, const Vec3& position) {
  CharacterBody* character = characterById(id);
  if (character == nullptr) return;
  character->position = position;
  character->velocity = Vec3{0.0, 0.0, 0.0};
  character->onGround = false;
  character->collisionCount = 0U;
}

bool PhysicsWorld::characterJump(f64 height) { return characterJump(kPrimaryCharacter, height); }

bool PhysicsWorld::characterJump(u32 id, f64 height) {
  CharacterBody* character = characterById(id);
  if (character == nullptr || !character->onGround) return false;
  character->velocity.y = std::sqrt(2.0 * kGravity * height);
  character->onGround = false;
  return true;
}

void PhysicsWorld::moveCharacter(f64 dt, const Vec3& desiredVelocity) {
  moveCharacter(kPrimaryCharacter, dt, desiredVelocity);
}

void PhysicsWorld::moveCharacter(u32 id, f64 dt, const Vec3& desiredVelocity) {
  CharacterBody* self = characterById(id);
  if (self == nullptr) return;
  CharacterBody& character = *self;
  character.collisionCount = 0U;

  // Horizontal control is direct; gravity owns the vertical.
  character.velocity.x = desiredVelocity.x;
  character.velocity.z = desiredVelocity.z;
  if (!character.onGround) {
    character.velocity.y = std::max(character.velocity.y - kGravity * dt, -kMaxCharacterFallSpeed);
  }

  // Collide-and-slide: one axis at a time, so the character slides along
  // walls instead of sticking to corners.
  character.position.x += character.velocity.x * dt;
  for (const auto& pair : boxes_) {
    characterResolveAxis(character, pair.second.center, pair.second.halfExtents, 0);
  }
  for (const auto& pair : dynamicBoxes_) {
    characterResolveAxis(character, pair.second.position, pair.second.halfExtents, 0);
  }
  for (const auto& pair : characters_) {
    if (pair.first == id) continue;  // never collide with yourself
    characterResolveAxis(character, pair.second.position, pair.second.halfExtents, 0);
  }

  character.position.z += character.velocity.z * dt;
  for (const auto& pair : boxes_) {
    characterResolveAxis(character, pair.second.center, pair.second.halfExtents, 2);
  }
  for (const auto& pair : dynamicBoxes_) {
    characterResolveAxis(character, pair.second.position, pair.second.halfExtents, 2);
  }
  for (const auto& pair : characters_) {
    if (pair.first == id) continue;
    characterResolveAxis(character, pair.second.position, pair.second.halfExtents, 2);
  }

  // Vertical: land on top faces while falling, bump the head while rising.
  const bool falling = character.velocity.y <= 0.0;
  character.position.y += character.velocity.y * dt;
  if (falling) {
    for (const auto& pair : planes_) {
      if (character.position.y - character.halfExtents.y < pair.second.y) {
        character.position.y = pair.second.y + character.halfExtents.y;
        character.velocity.y = 0.0;
        character.onGround = true;
        ++character.collisionCount;
      }
    }
  }
  for (const auto& pair : boxes_) {
    characterResolveAxis(character, pair.second.center, pair.second.halfExtents, 1);
  }
  for (const auto& pair : dynamicBoxes_) {
    characterResolveAxis(character, pair.second.position, pair.second.halfExtents, 1);
  }
  for (const auto& pair : characters_) {
    if (pair.first == id) continue;
    characterResolveAxis(character, pair.second.position, pair.second.halfExtents, 1);
  }
  if (character.velocity.y == 0.0 && falling) character.onGround = true;
  // Standing still: verify the support is still there (walking off an edge
  // must drop the character).
  if (character.onGround && !characterSupported(character, id)) character.onGround = false;
}

// --- Raycasting (stage 30) ---

namespace {

// Slab test: where does a ray enter and leave an axis-aligned box?
// Returns false when it misses or the box is entirely behind the origin.
bool rayBox(const Vec3& origin, const Vec3& direction, const Vec3& center, const Vec3& halfExtents,
            f64 maxDistance, f64& outDistance, Vec3& outNormal) {
  f64 nearest = 0.0;
  f64 farthest = maxDistance;
  i32 nearAxis = 0;
  f64 nearSign = 0.0;

  const f64 o[3] = {origin.x, origin.y, origin.z};
  const f64 d[3] = {direction.x, direction.y, direction.z};
  const f64 c[3] = {center.x, center.y, center.z};
  const f64 h[3] = {halfExtents.x, halfExtents.y, halfExtents.z};

  for (i32 axis = 0; axis < 3; ++axis) {
    const f64 lo = c[axis] - h[axis];
    const f64 hi = c[axis] + h[axis];
    if (std::abs(d[axis]) < 1e-12) {
      // Parallel to this pair of faces: a miss unless we start between them.
      if (o[axis] < lo || o[axis] > hi) return false;
      continue;
    }
    const f64 inverse = 1.0 / d[axis];
    f64 t1 = (lo - o[axis]) * inverse;
    f64 t2 = (hi - o[axis]) * inverse;
    f64 sign = -1.0;
    if (t1 > t2) {
      const f64 swap = t1;
      t1 = t2;
      t2 = swap;
      sign = 1.0;
    }
    if (t1 > nearest) {
      nearest = t1;
      nearAxis = axis;
      nearSign = sign;
    }
    if (t2 < farthest) farthest = t2;
    if (nearest > farthest) return false;
  }
  outDistance = nearest;
  outNormal = Vec3{0.0, 0.0, 0.0};
  if (nearAxis == 0) outNormal.x = nearSign;
  else if (nearAxis == 1) outNormal.y = nearSign;
  else outNormal.z = nearSign;
  return true;
}

}  // namespace

PhysicsWorld::RayHit PhysicsWorld::raycast(const Vec3& origin, const Vec3& direction, f64 maxDistance,
                                           u32 ignoreCharacter) const {
  RayHit best;
  const f64 length = direction.length();
  if (length < 1e-12 || maxDistance <= 0.0) return best;
  const Vec3 ray = direction * (1.0 / length);

  const auto consider = [&best](f64 distance, const Vec3& point, const Vec3& normal, u32 character, u32 box,
                                bool ground) {
    if (best.hit && distance >= best.distance) return;
    best.hit = true;
    best.distance = distance;
    best.point = point;
    best.normal = normal;
    best.character = character;
    best.box = box;
    best.ground = ground;
  };

  // --- Static boxes: the walls and cover ---
  for (const auto& entry : boxes_) {
    f64 distance = 0.0;
    Vec3 normal{0.0, 0.0, 0.0};
    if (!rayBox(origin, ray, entry.second.center, entry.second.halfExtents, maxDistance, distance, normal)) {
      continue;
    }
    consider(distance, origin + ray * distance, normal, 0U, entry.first, false);
  }

  // --- Characters: the targets ---
  // A character is its axis-aligned box proxy, so the same slab test does.
  for (const auto& entry : characters_) {
    if (entry.first == ignoreCharacter) continue;  // never shoot yourself
    f64 distance = 0.0;
    Vec3 normal{0.0, 0.0, 0.0};
    if (!rayBox(origin, ray, entry.second.position, entry.second.halfExtents, maxDistance, distance, normal)) {
      continue;
    }
    consider(distance, origin + ray * distance, normal, entry.first, 0U, false);
  }

  // --- Ground planes ---
  if (std::abs(ray.y) > 1e-12) {
    for (const auto& entry : planes_) {
      const f64 distance = (entry.second.y - origin.y) / ray.y;
      if (distance < 0.0 || distance > maxDistance) continue;
      consider(distance, origin + ray * distance, Vec3{0.0, 1.0, 0.0}, 0U, 0U, true);
    }
  }
  return best;
}


}  // namespace kimia

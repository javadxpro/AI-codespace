#pragma once

#include <kimia/Time.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <map>
#include <vector>

namespace kimia {

// Gravity magnitude; pulls along -Y (acceleration vector is (0, -kGravity, 0)).
inline constexpr f64 kGravity = 9.81;

// Impacts slower than this (m/s) settle instead of bouncing: a body resting
// on a surface stops instead of micro-bouncing forever with high restitution.
inline constexpr f64 kContactRestitutionThreshold = 0.5;

// Dynamic sphere body. Semi-implicit (symplectic) Euler integration.
struct SphereBody {
  Vec3 position{0.0, 0.0, 0.0};
  Vec3 velocity{0.0, 0.0, 0.0};
  f64 radius = 0.12;
  f64 mass = 1.0;  // two-way impacts (e.g. ball vs dynamic box) use mass ratio
  f64 restitution = 0.40;
  f64 friction = 0.40;
  f64 rollingFriction = 0.22;
  u32 collisionCount = 0U;  // contacts resolved during the last step
};

// Dynamic axis-aligned box body (a crate): falls, slides, stacks and gets
// knocked around. No rotation — boxes stay axis-aligned. Semi-implicit
// Euler like the sphere; contacts are solved with iterative sequential
// impulses plus positional correction.
struct DynamicBox {
  Vec3 position{0.0, 0.0, 0.0};
  Vec3 halfExtents{0.5, 0.5, 0.5};
  Vec3 velocity{0.0, 0.0, 0.0};
  f64 mass = 1.0;
  f64 restitution = 0.25;
  f64 friction = 0.50;
  f64 rollingFriction = 0.05;  // extra tangential deceleration while sliding
  u32 collisionCount = 0U;     // contacts resolved during the last step
};

// Static plane at y = const (normal +Y).
struct StaticPlane {
  f64 y = 0.0;
};

// Static axis-aligned box.
struct StaticBox {
  Vec3 center{0.0, 0.0, 0.0};
  Vec3 halfExtents{0.5, 0.5, 0.5};
};

// Fixed-timestep physics world: dynamic spheres and dynamic boxes vs static
// planes and AABBs, plus dynamic-vs-dynamic pairs (sphere-sphere, sphere-box,
// box-box). Fixed dt = 1/120 s; host-rate advance() uses an accumulator with
// a step cap (default 5) so a slow frame cannot spiral. Bodies are referenced
// by 1-based ids (0 is null); ids are never reused.
class PhysicsWorld {
public:
  explicit PhysicsWorld(f64 fixedDt = 1.0 / 120.0, u32 maxStepsPerFrame = 5U);

  u32 addSphere(const SphereBody& body);
  bool removeSphere(u32 id);
  SphereBody* sphere(u32 id);
  const SphereBody* sphere(u32 id) const;

  u32 addDynamicBox(const DynamicBox& body);
  bool removeDynamicBox(u32 id);
  DynamicBox* dynamicBox(u32 id);
  const DynamicBox* dynamicBox(u32 id) const;

  u32 addPlane(f64 y);
  u32 addBox(const Vec3& center, const Vec3& halfExtents);

  // Highest Y (starting from center.y, capped at maxHeight) at which a sphere
  // of this radius at (center.x, ?, center.z) does NOT strictly overlap any
  // static or dynamic box. Used to spawn a ball safely above objects that
  // were placed on its spawn point. Returns the input y when nothing overlaps.
  f64 resolveSpawnHeight(const Vec3& center, f64 radius, f64 maxHeight) const;

  void clear();

  // One fixed step (fixedDt() seconds).
  void step();

  // Host-rate advance: accumulator-fed, capped at maxStepsPerFrame steps.
  // Returns how many fixed steps ran this frame.
  u32 advance(f64 hostSeconds);

  f64 fixedDt() const { return fixedDt_; }
  f64 time() const { return time_; }
  u64 stepCount() const { return steps_; }
  usize sphereCount() const { return spheres_.size(); }
  usize dynamicBoxCount() const { return dynamicBoxes_.size(); }
  usize planeCount() const { return planes_.size(); }
  usize boxCount() const { return boxes_.size(); }

private:
  struct Contact {
    bool sphereA = false;  // A is a dynamic sphere (else a dynamic box)
    bool sphereB = false;  // B is a dynamic sphere; false = dynamic box
    bool staticB = false;  // B is a static plane or box (A is never static)
    bool embedded = false; // sphere center inside a box: position-only fix
    u32 idA = 0U;
    u32 idB = 0U;
    Vec3 normal{0.0, 0.0, 0.0};  // points from A toward B
    f64 penetration = 0.0;
    f64 restitution = 0.0;
  };

  void collectContacts(std::vector<Contact>& contacts) const;
  void resolvePair(const Contact& contact, bool countContacts);
  void applyPairFriction(const Contact& contact);

  f64 fixedDt_;
  FixedTimeStep accumulator_;
  std::map<u32, SphereBody> spheres_;
  std::map<u32, DynamicBox> dynamicBoxes_;
  std::map<u32, StaticPlane> planes_;
  std::map<u32, StaticBox> boxes_;
  u32 nextId_ = 1U;
  f64 time_ = 0.0;
  u64 steps_ = 0U;
};

}  // namespace kimia

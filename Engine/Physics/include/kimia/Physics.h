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
  // How strongly the world wind pushes this body while it is airborne
  // (0 = immune, 1 = full). A heavy accurate ball can be tuned below 1.
  f64 windFactor = 1.0;
  // Spin, in radians per second, as an axis-angle vector (stage 23). A ball
  // spinning about the Y axis curls sideways; about X it dips or floats.
  // Only the Magnus force reads it — the renderer does not roll the ball.
  Vec3 spin{0.0, 0.0, 0.0};
  // How much Magnus lift this ball gets (0 = immune). A heavy accurate ball
  // curls less than a light fantasy one.
  f64 magnusFactor = 1.0;
  u32 collisionCount = 0U;  // contacts resolved during the last step
};

// Magnus: F/m = kMagnusCoefficient * (spin x velocity). Tuned so a 6 m/s
// shot with 12 rad/s of side spin bends about a third of a metre over a
// 10 m flight — a visible curl, not a boomerang.
inline constexpr f64 kMagnusCoefficient = 0.06;

// Spin bleeds away in flight with this time constant (per second), and a
// ball that touches down loses most of it at once: grass kills spin.
inline constexpr f64 kSpinAirDecay = 0.35;
inline constexpr f64 kSpinGroundKeep = 0.25;

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

// Kinematic character: an axis-aligned capsule proxy the caller drives.
// Gravity owns the vertical velocity; the caller supplies the desired
// horizontal velocity. moveCharacter() collides and slides the body along
// static planes/boxes and dynamic boxes, lands it on top faces (onGround)
// and bumps its head on ceilings. Dynamic boxes are pushed by the caller
// layer, not here — here they are solid obstacles.
struct CharacterBody {
  Vec3 position{0.0, 0.0, 0.0};  // center; feet at position.y - halfExtents.y
  Vec3 halfExtents{0.3, 0.5, 0.3};
  Vec3 velocity{0.0, 0.0, 0.0};
  bool onGround = false;
  u32 collisionCount = 0U;  // faces touched during the last move
  // Which side this character plays for (stage 21). 0 = no team (the lone
  // player of a sandbox world); 1 and 2 are the two sides of a match. The
  // physics layer only carries the number — the rules live in the game.
  u32 team = 0U;
};

// Falls faster than this are clamped so a slow frame cannot tunnel through
// a one-unit obstacle (6 m/s * 0.1 s = 0.6 m per host frame).
inline constexpr f64 kMaxCharacterFallSpeed = 6.0;

// Every world has a character with this id: the one the player drives.
inline constexpr u32 kPrimaryCharacter = 1U;

// --- Wind (stage 20.5-b2) ---
//
// A constant horizontal breeze blowing over the whole world. It is an
// ACCELERATION (m/s^2) applied to a dynamic sphere on every fixed step,
// exactly like gravity but sideways, scaled by the body's windFactor.
//
// Wind pushes a ball that is MOVING — in the air (a lofted football) or
// rolling along the ground (a putt drifting off line). A ball that has come
// to REST is immune: friction holds it, so a breeze can never creep a still
// ball across the course forever. On the ground the push is scaled by
// kWindGroundFactor, because the turf takes most of it.
//
// Wind is deterministic: the same wind and the same shot always land on the
// same spot, at any host frame rate.
//
// The vertical component is ignored: wind is horizontal by definition.
struct Wind {
  Vec3 acceleration{0.0, 0.0, 0.0};  // m/s^2, horizontal (y is ignored)

  bool active() const { return acceleration.x != 0.0 || acceleration.z != 0.0; }
  f64 speed() const;      // magnitude of the horizontal acceleration
  f64 direction() const;  // radians, 0 = blowing toward -Z, like the aim yaw
};

// A wind stronger than this is refused (clamped): beyond it a shot can no
// longer be aimed and the game stops being a game.
inline constexpr f64 kMaxWindAcceleration = 20.0;

// A ball slower than this (m/s) counts as at rest and ignores the wind.
inline constexpr f64 kWindRestSpeed = 0.05;
// How much of the wind reaches a ball that is touching the ground. The
// ground push is additionally capped at the friction deceleration the turf
// is already supplying, so no gale can accelerate a rolling ball for ever.
inline constexpr f64 kWindGroundFactor = 0.35;
// The hard ceiling on the ground push, as a fraction of the friction the
// surface supplies. Strictly below 1 so friction always wins in the end and
// a wind-blown ball comes to a stop instead of drifting for ever.
inline constexpr f64 kWindGroundGrip = 0.5;

// Builds a wind from a speed (m/s^2) and a direction (radians, 0 = toward -Z,
// matching WorldEditor::aimYaw). The speed is clamped to [0, kMaxWind...].
Wind makeWind(f64 speed, f64 direction);

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

  // --- Characters (stage 21: N of them) ---
  //
  // Characters have their OWN 1-based id space (they are kinematic, not
  // solver bodies, and keeping them separate leaves every sphere/box id
  // exactly where it was). Ids are never reused. Character 1 always
  // exists, so the single-character API below keeps working unchanged:
  // character() is character 1, and a world that never adds another one
  // behaves exactly as it did before.
  u32 addCharacter(const CharacterBody& body);
  bool removeCharacter(u32 id);
  CharacterBody* characterById(u32 id);
  const CharacterBody* characterById(u32 id) const;
  usize characterCount() const { return characters_.size(); }
  // The character ids in ascending (creation) order — a deterministic walk
  // for the game layer and for serialization.
  std::vector<u32> characterIds() const;

  // The first character (id kPrimaryCharacter). The one-player API.
  CharacterBody* character() { return characterById(kPrimaryCharacter); }
  const CharacterBody* character() const { return characterById(kPrimaryCharacter); }

  // The world wind (see Wind). Off by default, so every existing world and
  // every existing test behaves exactly as before.
  void setWind(const Wind& wind) { wind_ = wind; }
  const Wind& wind() const { return wind_; }

  // Teleports the character to `position`, zeroing velocity and ground state.
  void resetCharacter(const Vec3& position);
  void resetCharacter(u32 id, const Vec3& position);

  // Moves the character for dt seconds toward the desired horizontal
  // velocity (the y component is ignored — gravity owns the vertical).
  // Characters are solid to each other: a mover is blocked by, and slides
  // along, every OTHER character as well as the level geometry.
  void moveCharacter(f64 dt, const Vec3& desiredVelocity);
  void moveCharacter(u32 id, f64 dt, const Vec3& desiredVelocity);

  // Starts a jump of the given height (meters, feet apex) when the character
  // stands on something: v = sqrt(2 g h). Returns true when the jump began.
  bool characterJump(f64 height);
  bool characterJump(u32 id, f64 height);

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
  bool characterSupported(const CharacterBody& character, u32 selfId) const;

  f64 fixedDt_;
  FixedTimeStep accumulator_;
  std::map<u32, SphereBody> spheres_;
  std::map<u32, DynamicBox> dynamicBoxes_;
  std::map<u32, StaticPlane> planes_;
  std::map<u32, StaticBox> boxes_;
  std::map<u32, CharacterBody> characters_;
  u32 nextCharacterId_ = kPrimaryCharacter;
  Wind wind_;
  u32 nextId_ = 1U;
  f64 time_ = 0.0;
  u64 steps_ = 0U;
};

}  // namespace kimia

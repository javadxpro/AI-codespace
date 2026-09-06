#pragma once

#include <kimia/Entity.h>
#include <kimia/Quat.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace kimia {

struct Transform {
  Vec3 position{0.0, 0.0, 0.0};
  Vec3 scale{1.0, 1.0, 1.0};
  Quat rotation{};  // identity
};

// Mesh reference: primitive kind (scene v1) — later stages extend this with
// real mesh assets.
enum class MeshKind { cube, plane, sphere };

// --- Components and tags (stage 31) ---
//
// Until now an entity was a shape and a colour, and everything it could DO
// was decided by its name: "Ball" bounced, "Goal*" scored, "Crate_*" was
// pushable. That works for four built-in games and for nothing else — you
// could not import a model and make it a solid, animated, noisy thing.
//
// An entity now carries optional components instead. The name still works
// (every existing world loads unchanged), but a component overrides it.

// How an entity takes part in the simulation.
enum class BodyKind {
  None,     // decoration: drawn, never collided with
  Static,   // immovable solid: walls, cover, scenery
  Dynamic,  // pushed around: crates, props
  Sphere,   // rolling body: balls
};

// Physics component. Attached to anything that should be solid.
struct BodyComponent {
  BodyKind kind = BodyKind::None;
  f64 mass = 1.0;
  f64 friction = 0.4;
  f64 restitution = 0.3;   // bounciness, 0 = dead, 1 = perfectly elastic
  f64 radius = 0.0;        // Sphere only; 0 = derive from the transform
};

// Animation component: a clip from the entity's model, and the input that
// plays it. `trigger` is a key name ("j", "space") or one of the built-in
// events ("walk", "idle", "kick") the game drives itself.
struct AnimationComponent {
  std::string clip;     // clip name inside the FBX
  std::string trigger;  // key name or built-in event
  bool loop = true;
  f64 speed = 1.0;
};

// --- Rig component (stage 35) ---
//
// One bone of a character, as the editor edits it: a named segment with a
// start and an end, in the character's own space with the feet at y = 0.
// This is deliberately "from here to there" rather than a rotation and a
// length, because that is how a person describes a limb and how the
// Workbench lets you drag one.
struct RigBone {
  std::string name;    // "LeftLeg", "Tail", whatever the character needs
  std::string parent;  // parent bone's name ("" = a root)
  Vec3 from{0.0, 0.0, 0.0};
  Vec3 to{0.0, 0.0, 0.0};
  f64 thickness = 0.08;
  // Which way the bone swings when the character walks. 0 = still (a head
  // or a torso), 1 = a full stride, negative = opposite phase, which is
  // how the arms are made to swing against the legs.
  f64 swing = 0.0;
};

// Sound component: a registered sound name, played on the same kind of
// trigger as an animation.
struct SoundComponent {
  std::string sound;    // name registered with the server (/sfx/<name>)
  std::string trigger;
  f64 volume = 1.0;
};

struct EntityData {
  std::string name;
  Transform transform;
  MeshKind mesh = MeshKind::cube;
  // Model entities: a mesh asset file (OBJ/FBX) placed in the scene. Empty
  // for primitive entities. Rendering resolves this path at run time; the
  // primitive mesh acts as the fallback shape.
  std::string meshFile;
  Vec3 color{1.0, 1.0, 1.0};
  f64 roughness = 0.5;

  // --- Components (stage 31) ---
  // Free-form labels. A tag is how one object refers to a GROUP of others
  // without knowing their names: "goal", "cover", "enemy". The editor and
  // the game rules both look things up this way.
  std::vector<std::string> tags;
  // Optional components. Absent means "this entity does not do that".
  std::optional<BodyComponent> body;
  std::vector<AnimationComponent> animations;
  std::vector<SoundComponent> sounds;
  // A character's own bones (stage 35). Empty means "use the engine's
  // default figure", so nothing that worked before needs changing.
  std::vector<RigBone> rig;

  bool hasTag(const std::string& tag) const {
    for (const std::string& own : tags) {
      if (own == tag) return true;
    }
    return false;
  }
  void addTag(const std::string& tag) {
    if (!tag.empty() && !hasTag(tag)) tags.push_back(tag);
  }
  bool removeTag(const std::string& tag) {
    for (usize i = 0; i < tags.size(); ++i) {
      if (tags[i] != tag) continue;
      tags.erase(tags.begin() + static_cast<std::ptrdiff_t>(i));
      return true;
    }
    return false;
  }
};

// Player-authored demo shot stored in scene files as "# demo <aim> <power>".
struct DemoShot {
  f64 aim = 0.0;
  f64 power = 0.0;
};

// Entity container. Handles are 1-based and never reused; destroyed handles
// become null immediately (get/alive return null/false). Iteration is in
// handle order (deterministic, useful for stable serialization).
class Scene {
public:
  Scene() = default;
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  Scene(Scene&&) = default;
  Scene& operator=(Scene&&) = default;

  EntityHandle create(const std::string& name = "Entity");
  EntityHandle create(const EntityData& data);
  bool destroy(EntityHandle handle);

  EntityData* get(EntityHandle handle);
  const EntityData* get(EntityHandle handle) const;
  bool alive(EntityHandle handle) const;
  usize count() const { return entities_.size(); }

  // First entity with this name, or kNullEntity (iteration order stable).
  EntityHandle find(const std::string& name) const;

  void forEach(const std::function<void(EntityHandle, const EntityData&)>& callback) const;
  void clear();

  std::optional<DemoShot> demoShot;

private:
  std::map<EntityHandle, EntityData> entities_;
  u32 nextHandle_ = 1U;
};

}  // namespace kimia

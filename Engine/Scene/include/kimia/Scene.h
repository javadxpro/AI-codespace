#pragma once

#include <kimia/Entity.h>
#include <kimia/Quat.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace kimia {

struct Transform {
  Vec3 position{0.0, 0.0, 0.0};
  Vec3 scale{1.0, 1.0, 1.0};
  Quat rotation{};  // identity
};

// Mesh reference: primitive kind (scene v1) — later stages extend this with
// real mesh assets.
enum class MeshKind { cube, plane, sphere };

struct EntityData {
  std::string name;
  Transform transform;
  MeshKind mesh = MeshKind::cube;
  Vec3 color{1.0, 1.0, 1.0};
  f64 roughness = 0.5;
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

  void forEach(const std::function<void(EntityHandle, const EntityData&)>& callback) const;
  void clear();

  std::optional<DemoShot> demoShot;

private:
  std::map<EntityHandle, EntityData> entities_;
  u32 nextHandle_ = 1U;
};

}  // namespace kimia

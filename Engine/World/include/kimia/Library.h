#pragma once

#include <kimia/Scene.h>
#include <kimia/Types.h>

#include <string>
#include <vector>

namespace kimia {

// --- Blueprints and stages (the project's reusable parts) ---
//
// Two things a person needs before they can build a real game rather than
// a single room:
//
//   * a BLUEPRINT — "this enemy, with its physics, tags and sounds
//     already set up" — so it can be stamped down twenty times without
//     twenty rounds of the same form-filling;
//   * a STAGE — a whole scene — so a game can be a menu, a level and a
//     victory screen instead of one endless field.
//
// Both are deliberately plain data: a blueprint IS an EntityData, and a
// stage IS a Scene. Nothing here invents a new object model, which means
// everything that already works on an entity works on a blueprint too.

// A saved object, ready to be stamped into a scene.
struct Blueprint {
  std::string name;   // what the user called it, e.g. "Barrel"
  EntityData entity;  // the object itself, exactly as it was
};

// One scene of a game, with the name the rules refer to it by.
struct Stage {
  std::string name = "Main";
  Scene scene;
};

// Everything a project keeps besides the scene being edited.
struct Library {
  std::vector<Blueprint> blueprints;
  std::vector<Stage> stages;

  const Blueprint* findBlueprint(const std::string& name) const;
  const Stage* findStage(const std::string& name) const;
  Stage* findStage(const std::string& name);

  // Saves `entity` as a blueprint under `name`, replacing any blueprint
  // of the same name. Re-saving is how a person edits one.
  void keep(const std::string& name, const EntityData& entity);
  bool forget(const std::string& name);

  // A blueprint stamped into a scene: the same object, given a fresh
  // unique name and a position. Returns the name it was given, or empty
  // when there is no such blueprint.
  std::string stamp(const std::string& blueprintName, Scene& into, const Vec3& at) const;
};

// A name nothing in `scene` is using yet, based on `wanted`: "Barrel",
// then "Barrel_2", "Barrel_3"... Two copies of a thing must be two
// objects, not one object the rules cannot tell apart.
std::string uniqueName(const Scene& scene, const std::string& wanted);

}  // namespace kimia

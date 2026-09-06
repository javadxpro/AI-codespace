#include <kimia/Library.h>

namespace kimia {

const Blueprint* Library::findBlueprint(const std::string& name) const {
  for (const Blueprint& blueprint : blueprints) {
    if (blueprint.name == name) return &blueprint;
  }
  return nullptr;
}

const Stage* Library::findStage(const std::string& name) const {
  for (const Stage& stage : stages) {
    if (stage.name == name) return &stage;
  }
  return nullptr;
}

Stage* Library::findStage(const std::string& name) {
  for (Stage& stage : stages) {
    if (stage.name == name) return &stage;
  }
  return nullptr;
}

void Library::keep(const std::string& name, const EntityData& entity) {
  if (name.empty()) return;
  for (Blueprint& blueprint : blueprints) {
    if (blueprint.name != name) continue;
    // Saving over a blueprint is how you edit one, so this replaces
    // rather than making a second with the same name.
    blueprint.entity = entity;
    return;
  }
  Blueprint fresh;
  fresh.name = name;
  fresh.entity = entity;
  blueprints.push_back(fresh);
}

bool Library::forget(const std::string& name) {
  for (usize i = 0; i < blueprints.size(); ++i) {
    if (blueprints[i].name != name) continue;
    blueprints.erase(blueprints.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
  }
  return false;
}

std::string uniqueName(const Scene& scene, const std::string& wanted) {
  const std::string base = wanted.empty() ? std::string("Object") : wanted;
  if (scene.find(base) == kNullEntity) return base;
  u32 index = 2U;
  for (;;) {
    const std::string candidate = base + "_" + std::to_string(index);
    if (scene.find(candidate) == kNullEntity) return candidate;
    ++index;
  }
}

std::string Library::stamp(const std::string& blueprintName, Scene& into, const Vec3& at) const {
  const Blueprint* blueprint = findBlueprint(blueprintName);
  if (blueprint == nullptr) return std::string();
  EntityData copy = blueprint->entity;
  // Everything else — physics, tags, sounds, bones — comes along, because
  // that is the entire point of saving one.
  copy.name = uniqueName(into, blueprint->name);
  copy.transform.position = at;
  into.create(copy);
  return copy.name;
}

}  // namespace kimia

#include <kimia/Scene.h>

namespace kimia {

EntityHandle Scene::create(const std::string& name) {
  EntityData data;
  data.name = name;
  return create(data);
}

EntityHandle Scene::create(const EntityData& data) {
  const EntityHandle handle = nextHandle_;
  ++nextHandle_;
  if (nextHandle_ == kNullEntity) ++nextHandle_;  // never wrap onto null
  entities_.emplace(handle, data);
  return handle;
}

bool Scene::destroy(EntityHandle handle) { return entities_.erase(handle) > 0U; }

EntityData* Scene::get(EntityHandle handle) {
  const auto found = entities_.find(handle);
  return found == entities_.end() ? nullptr : &found->second;
}

const EntityData* Scene::get(EntityHandle handle) const {
  const auto found = entities_.find(handle);
  return found == entities_.end() ? nullptr : &found->second;
}

bool Scene::alive(EntityHandle handle) const { return entities_.find(handle) != entities_.end(); }

void Scene::forEach(const std::function<void(EntityHandle, const EntityData&)>& callback) const {
  for (const auto& [handle, entity] : entities_) {
    callback(handle, entity);
  }
}

void Scene::clear() { entities_.clear(); }

}  // namespace kimia

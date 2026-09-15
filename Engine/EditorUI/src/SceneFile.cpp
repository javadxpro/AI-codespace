// SceneFile implementation — see SceneFile.h.
#include <kimia/SceneFile.h>
#include <kimia/World.h>

namespace kimia::ui {

SceneFileResult saveScene(::kimia::WorldEditor& editor,
                          const std::string& path) {
  SceneFileResult r;
  if (path.empty()) {
    r.status = SceneFileStatus::NoPath;
    r.message = "No scene path; use 'Save As' first.";
    return r;
  }
  std::string err;
  if (!editor.saveWorld(path, err)) {
    r.status = SceneFileStatus::SaveFailed;
    r.message = err.empty() ? "saveWorld returned false" : err;
    return r;
  }
  r.status = SceneFileStatus::Ok;
  r.message = "Saved to " + path;
  return r;
}

SceneFileResult loadScene(::kimia::WorldEditor& editor,
                          const std::string& path) {
  SceneFileResult r;
  if (path.empty()) {
    r.status = SceneFileStatus::NoPath;
    r.message = "No scene path provided.";
    return r;
  }
  std::string err;
  if (!editor.loadWorld(path, err)) {
    r.status = SceneFileStatus::LoadFailed;
    r.message = err.empty() ? "loadWorld returned false" : err;
    return r;
  }
  r.status = SceneFileStatus::Ok;
  r.message = "Loaded " + path;
  return r;
}

}  // namespace kimia::ui

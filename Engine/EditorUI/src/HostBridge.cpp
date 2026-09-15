// HostBridge implementation.
#include <kimia/HostBridge.h>
#include <kimia/World.h>

namespace kimia::ui {

namespace {

// Scene snapshot is a flat per-frame copy of what the engine knows about
// the world. Reads through WorldEditor are O(entities), which is fine for
// Phase 1; Phase 2 will replace this with a delta-only path so the
// editor can scale to large worlds.
SceneSnapshot snapshotFromImpl(WorldEditor* editor) {
  SceneSnapshot snap;
  if (editor == nullptr) return snap;
  snap.valid = true;
  snap.playing = editor->playing();
  snap.paused = editor->paused();
  snap.scenePath = editor->worldPath();
  for (const std::string& name : editor->entityNames()) {
    EntityRef e;
    e.name = name;
    if (const EntityData* data = editor->world().scene.get(
            editor->world().scene.find(name))) {
      e.posX = static_cast<float>(data->transform.position.x);
      e.posY = static_cast<float>(data->transform.position.y);
      e.posZ = static_cast<float>(data->transform.position.z);
      e.scaleX = static_cast<float>(data->transform.scale.x);
      e.scaleY = static_cast<float>(data->transform.scale.y);
      e.scaleZ = static_cast<float>(data->transform.scale.z);
      const Quat& q = data->transform.rotation;
      e.rotX = static_cast<float>(q.x);
      e.rotY = static_cast<float>(q.y);
      e.rotZ = static_cast<float>(q.z);
      e.rotW = static_cast<float>(q.w);
      e.colorR = static_cast<float>(data->color.x);
      e.colorG = static_cast<float>(data->color.y);
      e.colorB = static_cast<float>(data->color.z);
      e.rough = static_cast<float>(data->roughness);
      e.metal = static_cast<float>(data->metallic);
      e.locked = (name == "Player" || name == "Ball" || name == "Ground");
    }
    snap.entities.push_back(std::move(e));
  }
  const std::string& sel = editor->selectedName();
  if (!sel.empty()) snap.selectedNames.push_back(sel);
  return snap;
}

bool applyCommandImpl(WorldEditor* editor, const UiCommand& cmd) {
  if (editor == nullptr) return false;
  switch (cmd.kind) {
    case UiCommandKind::None:
      return false;
    case UiCommandKind::SelectEntity:
      editor->selectEntity(cmd.name);
      return true;
    case UiCommandKind::ClearSelection:
      editor->selectEntity(std::string());
      return true;
    case UiCommandKind::SetPosition:
      // setEntityTransform keeps the existing scale; we only change position.
      // (For Phase 1 the UI only edits position via the vec3 field.)
      if (const EntityData* data = editor->world().scene.get(
              editor->world().scene.find(cmd.name))) {
        return editor->setEntityTransform(
            cmd.name, Vec3{cmd.x, cmd.y, cmd.z}, data->transform.scale);
      }
      return false;
    case UiCommandKind::SetColor:
      return editor->setEntityColor(
          cmd.name, Vec3{cmd.r, cmd.g, cmd.b});
    case UiCommandKind::DeleteSelected: {
      const std::string& n = editor->selectedName();
      if (n.empty()) return false;
      return editor->deleteEntity(n);
    }
    case UiCommandKind::CreateCube:
    case UiCommandKind::CreateSphere:
    case UiCommandKind::CreatePlane: {
      const char* kind = cmd.kind == UiCommandKind::CreateCube ? "cube"
                       : cmd.kind == UiCommandKind::CreateSphere ? "sphere"
                                                                  : "plane";
      editor->createObject(kind, Vec3{0.0, 0.0, 0.0});
      return true;
    }
    case UiCommandKind::PlayPressed:
      if (!editor->playing()) editor->enterPlayMode();
      return true;
    case UiCommandKind::PausePressed:
      editor->setPaused(!editor->paused());
      return true;
    case UiCommandKind::StepPressed:
      if (!editor->playing()) editor->enterPlayMode();
      editor->stepOnce(1.0 / 60.0);
      return true;
    case UiCommandKind::StopPressed:
      // Phase 1 doesn't track a separate Edit/Play screen; ignored.
      return false;
    default:
      return false;
  }
}

}  // namespace

SceneSnapshot snapshotFrom(WorldEditor* editor) {
  return snapshotFromImpl(editor);
}

bool applyCommand(WorldEditor* editor, const UiCommand& cmd) {
  return applyCommandImpl(editor, cmd);
}

void applyCommands(WorldEditor* editor, const std::vector<UiCommand>& cmds) {
  for (const UiCommand& c : cmds) {
    applyCommandImpl(editor, c);
  }
}

}  // namespace kimia::ui

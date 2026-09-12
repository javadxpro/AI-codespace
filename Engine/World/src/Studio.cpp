#include <kimia/AssetPipeline.h>
#include <kimia/Assets.h>
#include <kimia/MathUtils.h>
#include <kimia/Studio.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace kimia {
namespace studio {

namespace {

// --- Tiny JSON writing ---
// The engine only ever emits objects, arrays, strings and numbers, so a
// full JSON library would be a dependency bought for nothing.

std::string escape(const std::string& text) {
  std::string out;
  out.reserve(text.size() + 8U);
  for (const char c : text) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        // Control characters would make the JSON invalid.
        if (static_cast<unsigned char>(c) < 0x20U) {
          char buffer[8];
          std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(c) & 0xFFU);
          out += buffer;
        } else {
          out += c;
        }
    }
  }
  return out;
}

std::string quoted(const std::string& text) { return "\"" + escape(text) + "\""; }

std::string number(f64 value) {
  std::ostringstream stream;
  stream.precision(6);
  stream << std::fixed << value;
  return stream.str();
}

std::string vec3Json(const Vec3& v) {
  return "[" + number(v.x) + "," + number(v.y) + "," + number(v.z) + "]";
}

std::string stringsJson(const std::vector<std::string>& values) {
  std::string out = "[";
  for (usize i = 0; i < values.size(); ++i) {
    if (i > 0U) out += ",";
    out += quoted(values[i]);
  }
  return out + "]";
}

std::string errorJson(const std::string& why) { return "{\"ok\":false,\"error\":" + quoted(why) + "}"; }
std::string okJson() { return "{\"ok\":true}"; }
std::string okJson(const std::string& field, const std::string& value) {
  return "{\"ok\":true," + quoted(field) + ":" + quoted(value) + "}";
}

// --- Parameter helpers ---
// A missing or malformed parameter falls back rather than failing: a UI
// that forgets a field should get a sensible object, not an error page.

std::string param(const std::map<std::string, std::string>& params, const std::string& key,
                  const std::string& fallback = std::string()) {
  const auto found = params.find(key);
  return found == params.end() ? fallback : found->second;
}

f64 numberParam(const std::map<std::string, std::string>& params, const std::string& key, f64 fallback) {
  const auto found = params.find(key);
  if (found == params.end() || found->second.empty()) return fallback;
  try {
    return std::stod(found->second);
  } catch (...) {
    return fallback;
  }
}

bool flagParam(const std::map<std::string, std::string>& params, const std::string& key, bool fallback) {
  const auto found = params.find(key);
  if (found == params.end()) return fallback;
  return found->second == "1" || found->second == "true" || found->second == "on";
}

const char* bodyKindText(BodyKind kind) {
  switch (kind) {
    case BodyKind::Static: return "static";
    case BodyKind::Dynamic: return "dynamic";
    case BodyKind::Sphere: return "sphere";
    case BodyKind::None: break;
  }
  return "none";
}

BodyKind bodyKindFrom(const std::string& text) {
  if (text == "static") return BodyKind::Static;
  if (text == "dynamic") return BodyKind::Dynamic;
  if (text == "sphere") return BodyKind::Sphere;
  return BodyKind::None;
}

// How big the thing actually is in the world, in meters. For a built-in
// shape that is just its scale; for an imported model it is the file's
// own size multiplied by the transform.
f64 entitySpan(const EntityData& entity, const std::string& resolvedAssetPath) {
  const Vec3& s = entity.transform.scale;
  f64 largest = std::max(std::abs(s.x), std::max(std::abs(s.y), std::abs(s.z)));
  if (entity.meshFile.empty()) return largest;
  std::string error;
  auto loaded = assets::loadMesh(resolvedAssetPath, error);
  // A bare rig (an animation-only FBX) has no vertices; its rest-pose
  // joints measure it instead.
  std::vector<Vec3> rigJoints;
  if (!loaded.has_value() || loaded->mesh.positions.empty()) {
    std::string rigError;
    auto rigged = assets::loadFBXSkinned(resolvedAssetPath, rigError);
    if (rigged.has_value() && !rigged->skinned.skeleton.isEmpty()) {
      rigJoints = restJointPositions(rigged->skinned.skeleton);
    }
  }
  const bool bareRig = !rigJoints.empty();
  if (!bareRig && (!loaded.has_value() || loaded->mesh.positions.empty())) return largest;
  const std::vector<Vec3>& points = bareRig ? rigJoints : loaded->mesh.positions;
  Vec3 lo = points[0];
  Vec3 hi = lo;
  for (const Vec3& p : points) {
    lo.x = std::min(lo.x, p.x);
    lo.y = std::min(lo.y, p.y);
    lo.z = std::min(lo.z, p.z);
    hi.x = std::max(hi.x, p.x);
    hi.y = std::max(hi.y, p.y);
    hi.z = std::max(hi.z, p.z);
  }
  const f64 raw = std::max(hi.x - lo.x, std::max(hi.y - lo.y, hi.z - lo.z));
  return raw * largest;
}

// One object's full Dossier, as the panel shows it.
std::string dossierJson(const EntityData& entity, const Vec3& rotationDegrees,
                        const std::string& resolvedAssetPath) {
  std::string out = "{";
  out += "\"name\":" + quoted(entity.name);
  out += ",\"mesh\":" + quoted(entity.meshFile);
  out += ",\"position\":" + vec3Json(entity.transform.position);
  out += ",\"rotation\":" + vec3Json(rotationDegrees);
  out += ",\"scale\":" + vec3Json(entity.transform.scale);
  // A raw scale multiplier is meaningless for an imported model: after
  // bring-in auto-fits the file, "3" means three times the original, not
  // three units across. The Bench shows this measured size instead.
  out += ",\"span\":" + number(entitySpan(entity, resolvedAssetPath));
  out += ",\"color\":" + vec3Json(entity.color);
  out += ",\"labels\":" + stringsJson(entity.tags);

  out += ",\"body\":";
  if (entity.body.has_value()) {
    const BodyComponent& body = *entity.body;
    out += "{\"kind\":" + quoted(bodyKindText(body.kind));
    out += ",\"mass\":" + number(body.mass);
    out += ",\"friction\":" + number(body.friction);
    out += ",\"bounce\":" + number(body.restitution);
    out += ",\"radius\":" + number(body.radius) + "}";
  } else {
    out += "null";
  }

  out += ",\"motions\":[";
  for (usize i = 0; i < entity.animations.size(); ++i) {
    const AnimationComponent& clip = entity.animations[i];
    if (i > 0U) out += ",";
    out += "{\"clip\":" + quoted(clip.clip);
    out += ",\"wiring\":" + quoted(clip.trigger);
    out += ",\"loop\":" + std::string(clip.loop ? "true" : "false");
    out += ",\"speed\":" + number(clip.speed) + "}";
  }
  out += "]";

  // The character's own bones, so the Bench can list and drag them.
  out += ",\"bones\":[";
  for (usize i = 0; i < entity.rig.size(); ++i) {
    const RigBone& bone = entity.rig[i];
    if (i > 0U) out += ",";
    out += "{\"name\":" + quoted(bone.name);
    out += ",\"parent\":" + quoted(bone.parent);
    out += ",\"from\":" + vec3Json(bone.from);
    out += ",\"to\":" + vec3Json(bone.to);
    out += ",\"thickness\":" + number(bone.thickness);
    out += ",\"swing\":" + number(bone.swing) + "}";
  }
  out += "]";

  out += ",\"noises\":[";
  for (usize i = 0; i < entity.sounds.size(); ++i) {
    const SoundComponent& sound = entity.sounds[i];
    if (i > 0U) out += ",";
    out += "{\"sound\":" + quoted(sound.sound);
    out += ",\"wiring\":" + quoted(sound.trigger);
    out += ",\"volume\":" + number(sound.volume) + "}";
  }
  out += "]";
  return out + "}";
}

// A name the browser may touch on disk: bare, no folders, no tricks.
bool plainFileName(const std::string& name) {
  if (name.empty() || name.size() > 255U) return false;
  if (name == "." || name == "..") return false;
  for (const char c : name) {
    if (c == '/' || c == '\\' || c == '\0') return false;
  }
  return true;
}

// A path the browser may touch UNDER the import folder: "actions/Kick.fbx"
// is fine, but every segment must be plain, so "..", absolute paths and
// backslashes never reach the filesystem. The editor joins it onto the
// import folder itself.
bool safeAssetPath(const std::string& path) {
  if (path.empty() || path.size() > 1024U) return false;
  if (path.front() == '/' || path.front() == '\\') return false;
  usize begin = 0U;
  for (;;) {
    const usize slash = path.find('/', begin);
    const std::string part = path.substr(begin, slash == std::string::npos ? slash : slash - begin);
    if (!plainFileName(part)) return false;
    if (slash == std::string::npos) return true;
    begin = slash + 1U;
  }
}

bool fileExists(const std::string& path) {
  if (std::FILE* found = std::fopen(path.c_str(), "rb")) {
    std::fclose(found);
    return true;
  }
  return false;
}

}  // namespace

std::string handleApi(WorldEditor& editor, const std::string& path,
                      const std::map<std::string, std::string>& params) {
  // --- Reading the world ---

  // The Rack: everything in the world, with just enough per row to draw a
  // list without a second request each.
  if (path == "/api/rack") {
    if (!editor.hasWorld()) return "{\"ok\":true,\"world\":null,\"items\":[]}";
    std::string out = "{\"ok\":true,\"world\":" + quoted(editor.profile().title);
    out += ",\"game\":" + quoted(editor.profile().name);
    out += ",\"labels\":" + stringsJson(editor.allTags());
    out += ",\"items\":[";
    const std::vector<std::string> names = editor.entityNames();
    for (usize i = 0; i < names.size(); ++i) {
      const EntityData* entity = editor.entity(names[i]);
      if (entity == nullptr) continue;
      if (i > 0U) out += ",";
      out += "{\"name\":" + quoted(entity->name);
      out += ",\"body\":" + quoted(entity->body.has_value() ? bodyKindText(entity->body->kind) : "");
      out += ",\"labels\":" + std::to_string(entity->tags.size());
      out += ",\"motions\":" + std::to_string(entity->animations.size());
      out += ",\"noises\":" + std::to_string(entity->sounds.size());
      out += ",\"imported\":" + std::string(entity->meshFile.empty() ? "false" : "true") + "}";
    }
    return out + "]}";
  }

  // One object's Dossier.
  if (path == "/api/dossier") {
    const EntityData* entity = editor.entity(param(params, "name"));
    if (entity == nullptr) return errorJson("no such object");
    return "{\"ok\":true,\"dossier\":" +
           dossierJson(*entity, editor.entityEulerDegrees(entity->name), editor.assetPath(entity->meshFile)) + "}";
  }

  // Everything carrying a label — the point of labels is addressing a
  // GROUP, so the Bench has to be able to show that group.
  if (path == "/api/labelled") {
    return "{\"ok\":true,\"items\":" + stringsJson(editor.entitiesWithTag(param(params, "label"))) + "}";
  }

  // --- The game's own interface ---

  if (path == "/api/panels") {
    std::string out = "{\"ok\":true,\"panels\":[";
    const HudLayout& hud = editor.hud();
    for (usize i = 0; i < hud.panels.size(); ++i) {
      const Panel& panel = hud.panels[i];
      if (i > 0U) out += ",";
      out += "{\"name\":" + quoted(panel.name);
      out += ",\"kind\":" + quoted(panelKindName(panel.kind));
      out += ",\"visible\":" + std::string(panel.visible ? "true" : "false");
      out += ",\"x\":" + number(panel.x) + ",\"y\":" + number(panel.y);
      out += ",\"w\":" + number(panel.width) + ",\"h\":" + number(panel.height);
      out += ",\"text\":" + quoted(panel.text);
      out += ",\"variable\":" + quoted(panel.variable);
      out += ",\"maximum\":" + number(panel.maximum);
      out += ",\"event\":" + quoted(panel.event);
      out += ",\"color\":" + vec3Json(panel.color);
      out += ",\"background\":" + vec3Json(panel.background);
      out += ",\"opacity\":" + number(panel.opacity);
      out += ",\"scale\":" + std::to_string(panel.scale) + "}";
    }
    return out + "]}";
  }

  // Add or move a panel. "Set" rather than "add" because dragging one in
  // the editor calls this over and over with the same name.
  if (path == "/api/set-panel") {
    Panel panel;
    panel.name = param(params, "panel");
    if (panel.name.empty()) return errorJson("a panel needs a name");
    if (!panelKindFromName(param(params, "kind", "label"), panel.kind)) panel.kind = PanelKind::Label;
    panel.visible = flagParam(params, "visible", true);
    panel.x = numberParam(params, "x", 0.02);
    panel.y = numberParam(params, "y", 0.02);
    panel.width = numberParam(params, "w", 0.3);
    panel.height = numberParam(params, "h", 0.06);
    panel.text = param(params, "text");
    panel.variable = param(params, "variable");
    panel.maximum = numberParam(params, "maximum", 100.0);
    panel.event = param(params, "event");
    panel.color = Vec3{numberParam(params, "r", 0.9), numberParam(params, "g", 0.9),
                       numberParam(params, "b", 0.95)};
    panel.background = Vec3{numberParam(params, "br", 0.1), numberParam(params, "bg", 0.12),
                            numberParam(params, "bb", 0.15)};
    panel.opacity = numberParam(params, "opacity", 0.75);
    panel.scale = static_cast<i32>(numberParam(params, "scale", 2.0));
    if (!editor.setPanel(panel)) return errorJson("could not set that panel");
    return okJson();
  }

  if (path == "/api/drop-panel") {
    if (!editor.removePanel(param(params, "panel"))) return errorJson("no such panel");
    return okJson();
  }

  // Press a button by name, to check the wiring without playing.
  if (path == "/api/press") {
    const Panel* panel = editor.hud().find(param(params, "panel"));
    if (panel == nullptr) return errorJson("no such panel");
    if (panel->kind != PanelKind::Button) return errorJson("that panel is not a button");
    const std::string hit = editor.pressHudAt(1000, 1000, (panel->x + panel->width * 0.5) * 1000.0,
                                              (panel->y + panel->height * 0.5) * 1000.0);
    if (hit.empty()) return errorJson("the press missed");
    return okJson("pressed", hit);
  }

  // --- The asset folder: what the user dropped in ---

  // Lists the folder and, when asked, looks inside each model to find
  // its skeleton and clips. Deep is opt-in because opening every FBX is
  // slow enough to notice on a phone.
  if (path == "/api/assets") {
    const bool deep = flagParam(params, "deep", false);
    const std::vector<assetscan::ScannedAsset> found =
        assetscan::scan(param(params, "folder", editor.importDirectory()), deep);
    std::string out = "{\"ok\":true,\"deep\":" + std::string(deep ? "true" : "false");
    out += ",\"assets\":[";
    for (usize i = 0; i < found.size(); ++i) {
      const assetscan::ScannedAsset& asset = found[i];
      if (i > 0U) out += ",";
      out += "{\"file\":" + quoted(asset.file);
      // The browser must never save or echo the server's absolute path. The
      // relative asset name is stable in a .kimia world and in a package.
      out += ",\"path\":" + quoted(asset.file);
      out += ",\"kind\":" + quoted(assetscan::assetKindName(asset.kind));
      out += ",\"bytes\":" + std::to_string(asset.bytes);
      out += ",\"skeleton\":" + std::string(asset.hasSkeleton ? "true" : "false");
      out += ",\"bones\":" + std::to_string(asset.boneCount);
      out += ",\"clips\":" + stringsJson(asset.clips);
      out += ",\"note\":" + quoted(asset.note) + "}";
    }
    return out + "]}";
  }

  // Paint an image from the folder onto an object.
  if (path == "/api/skin") {
    const std::string name = param(params, "name");
    const std::string image = param(params, "image");
    if (image.empty()) {
      if (!editor.clearEntityTexture(name)) return errorJson("nothing to remove");
      return okJson();
    }
    if (!editor.setEntityTexture(name, image)) return errorJson("could not use that image");
    return okJson();
  }

  // --- Controls: one action, many ways to do it ---

  if (path == "/api/controls") {
    const InputMap& map = editor.input();
    std::string out = "{\"ok\":true,\"stick\":" + std::string(map.showStick ? "true" : "false");
    out += ",\"controls\":[";
    for (usize i = 0; i < map.controls.size(); ++i) {
      const Control& control = map.controls[i];
      if (i > 0U) out += ",";
      out += "{\"name\":" + quoted(control.name);
      out += ",\"label\":" + quoted(control.spot.label);
      out += ",\"x\":" + number(control.spot.x) + ",\"y\":" + number(control.spot.y);
      out += ",\"size\":" + number(control.spot.size);
      out += ",\"clipFile\":" + quoted(control.clipFile);
      out += ",\"clip\":" + quoted(control.clip);
      out += ",\"target\":" + quoted(control.target);
      out += ",\"sound\":" + quoted(control.sound);
      out += ",\"bindings\":[";
      for (usize b = 0; b < control.bindings.size(); ++b) {
        if (b > 0U) out += ",";
        out += "{\"source\":" + quoted(sourceName(control.bindings[b].source));
        out += ",\"code\":" + quoted(control.bindings[b].code) + "}";
      }
      out += "]}";
    }
    return out + "]}";
  }

  // Add or edit a control, with its bindings in one go: "jump" is the
  // space key AND an on-screen button AND the pad's A, all at once.
  if (path == "/api/set-control") {
    Control control;
    control.name = param(params, "control");
    if (control.name.empty()) return errorJson("a control needs a name");
    control.spot.label = param(params, "label", control.name);
    control.spot.x = numberParam(params, "x", 0.85);
    control.spot.y = numberParam(params, "y", 0.8);
    control.spot.size = numberParam(params, "size", 0.12);
    control.clipFile = param(params, "clipfile");
    control.clip = param(params, "clip");
    control.target = param(params, "target");
    control.sound = param(params, "sound");

    const std::string key = param(params, "key");
    if (!key.empty()) {
      Binding binding;
      binding.source = Source::Key;
      binding.code = key;
      control.bindings.push_back(binding);
    }
    if (flagParam(params, "touch", false)) {
      Binding binding;
      binding.source = Source::Touch;
      binding.code = control.name;
      control.bindings.push_back(binding);
    }
    const std::string pad = param(params, "pad");
    if (!pad.empty()) {
      Binding binding;
      binding.source = Source::Pad;
      binding.code = pad;
      control.bindings.push_back(binding);
    }
    if (!editor.setControl(control)) return errorJson("could not set that control");
    return okJson();
  }

  if (path == "/api/drop-control") {
    if (!editor.removeControl(param(params, "control"))) return errorJson("no such control");
    return okJson();
  }
  if (path == "/api/stick") {
    editor.setShowStick(flagParam(params, "on", true));
    return okJson();
  }
  // Fire a control from the Bench to check its clip and sound.
  if (path == "/api/do") {
    if (!editor.fireControl(param(params, "control"))) return errorJson("no such control");
    const std::string& warning = editor.lastAnimationError();
    if (warning.empty()) return okJson();
    return "{\"ok\":true,\"warning\":" + quoted(warning) + "}";
  }

  // --- Particles ---

  if (path == "/api/effects") {
    std::string out = "{\"ok\":true,\"live\":" + std::to_string(editor.particles().count());
    out += ",\"effects\":[";
    const EmitterBook& book = editor.emitters();
    for (usize i = 0; i < book.emitters.size(); ++i) {
      const Emitter& emitter = book.emitters[i];
      if (i > 0U) out += ",";
      out += "{\"name\":" + quoted(emitter.name);
      out += ",\"count\":" + std::to_string(emitter.count);
      out += ",\"life\":" + number(emitter.life);
      out += ",\"speed\":" + number(emitter.speed);
      out += ",\"spread\":" + number(emitter.spread);
      out += ",\"gravity\":" + number(emitter.gravity);
      out += ",\"size\":" + number(emitter.size);
      out += ",\"from\":" + vec3Json(emitter.colorStart);
      out += ",\"to\":" + vec3Json(emitter.colorEnd) + "}";
    }
    return out + "]}";
  }

  if (path == "/api/set-effect") {
    Emitter emitter;
    emitter.name = param(params, "effect");
    if (emitter.name.empty()) return errorJson("an effect needs a name");
    emitter.count = static_cast<u32>(numberParam(params, "count", 24.0));
    emitter.life = numberParam(params, "life", 0.8);
    emitter.speed = numberParam(params, "speed", 3.0);
    emitter.spread = numberParam(params, "spread", 1.0);
    emitter.direction = Vec3{numberParam(params, "dx", 0.0), numberParam(params, "dy", 1.0),
                             numberParam(params, "dz", 0.0)};
    emitter.gravity = numberParam(params, "gravity", -4.0);
    emitter.size = numberParam(params, "size", 0.12);
    emitter.shrink = numberParam(params, "shrink", 1.0);
    emitter.colorStart = Vec3{numberParam(params, "r", 1.0), numberParam(params, "g", 0.75),
                              numberParam(params, "b", 0.2)};
    emitter.colorEnd = Vec3{numberParam(params, "r2", 0.5), numberParam(params, "g2", 0.1),
                            numberParam(params, "b2", 0.0)};
    emitter.drag = numberParam(params, "drag", 0.0);
    if (!editor.setEmitter(emitter)) return errorJson("could not set that effect");
    return okJson();
  }

  if (path == "/api/drop-effect") {
    if (!editor.removeEmitter(param(params, "effect"))) return errorJson("no such effect");
    return okJson();
  }

  // Fire one from the Bench, to see it without playing the game.
  if (path == "/api/fire-effect") {
    const Vec3 at{numberParam(params, "x", 0.0), numberParam(params, "y", 1.0),
                  numberParam(params, "z", 0.0)};
    if (!editor.playEffect(param(params, "effect"), at)) return errorJson("no such effect");
    return "{\"ok\":true,\"live\":" + std::to_string(editor.particles().count()) + "}";
  }

  // --- Publishing: handing the game to somebody else ---
  if (path == "/api/publish") {
    std::string error;
    const std::string folder = editor.publish(param(params, "folder", "published"), error);
    if (folder.empty()) return errorJson(error.empty() ? "could not publish" : error);
    return okJson("folder", folder);
  }

  // --- Blueprints and stages ---

  if (path == "/api/library") {
    std::string out = "{\"ok\":true,\"blueprints\":" + stringsJson(editor.blueprintNames());
    out += ",\"stages\":" + stringsJson(editor.stageNames());
    out += ",\"stage\":" + quoted(editor.currentStage());
    return out + "}";
  }
  // Save the selected object as a reusable blueprint.
  if (path == "/api/keep") {
    const std::string as = param(params, "as", param(params, "name"));
    if (!editor.keepBlueprint(param(params, "name"), as)) return errorJson("no such object");
    return okJson("name", as);
  }
  if (path == "/api/forget") {
    if (!editor.forgetBlueprint(param(params, "blueprint"))) return errorJson("no such blueprint");
    return okJson();
  }
  // Stamp one into the scene — the whole point of saving it.
  if (path == "/api/stamp") {
    const Vec3 at{numberParam(params, "x", 0.0), numberParam(params, "y", 0.0),
                  numberParam(params, "z", 0.0)};
    const std::string name = editor.stampBlueprint(param(params, "blueprint"), at);
    if (name.empty()) return errorJson("no such blueprint");
    return okJson("name", name);
  }
  if (path == "/api/add-stage") {
    if (!editor.addStage(param(params, "stage"))) return errorJson("that stage already exists");
    return okJson();
  }
  if (path == "/api/go-stage") {
    if (!editor.goToStage(param(params, "stage"))) return errorJson("no such stage");
    return okJson("stage", editor.currentStage());
  }
  if (path == "/api/drop-stage") {
    if (!editor.removeStage(param(params, "stage"))) {
      return errorJson("cannot remove the stage you are on");
    }
    return okJson();
  }

  // --- The live viewport: tap and drag on the scene itself ---

  // What did I tap? Returns the object's name and opens its Dossier, so a
  // tap on the picture and a click in the Rack do the same thing.
  if (path == "/api/tap") {
    const std::string name = editor.pickEntityAt(numberParam(params, "x", 0.0), numberParam(params, "y", 0.0));
    if (name.empty()) return "{\"ok\":true,\"name\":\"\"}";  // a miss is not an error
    editor.selectEntity(name);
    return okJson("name", name);
  }

  // Slide the object across the ground between two pixels.
  if (path == "/api/drag") {
    const std::string name = param(params, "name");
    if (!editor.dragEntity(name, numberParam(params, "fromx", 0.0), numberParam(params, "fromy", 0.0),
                           numberParam(params, "tox", 0.0), numberParam(params, "toy", 0.0),
                           numberParam(params, "grid", 0.0))) {
      return errorJson("that drag went nowhere useful");
    }
    const EntityData* moved = editor.entity(name);
    if (moved == nullptr) return errorJson("no such object");
    return "{\"ok\":true,\"position\":" + vec3Json(moved->transform.position) + "}";
  }

  // --- Rules: the game's logic, without code ---

  // The rule list, each one as the sentence it reads as.
  if (path == "/api/rules") {
    const LogicBook& book = editor.logic();
    std::string out = "{\"ok\":true,\"rules\":[";
    for (usize i = 0; i < book.rules.size(); ++i) {
      const Rule& rule = book.rules[i];
      if (i > 0U) out += ",";
      out += "{\"index\":" + std::to_string(i);
      out += ",\"name\":" + quoted(rule.name);
      out += ",\"enabled\":" + std::string(rule.enabled ? "true" : "false");
      out += ",\"trigger\":" + quoted(triggerName(rule.trigger));
      out += ",\"subject\":" + quoted(rule.subject);
      out += ",\"other\":" + quoted(rule.other);
      out += ",\"number\":" + number(rule.number);
      out += ",\"reads\":" + quoted(describeRule(rule));
      out += ",\"conditions\":" + std::to_string(rule.conditions.size());
      out += ",\"actions\":" + std::to_string(rule.actions.size()) + "}";
    }
    out += "],\"variables\":[";
    for (usize i = 0; i < book.variables.size(); ++i) {
      const Variable& variable = book.variables[i];
      if (i > 0U) out += ",";
      out += "{\"name\":" + quoted(variable.name);
      out += ",\"number\":" + number(variable.number);
      out += ",\"text\":" + quoted(variable.text);
      out += ",\"isText\":" + std::string(variable.isText ? "true" : "false") + "}";
    }
    out += "]";
    out += ",\"finished\":" + std::string(editor.logicFinished() ? "true" : "false");
    out += ",\"won\":" + std::string(editor.logicWon() ? "true" : "false");
    out += ",\"message\":" + quoted(editor.logicMessage());
    return out + "}";
  }

  // Everything the rule editor's dropdowns need, so the page never has to
  // hard-code a list that could drift from the engine.
  if (path == "/api/rule-parts") {
    std::string out = "{\"ok\":true,\"triggers\":[";
    const char* triggers[] = {"start", "every-frame", "key", "key-held", "collision",
                              "area-enter", "area-exit", "timer", "variable", "event"};
    for (usize i = 0; i < sizeof(triggers) / sizeof(triggers[0]); ++i) {
      if (i > 0U) out += ",";
      out += quoted(triggers[i]);
    }
    out += "],\"compares\":[\"==\",\"!=\",\"<\",\"<=\",\">\",\">=\"],\"actions\":[";
    const char* acts[] = {"set", "add", "move", "move-to", "rotate", "spawn", "destroy",
                          "sound", "animate", "message", "raise", "scene", "wait", "end-game"};
    for (usize i = 0; i < sizeof(acts) / sizeof(acts[0]); ++i) {
      if (i > 0U) out += ",";
      out += quoted(acts[i]);
    }
    return out + "]}";
  }

  if (path == "/api/add-rule") {
    Rule rule;
    rule.name = param(params, "rulename", "new rule");
    if (!triggerFromName(param(params, "trigger", "start"), rule.trigger)) rule.trigger = Trigger::Start;
    rule.subject = param(params, "subject");
    rule.other = param(params, "other");
    rule.number = numberParam(params, "number", 0.0);
    const usize index = editor.addRule(rule);
    return "{\"ok\":true,\"index\":" + std::to_string(index) + "}";
  }

  // Conditions and actions are added to an existing rule, so the editor
  // builds a sentence a piece at a time the way a person says it.
  if (path == "/api/add-condition") {
    const usize index = static_cast<usize>(numberParam(params, "index", -1.0));
    LogicBook& book = editor.logic();
    if (index >= book.rules.size()) return errorJson("no such rule");
    Condition condition;
    condition.variable = param(params, "variable");
    if (!compareFromName(param(params, "compare", "=="), condition.compare)) condition.compare = Compare::Equal;
    condition.text = param(params, "text");
    condition.useText = !condition.text.empty();
    condition.number = numberParam(params, "number", 0.0);
    if (condition.variable.empty()) return errorJson("a condition needs a variable");
    book.rules[index].conditions.push_back(condition);
    return okJson();
  }

  if (path == "/api/add-action") {
    const usize index = static_cast<usize>(numberParam(params, "index", -1.0));
    LogicBook& book = editor.logic();
    if (index >= book.rules.size()) return errorJson("no such rule");
    Action action;
    if (!actFromName(param(params, "act", "set"), action.act)) action.act = Act::SetVariable;
    action.target = param(params, "target");
    action.text = param(params, "text");
    action.number = numberParam(params, "number", 0.0);
    action.amount = Vec3{numberParam(params, "ax", 0.0), numberParam(params, "ay", 0.0),
                         numberParam(params, "az", 0.0)};
    book.rules[index].actions.push_back(action);
    return okJson();
  }

  if (path == "/api/drop-rule") {
    if (!editor.removeRule(static_cast<usize>(numberParam(params, "index", -1.0)))) {
      return errorJson("no such rule");
    }
    return okJson();
  }
  if (path == "/api/toggle-rule") {
    const usize index = static_cast<usize>(numberParam(params, "index", -1.0));
    if (!editor.enableRule(index, flagParam(params, "on", true))) return errorJson("no such rule");
    return okJson();
  }
  // Order matters: it decides which rule wins when two disagree.
  if (path == "/api/move-rule") {
    if (!editor.moveRule(static_cast<usize>(numberParam(params, "index", -1.0)),
                         param(params, "dir") == "up")) {
      return errorJson("cannot move it there");
    }
    return okJson();
  }

  if (path == "/api/set-var") {
    const std::string name = param(params, "variable");
    if (name.empty()) return errorJson("a variable needs a name");
    const std::string text = param(params, "text");
    if (!text.empty()) {
      editor.setVariableText(name, text);
    } else {
      editor.setVariable(name, numberParam(params, "number", 0.0));
    }
    return okJson();
  }
  if (path == "/api/drop-var") {
    if (!editor.removeVariable(param(params, "variable"))) return errorJson("no such variable");
    return okJson();
  }

  // --- Changing the world ---

  if (path == "/api/place") {
    const std::string name = param(params, "name");
    const Vec3 position{numberParam(params, "px", 0.0), numberParam(params, "py", 0.0),
                        numberParam(params, "pz", 0.0)};
    const Vec3 scale{numberParam(params, "sx", 1.0), numberParam(params, "sy", 1.0),
                     numberParam(params, "sz", 1.0)};
    if (!editor.setEntityTransform(name, position, scale)) return errorJson("no such object");
    return okJson();
  }

  if (path == "/api/paint") {
    const Vec3 color{numberParam(params, "r", 1.0), numberParam(params, "g", 1.0), numberParam(params, "b", 1.0)};
    if (!editor.setEntityColor(param(params, "name"), color)) return errorJson("no such object");
    return okJson();
  }

  // Fittings: the physics component.
  if (path == "/api/fit-body") {
    const std::string kind = param(params, "kind", "none");
    if (kind == "off") {
      if (!editor.clearEntityBody(param(params, "name"))) return errorJson("nothing to remove");
      return okJson();
    }
    BodyComponent body;
    body.kind = bodyKindFrom(kind);
    body.mass = numberParam(params, "mass", 1.0);
    body.friction = numberParam(params, "friction", 0.4);
    body.restitution = numberParam(params, "bounce", 0.3);
    body.radius = numberParam(params, "radius", 0.0);
    if (!editor.setEntityBody(param(params, "name"), body)) return errorJson("no such object");
    return okJson();
  }

  // Wiring a motion to a button or a game event.
  if (path == "/api/wire-motion") {
    AnimationComponent clip;
    clip.clip = param(params, "clip");
    clip.trigger = param(params, "wiring");
    clip.loop = flagParam(params, "loop", true);
    clip.speed = numberParam(params, "speed", 1.0);
    if (clip.clip.empty() || clip.trigger.empty()) return errorJson("a motion needs a clip and a wiring");
    if (!editor.addEntityAnimation(param(params, "name"), clip)) return errorJson("no such object");
    return okJson();
  }

  if (path == "/api/wire-noise") {
    SoundComponent sound;
    sound.sound = param(params, "sound");
    sound.trigger = param(params, "wiring");
    sound.volume = numberParam(params, "volume", 1.0);
    if (sound.sound.empty() || sound.trigger.empty()) return errorJson("a noise needs a sound and a wiring");
    if (!editor.addEntitySound(param(params, "name"), sound)) return errorJson("no such object");
    return okJson();
  }

  if (path == "/api/unwire") {
    const std::string name = param(params, "name");
    const std::string what = param(params, "what");
    if (what == "motions") return editor.clearEntityAnimations(name) ? okJson() : errorJson("no such object");
    if (what == "noises") return editor.clearEntitySounds(name) ? okJson() : errorJson("no such object");
    return errorJson("unwire what?");
  }

  // --- A character's own bones (stage 35) ---
  // "Set" rather than "add": dragging a bone in the Bench calls this over
  // and over with the same name.
  if (path == "/api/set-bone") {
    RigBone bone;
    bone.name = param(params, "bone");
    bone.parent = param(params, "parent");
    bone.from = Vec3{numberParam(params, "fx", 0.0), numberParam(params, "fy", 0.0),
                     numberParam(params, "fz", 0.0)};
    bone.to = Vec3{numberParam(params, "tx", 0.0), numberParam(params, "ty", 0.0),
                   numberParam(params, "tz", 0.0)};
    bone.thickness = numberParam(params, "thickness", 0.08);
    bone.swing = numberParam(params, "swing", 0.0);
    if (bone.name.empty()) return errorJson("a bone needs a name");
    if (!editor.setEntityBone(param(params, "name"), bone)) return errorJson("no such object");
    return okJson();
  }
  if (path == "/api/drop-bone") {
    if (!editor.removeEntityBone(param(params, "name"), param(params, "bone"))) {
      return errorJson("no such bone");
    }
    return okJson();
  }
  if (path == "/api/clear-rig") {
    if (!editor.clearEntityRig(param(params, "name"))) return errorJson("no such object");
    return okJson();
  }
  // A starting point to edit, not a thing to accept as-is.
  if (path == "/api/default-rig") {
    if (!editor.fitDefaultRig(param(params, "name"), numberParam(params, "height", 1.7))) {
      return errorJson("no such object");
    }
    return okJson();
  }

  // Labels.
  if (path == "/api/label") {
    if (!editor.addEntityTag(param(params, "name"), param(params, "label"))) return errorJson("could not label");
    return okJson();
  }
  if (path == "/api/unlabel") {
    if (!editor.removeEntityTag(param(params, "name"), param(params, "label"))) return errorJson("no such label");
    return okJson();
  }

  // Bringing a model in from a file, and throwing one away.
  if (path == "/api/bring-in") {
    std::string error;
    const std::string name = editor.importModel(param(params, "file"), numberParam(params, "size", 1.0), error);
    if (name.empty()) return errorJson(error.empty() ? "could not import" : error);
    return okJson("name", name);
  }
  if (path == "/api/scrap") {
    if (!editor.deleteEntity(param(params, "name"))) return errorJson("no such object");
    return okJson();
  }

  // Pull the wiring by hand, to check it does what you meant without
  // leaving the Bench and playing the game.
  if (path == "/api/pull") {
    const std::string wiring = param(params, "wiring");
    const u32 fired = editor.fireTrigger(wiring);
    std::string out = "{\"ok\":true,\"fired\":" + std::to_string(fired);
    out += ",\"playing\":" + stringsJson(editor.playingAnimations());
    out += ",\"sounds\":" + stringsJson(editor.drainTriggeredSounds());
    return out + "}";
  }

  // What the engine is doing right now, for the status strip.
  if (path == "/api/pulse") {
    std::string out = "{\"ok\":true";
    out += ",\"playing\":" + std::string(editor.playing() ? "true" : "false");
    out += ",\"paused\":" + std::string(editor.paused() ? "true" : "false");
    out += ",\"clips\":" + stringsJson(editor.playingAnimations());
    out += ",\"stats\":" + quoted(editor.statsLine());
    return out + "}";
  }

  // --- Unity-style object control (the Hierarchy panel's verbs) ---

  if (path == "/api/object/create") {
    const std::string name = editor.createObject(
        param(params, "kind"), Vec3{numberParam(params, "x", 0.0), 0.0, numberParam(params, "z", 0.0)});
    if (name.empty()) return errorJson("unknown kind, or no world is open");
    return okJson("name", name);
  }
  if (path == "/api/object/duplicate") {
    std::string copy;
    if (!editor.duplicateEntity(param(params, "name"), copy)) return errorJson("no such object");
    return okJson("name", copy);
  }
  if (path == "/api/object/rename") {
    if (!editor.renameEntity(param(params, "name"), param(params, "to")))
      return errorJson("cannot rename that (the Player, Ball and Ground keep their names)");
    return okJson();
  }
  // Turntable yaw + tilt in DEGREES, the units the Inspector shows; the
  // page drags in pixels and converts, so the route speaks human units.
  if (path == "/api/object/rotate") {
    const std::string name = param(params, "name");
    if (!editor.rotateEntity(name, radians(numberParam(params, "dyaw", 0.0)),
                            radians(numberParam(params, "dpitch", 0.0)))) {
      return errorJson("no such object");
    }
    return "{\"ok\":true,\"rotation\":" + vec3Json(editor.entityEulerDegrees(name)) + "}";
  }
  if (path == "/api/object/scale") {
    const std::string name = param(params, "name");
    if (!editor.scaleEntity(name, numberParam(params, "factor", 1.0)))
      return errorJson("cannot scale that");
    const EntityData* grown = editor.entity(name);
    if (grown == nullptr) return errorJson("no such object");
    return "{\"ok\":true,\"scale\":" + vec3Json(grown->transform.scale) + "}";
  }
  if (path == "/api/object/euler") {
    if (!editor.setEntityEulerDegrees(param(params, "name"),
                                      Vec3{numberParam(params, "x", 0.0), numberParam(params, "y", 0.0),
                                           numberParam(params, "z", 0.0)})) {
      return errorJson("no such object");
    }
    return okJson();
  }
  // The Inspector's Animation section: which clips this object's own file
  // holds, and whether there is a skeleton to play them on.
  if (path == "/api/object/clips") {
    const std::string name = param(params, "name");
    std::string out = "{\"ok\":true";
    out += ",\"skeleton\":" + std::string(editor.hasSkeleton(name) ? "true" : "false");
    out += ",\"clips\":" + stringsJson(editor.animationClips(name));
    return out + "}";
  }
  if (path == "/api/object/play-clip") {
    const EntityData* target = editor.entity(param(params, "name"));
    if (target == nullptr || target->meshFile.empty()) return errorJson("that object has no model file");
    // Inspector playback is an action on this object, so preserve the
    // legacy `Entity:Clip` diagnostic. Controls use the three-argument
    // source form when they intentionally play a separate FBX.
    editor.playClip(std::string(), param(params, "clip"), target->name);
    const std::string& warning = editor.lastAnimationError();
    if (warning.empty()) return okJson();
    return "{\"ok\":true,\"warning\":" + quoted(warning) + "}";
  }

  // Live body anchors for gameplay and editor effects. The response keeps
  // both rig-local and entity/world coordinates, including x/z explicitly
  // so a caller does not have to know how the vector array is encoded.
  if (path == "/api/object/bones" || path == "/api/bones" || path == "/api/object/bone" ||
      path == "/api/bone") {
    const std::string name = param(params, "name");
    const std::vector<BoneMarker> markers = editor.characterBoneMarkers(name);
    const std::string requested = param(params, "bone");
    if (!requested.empty()) {
      for (const BoneMarker& marker : markers) {
        if (marker.name != requested) continue;
        return "{\"ok\":true,\"name\":" + quoted(marker.name) +
               ",\"local\":" + vec3Json(marker.localCenter) +
               ",\"world\":" + vec3Json(marker.center) +
               ",\"x\":" + number(marker.center.x) +
               ",\"z\":" + number(marker.center.z) + "}";
      }
      return errorJson("no such bone");
    }
    std::string out = "{\"ok\":true,\"bones\":[";
    for (usize i = 0; i < markers.size(); ++i) {
      if (i > 0U) out += ",";
      const BoneMarker& marker = markers[i];
      out += "{\"name\":" + quoted(marker.name);
      out += ",\"localStart\":" + vec3Json(marker.localStart);
      out += ",\"localEnd\":" + vec3Json(marker.localEnd);
      out += ",\"localCenter\":" + vec3Json(marker.localCenter);
      out += ",\"start\":" + vec3Json(marker.start);
      out += ",\"end\":" + vec3Json(marker.end);
      out += ",\"center\":" + vec3Json(marker.center);
      out += ",\"x\":" + number(marker.center.x) + ",\"z\":" + number(marker.center.z);
      out += ",\"length\":" + number(marker.length) + "}";
    }
    return out + "]}";
  }
  if (path == "/api/object/stop-clips") {
    const bool stopped = editor.stopEntityClips(param(params, "name"));
    return "{\"ok\":true,\"stopped\":" + std::string(stopped ? "true" : "false") + "}";
  }

  // --- Transport: the toolbar's Play/Pause/Step ---

  if (path == "/api/transport/play") {
    if (!editor.enterPlayMode()) return errorJson("no world is open");
    return "{\"ok\":true,\"playing\":true}";
  }
  if (path == "/api/transport/pause") {
    editor.setPaused(flagParam(params, "paused", true));
    return "{\"ok\":true,\"paused\":" + std::string(editor.paused() ? "true" : "false") + "}";
  }
  if (path == "/api/transport/step") {
    editor.stepOnce(1.0 / 60.0);
    return okJson();
  }
  if (path == "/api/transport/state") {
    std::string out = "{\"ok\":true";
    out += ",\"playing\":" + std::string(editor.playing() ? "true" : "false");
    out += ",\"paused\":" + std::string(editor.paused() ? "true" : "false");
    return out + "}";
  }

  // --- Asset files: the Project panel's file manager ---

  if (path == "/api/asset/rename") {
    const std::string from = param(params, "file");
    const std::string to = param(params, "to");
    // `to` stays a bare name: a rename never moves a file between folders.
    if (!safeAssetPath(from) || !plainFileName(to)) return errorJson("a plain file name, no tricks");
    const std::string oldPath = editor.importDirectory() + "/" + from;
    const usize slash = from.find_last_of('/');
    const std::string folder =
        slash == std::string::npos ? editor.importDirectory() : editor.importDirectory() + "/" + from.substr(0U, slash);
    const std::string newPath = folder + "/" + to;
    if (!fileExists(oldPath)) return errorJson("no such file");
    if (fileExists(newPath)) return errorJson("that name is taken");
    if (std::rename(oldPath.c_str(), newPath.c_str()) != 0) return errorJson("could not rename that file");
    return okJson();
  }
  if (path == "/api/asset/delete") {
    const std::string file = param(params, "file");
    if (!safeAssetPath(file)) return errorJson("a plain file name, no tricks");
    const std::string target = editor.importDirectory() + "/" + file;
    if (!fileExists(target)) return errorJson("no such file");
    if (std::remove(target.c_str()) != 0) return errorJson("could not delete that file");
    return okJson();
  }

  return errorJson("unknown request: " + path);
}

std::string saveAssetFile(WorldEditor& editor, const std::map<std::string, std::string>& params,
                          const std::string& bytes) {
  const std::string name = param(params, "name");
  if (!plainFileName(name)) return errorJson("a plain file name, no folders");
  const std::string target = editor.importDirectory() + "/" + name;
  // Never overwrite: an upload landing on an existing model would silently
  // swap every scene that uses it. Rename or delete first, on purpose.
  if (fileExists(target)) return errorJson("that name is taken (rename or delete it first)");
  std::FILE* out = std::fopen(target.c_str(), "wb");
  if (out == nullptr) return errorJson("could not write that file");
  const usize written = bytes.empty() ? 0U : std::fwrite(bytes.data(), 1U, bytes.size(), out);
  std::fclose(out);
  if (written != bytes.size()) {
    std::remove(target.c_str());  // a half-written model is worse than none
    return errorJson("could not write that file");
  }
  return okJson("file", name);
}


std::string benchPage() {
  // One self-contained document: no external files, so the editor works
  // offline on a phone exactly as it does on a desktop. The layout copies
  // Unity (Hierarchy | Inspector | Game, transport on top, Project and
  // Console below) with Unity terms; every line of markup, style and
  // script is written from scratch for this engine.
  return R"BENCH(<!doctype html>
<html lang="en" dir="ltr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>KIMIA Editor</title>
<style>
:root{
  --bg:#0a0e11; --panel:#111518; --panel2:#161b1f; --edge:#3e4245;
  --ink:#d7dade; --dim:#8a9299; --accent:#c9d2d8; --go:#7fbf9a; --bad:#d08a80;
}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;background:var(--bg);color:var(--ink);
  font:13px/1.45 "Segoe UI",system-ui,Roboto,sans-serif;overflow:hidden}
#editor{display:grid;height:100vh;
  grid-template-columns:232px 308px 1fr;
  grid-template-rows:46px 1fr 188px;
  grid-template-areas:"top top top" "hier insp game" "proj proj console"}
@media(max-width:1100px){
  #editor{grid-template-columns:200px 260px 1fr}
}
@media(max-width:900px){
  #editor{grid-template-columns:1fr;grid-template-rows:46px 30vh 1fr 150px;
    grid-template-areas:"top" "game" "insp" "console"}
  #hier{display:none}
  #hier.show{display:block;position:fixed;inset:46px 0 0 0;z-index:20}
  #proj{display:none}
}
#top{grid-area:top;display:flex;align-items:center;gap:8px;padding:0 10px;
  background:var(--panel2);border-bottom:1px solid var(--edge);overflow-x:auto}
.brand{font-weight:700;letter-spacing:.12em;color:var(--accent);white-space:nowrap}
.brand small{color:var(--dim);font-weight:400;letter-spacing:0}
#hier{grid-area:hier;background:var(--panel);border-right:1px solid var(--edge);
  overflow:auto;padding:8px;min-height:0}
#insp{grid-area:insp;background:var(--panel);border-right:1px solid var(--edge);
  overflow:auto;padding:10px;min-height:0}
#game{grid-area:game;position:relative;background:#05070a;display:flex;
  align-items:center;justify-content:center;overflow:hidden;min-width:0}
#game img{max-width:100%;max-height:100%;image-rendering:pixelated;
  touch-action:none;user-select:none;-webkit-user-drag:none}
#gamebar{position:absolute;left:0;right:0;bottom:0;display:flex;gap:12px;
  align-items:center;padding:6px 10px;background:rgba(6,9,12,.85);
  border-top:1px solid var(--edge);font-size:11px;color:var(--dim)}
#gametool{color:var(--accent);font-weight:600}
#tapHint{margin-left:auto;color:var(--go)}
#proj{grid-area:proj;background:var(--panel);border-top:1px solid var(--edge);
  border-right:1px solid var(--edge);overflow:auto;padding:8px 10px;min-height:0}
#console{grid-area:console;background:#0a0e11;border-top:1px solid var(--edge);
  display:flex;flex-direction:column;min-height:0}
#conbar{display:flex;align-items:center;gap:8px;padding:6px 10px;
  border-bottom:1px solid var(--edge);font-size:11px;color:var(--dim)}
#conlog{flex:1;overflow:auto;padding:6px 10px;font-family:ui-monospace,Menlo,monospace;
  font-size:11px}
#conlog div{white-space:nowrap}
#conlog .t{color:#4a5258;margin-right:8px}
h2{font-size:11px;letter-spacing:.14em;color:var(--dim);margin:12px 0 6px;
  text-transform:uppercase;font-weight:600}
h2:first-child{margin-top:0}
.row{display:flex;gap:6px;align-items:center;margin-bottom:6px}
.row label{color:var(--dim);min-width:48px;font-size:12px}
button{background:#232a2f;color:var(--ink);border:1px solid var(--edge);
  border-radius:4px;padding:5px 9px;cursor:pointer;font:inherit;font-size:12px;white-space:nowrap}
button:hover{border-color:var(--accent)}
button.on{background:#2b3238;border-color:var(--accent)}
button.go{background:#1f3a2a;border-color:#33553f;font-weight:600}
button.bad{background:#3a2422;border-color:#5a3a36}
button:disabled{opacity:.4;cursor:default}
input,select{background:#0d1216;color:var(--ink);border:1px solid var(--edge);
  border-radius:4px;padding:4px 6px;font:inherit;font-size:12px;width:100%;min-width:0}
input[type=color]{padding:1px;height:26px}
input[type=checkbox]{width:auto}
.item{padding:5px 7px;border:1px solid transparent;border-radius:4px;cursor:pointer;
  display:flex;align-items:center;gap:6px;font-size:12px}
.item:hover{background:#1a2126}
.item.on{background:#232b31;border-color:var(--accent)}
.item button{padding:1px 7px;font-size:10px}
.pip{width:7px;height:7px;border-radius:50%;background:#3e4245;flex:none}
.pip.solid{background:var(--accent)}
.pip.moving{background:var(--go)}
.tag{display:inline-flex;align-items:center;gap:4px;background:#232a2f;
  border:1px solid var(--edge);border-radius:10px;padding:1px 7px;font-size:11px;margin:0 4px 4px 0}
.tag b{cursor:pointer;color:var(--dim)}
.tag b:hover{color:var(--bad)}
.grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:4px}
.wire{background:#0d1216;border:1px solid var(--edge);border-radius:4px;
  padding:4px 6px;margin-bottom:4px;font-size:11px;display:flex;justify-content:space-between;gap:6px}
.wire span{color:var(--accent)}
.hint{color:var(--dim);font-size:11px;line-height:1.6}
.projcols{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}
@media(max-width:1100px){.projcols{grid-template-columns:1fr 1fr}}
#rulesSheet{display:none;position:fixed;inset:0;background:var(--bg);z-index:40;
  flex-direction:column}
#rulesSheet.show{display:flex}
.sheetbar{display:flex;align-items:center;gap:10px;padding:10px 14px;
  background:var(--panel2);border-bottom:1px solid var(--edge)}
.sheetbody{flex:1;overflow:auto;display:grid;gap:14px;padding:14px;
  grid-template-columns:1fr 1fr 1fr 1fr}
@media(max-width:1200px){.sheetbody{grid-template-columns:1fr 1fr}}
@media(max-width:900px){.sheetbody{grid-template-columns:1fr}}
.col{background:var(--panel);border:1px solid var(--edge);border-radius:6px;padding:10px}
.rule{background:#0d1216;border:1px solid var(--edge);border-radius:4px;
  padding:6px 8px;margin-bottom:5px;font-size:11px;cursor:pointer;line-height:1.5}
.rule.on{border-color:var(--accent)}
.rule.off{opacity:.45}
.rule .tools{display:flex;gap:4px;margin-top:5px}
.rule .tools button{padding:1px 6px;font-size:10px}
#flash{position:fixed;bottom:200px;left:50%;transform:translateX(-50%);
  background:#0d1216;border:1px solid var(--accent);border-radius:6px;
  padding:7px 14px;font-size:12px;opacity:0;transition:opacity .2s;pointer-events:none;z-index:50}
#flash.on{opacity:1}
#flash.err{border-color:var(--bad);color:#f0b5ae}
.toolbar-group{display:flex;gap:4px;align-items:center;padding:0 8px;
  border-left:1px solid var(--edge);border-right:1px solid var(--edge)}
.toolbar-group:first-of-type{border-left:none}
</style>
</head>
<body>
<div id="editor">
  <div id="top">
    <div class="brand">KIMIA <small>Editor</small></div>
    <button onclick="toggleHier()" id="hierBtn" style="display:none">Hierarchy</button>
    <div class="toolbar-group" title="Transform tools (Q/W/E/R)">
      <button id="toolSelect" class="on" onclick="setTool('select')">Select</button>
      <button id="toolMove" onclick="setTool('move')">Move</button>
      <button id="toolRotate" onclick="setTool('rotate')">Rotate</button>
      <button id="toolScale" onclick="setTool('scale')">Scale</button>
    </div>
    <div class="toolbar-group" title="Play controls">
      <button id="btnPlay" class="go" onclick="transportPlay()">&#9654; Play</button>
      <button id="btnPause" onclick="transportPause()">&#10073;&#10073; Pause</button>
      <button id="btnStep" onclick="transportStep()">Step &#9654;|</button>
    </div>
    <div class="toolbar-group" title="Snapping">
      <label style="font-size:11px;color:var(--dim)"><input type="checkbox" id="snapOn"> snap</label>
      <select id="snapVal" style="width:auto">
        <option value="0.1">0.1</option><option value="0.25">0.25</option>
        <option value="0.5" selected>0.5</option><option value="1">1</option>
      </select>
    </div>
    <div class="toolbar-group" title="Create a game object at the origin">
      <select id="createKind" style="width:auto">
        <option value="cube">Cube</option><option value="sphere">Sphere</option>
        <option value="plane">Plane</option><option value="block">Block</option>
        <option value="wall">Wall</option><option value="goal">Goal</option>
        <option value="crate">Crate</option><option value="hole">Hole</option>
        <option value="player">Player</option><option value="ball">Ball</option>
      </select>
      <button class="go" onclick="createObject()">+ Create</button>
    </div>
    <div style="flex:1"></div>
    <span class="hint" id="worldName">-</span>
    <button onclick="showRules()">Rules</button>
    <button class="go" onclick="publish()">Publish</button>
    <button onclick="location.href='/'">Game &rsaquo;</button>
  </div>

  <div id="hier">
    <h2>Hierarchy</h2>
    <div class="row">
      <button style="flex:1" onclick="duplicate()" title="Ctrl+D">Duplicate</button>
      <button class="bad" style="flex:1" onclick="scrap()">Delete</button>
    </div>
    <div id="hierList"></div>
    <div class="hint">Click to select. Ctrl+D duplicates, Delete removes.</div>
  </div>

  <div id="insp">
    <div id="empty" class="hint">Select something in the Hierarchy to inspect it.</div>
    <div id="sheet" style="display:none">
      <h2>Inspector</h2>
      <div class="row"><input id="dName"><button onclick="rename()">Rename</button></div>
      <div class="hint" id="dMesh"></div>

      <h2>Transform</h2>
      <div class="row"><label>Position</label></div>
      <div class="grid3">
        <input id="px" type="number" step="0.1" title="x">
        <input id="py" type="number" step="0.1" title="y">
        <input id="pz" type="number" step="0.1" title="z">
      </div>
      <div class="row" style="margin-top:6px"><label>Rotation</label></div>
      <div class="grid3">
        <input id="rx" type="number" step="1" title="pitch degrees">
        <input id="ry" type="number" step="1" title="yaw degrees">
        <input id="rz" type="number" step="1" title="roll degrees">
      </div>
      <div class="row" style="margin-top:6px"><label>Scale</label></div>
      <div class="grid3">
        <input id="sx" type="number" step="0.1" title="width">
        <input id="sy" type="number" step="0.1" title="height">
        <input id="sz" type="number" step="0.1" title="depth">
      </div>
      <div class="row" style="margin-top:6px">
        <input id="tint" type="color" style="max-width:52px">
        <button class="go" style="flex:1" onclick="applyTransform()">Apply</button>
      </div>

      <h2>Animation</h2>
      <div class="hint" id="animStat">-</div>
      <div class="row"><select id="clipSel"></select></div>
      <div class="row">
        <button class="go" style="flex:1" onclick="playClip()">Play clip</button>
        <button style="flex:1" onclick="stopClips()">Stop</button>
      </div>
      <h2>Live body anchors</h2>
      <div class="hint" id="liveBones">No named bones</div>

      <h2>Body</h2>
      <div class="row"><label>kind</label>
        <select id="bKind">
          <option value="off">none (scenery)</option>
          <option value="static">static (wall)</option>
          <option value="dynamic">dynamic (pushable)</option>
          <option value="sphere">sphere (rolls)</option>
        </select></div>
      <div class="row"><label>mass</label><input id="bMass" type="number" step="0.1" value="1"></div>
      <div class="row"><label>grip</label><input id="bFric" type="number" step="0.05" value="0.4"></div>
      <div class="row"><label>bounce</label><input id="bBounce" type="number" step="0.05" value="0.3"></div>
      <button class="go" style="width:100%" onclick="fitBody()">Apply body</button>

      <h2>Tags</h2>
      <div id="labels"></div>
      <div class="row"><input id="newLabel" placeholder="enemy"><button onclick="addLabel()">Add</button></div>

      <h2>Wiring &mdash; Motion</h2>
      <div id="motions"></div>
      <div class="row"><input id="mClip" placeholder="clip name"></div>
      <div class="row"><label>button</label><input id="mWire" placeholder="k or goal"></div>
      <div class="row"><button class="go" style="flex:1" onclick="wireMotion()">Wire up</button>
        <button class="bad" onclick="unwire('motions')">Clear</button></div>

      <h2>Wiring &mdash; Noise</h2>
      <div id="noises"></div>
      <div class="row"><input id="nSound" placeholder="kick"></div>
      <div class="row"><label>button</label><input id="nWire" placeholder="k or goal"></div>
      <div class="row"><button class="go" style="flex:1" onclick="wireNoise()">Wire up</button>
        <button class="bad" onclick="unwire('noises')">Clear</button></div>

      <h2>Frame &mdash; bones</h2>
      <div id="bones"></div>
      <div class="row"><input id="bName" placeholder="LeftLeg">
        <input id="bParent" placeholder="parent"></div>
      <div class="hint">from x y z &rarr; to x y z (feet at y=0)</div>
      <div class="grid3">
        <input id="bfx" type="number" step="0.01" value="0">
        <input id="bfy" type="number" step="0.01" value="0.9">
        <input id="bfz" type="number" step="0.01" value="0">
      </div>
      <div class="grid3" style="margin-top:4px">
        <input id="btx" type="number" step="0.01" value="0">
        <input id="bty" type="number" step="0.01" value="0.45">
        <input id="btz" type="number" step="0.01" value="0">
      </div>
      <div class="row" style="margin-top:4px">
        <label>thick</label><input id="bth" type="number" step="0.01" value="0.08">
        <label>swing</label><input id="bsw" type="number" step="0.1" value="1">
      </div>
      <div class="row">
        <button class="go" style="flex:1" onclick="setBone()">Set bone</button>
        <button onclick="defaultRig()">Default</button>
        <button class="bad" onclick="clearRig()">Clear</button>
      </div>

      <h2>Bench test</h2>
      <div class="row"><input id="pullWire" placeholder="k"><button onclick="pull()">Pull</button></div>
      <div class="hint">Pull a wire to fire it here, without playing the game.</div>

      <h2>&nbsp;</h2>
      <button class="bad" style="width:100%" onclick="scrap()">Delete this object</button>
    </div>
  </div>

  <div id="game">
    <img id="view" alt="game">
    <div id="gamebar">
      <span id="gametool">Select</span>
      <span id="playstat">edit</span>
      <span id="tapHint">click an object to select it</span>
    </div>
  </div>

  <div id="proj">
    <div class="projcols">
      <div>
        <h2>Project &mdash; Files</h2>
        <div class="row">
          <button onclick="loadAssets(0)">List</button>
          <button onclick="loadAssets(1)">Scan</button>
          <button class="go" onclick="uploadAsset()">Upload</button>
          <input type="file" id="upFile" style="display:none">
          <span class="hint" id="scanHint">Scan looks inside models</span>
        </div>
        <div id="assetList"></div>
        <div class="row"><input id="inFile" placeholder="assets/thing.obj"></div>
        <div class="row"><label>size</label><input id="inSize" type="number" value="1" step="0.1">
          <button class="go" onclick="bringIn()">Import</button></div>
      </div>
      <div>
        <h2>Project &mdash; Prefabs</h2>
        <div id="bpList"></div>
        <div class="row"><input id="bpName" placeholder="save selected as...">
          <button class="go" onclick="keepBlueprint()">Keep</button></div>
      </div>
      <div>
        <h2>Project &mdash; Scenes</h2>
        <div id="stageList"></div>
        <div class="row"><input id="newStage" placeholder="Level 2">
          <button onclick="addStage()">Add</button></div>
      </div>
    </div>
  </div>

  <div id="console">
    <div id="conbar"><b>Console</b><span id="conStat"></span>
      <div style="flex:1"></div><button onclick="clearLog()">Clear</button></div>
    <div id="conlog"></div>
  </div>
</div>

<div id="rulesSheet">
  <div class="sheetbar">
    <b>Rules &mdash; when this, do that</b>
    <div style="flex:1"></div>
    <button onclick="hideRules()">Close</button>
  </div>
  <div class="sheetbody">
    <div class="col">
      <h2>The rules</h2>
      <div id="ruleList"></div>
      <h2>New rule</h2>
      <div class="row"><input id="rName" placeholder="what it does"></div>
      <div class="row"><label>when</label>
        <select id="rTrigger">
          <option value="start">start (once)</option>
          <option value="every-frame">every frame</option>
          <option value="key">key pressed</option>
          <option value="key-held">key held</option>
          <option value="collision">collision</option>
          <option value="area-enter">enters area</option>
          <option value="area-exit">leaves area</option>
          <option value="timer">timer</option>
          <option value="event">event</option>
        </select></div>
      <div class="row"><label>who</label><input id="rSubject" placeholder="space / Ball / Player"></div>
      <div class="row"><label>with</label><input id="rOther" placeholder="Wall / Goal"></div>
      <div class="row"><label>number</label><input id="rNumber" type="number" step="0.1" value="0"
        title="timer seconds, or area radius"></div>
      <button class="go" style="width:100%" onclick="addRule()">Add rule</button>
    </div>

    <div class="col">
      <h2>Add to rule <span id="pickedRule" style="color:var(--accent)">&mdash;</span></h2>
      <div class="hint">Pick a rule on the left, then add an IF or a DO.</div>

      <h2>IF (optional)</h2>
      <div class="row"><input id="cVar" placeholder="score"></div>
      <div class="row"><label>is</label>
        <select id="cCmp">
          <option>==</option><option>!=</option><option>&lt;</option>
          <option>&lt;=</option><option>&gt;</option><option>&gt;=</option>
        </select>
        <input id="cNum" type="number" step="1" value="0"></div>
      <button style="width:100%" onclick="addCondition()">Add condition</button>

      <h2>DO</h2>
      <div class="row"><label>action</label>
        <select id="aAct">
          <option value="add">add to variable</option>
          <option value="set">set variable</option>
          <option value="move">move</option>
          <option value="move-to">move to</option>
          <option value="rotate">rotate</option>
          <option value="spawn">spawn a copy</option>
          <option value="destroy">destroy</option>
          <option value="sound">play sound</option>
          <option value="animate">play animation</option>
          <option value="message">show message</option>
          <option value="raise">raise event</option>
          <option value="effect">play effect</option>
          <option value="wait">wait</option>
          <option value="end-game">end the game</option>
        </select></div>
      <div class="row"><label>on</label><input id="aTarget" placeholder="Ball / score"></div>
      <div class="row"><label>name</label><input id="aText" placeholder="sound / clip / message"></div>
      <div class="row"><label>amount</label><input id="aNum" type="number" step="1" value="1"></div>
      <div class="grid3">
        <input id="aax" type="number" step="0.1" value="0" title="x">
        <input id="aay" type="number" step="0.1" value="0" title="y">
        <input id="aaz" type="number" step="0.1" value="0" title="z">
      </div>
      <button class="go" style="width:100%;margin-top:6px" onclick="addAction()">Add action</button>
    </div>

    <div class="col">
      <h2>Screen &mdash; panels</h2>
      <div id="panelList"></div>
      <div class="row"><input id="pName" placeholder="scoreLabel">
        <select id="pKind">
          <option value="label">label</option><option value="bar">bar</option>
          <option value="box">box</option><option value="button">button</option>
        </select></div>
      <div class="row"><label>text</label><input id="pText" placeholder="Score: {score}"></div>
      <div class="hint">{score} shows a variable's value</div>
      <div class="row"><label>bar of</label><input id="pVar" placeholder="lives">
        <input id="pMax" type="number" step="1" value="100" style="max-width:70px"></div>
      <div class="row"><label>on press</label><input id="pEvent" placeholder="restart"></div>
      <div class="row"><label>at</label>
        <input id="pX" type="number" step="0.01" value="0.02" title="left 0..1">
        <input id="pY" type="number" step="0.01" value="0.02" title="top 0..1"></div>
      <div class="row"><label>size</label>
        <input id="pW" type="number" step="0.01" value="0.3" title="width 0..1">
        <input id="pH" type="number" step="0.01" value="0.08" title="height 0..1"></div>
      <div class="row"><input id="pColor" type="color" value="#e6e6f2">
        <input id="pBack" type="color" value="#1a1f26"></div>
      <button class="go" style="width:100%" onclick="setPanel()">Place panel</button>

      <h2>Controls</h2>
      <div id="ctrlList"></div>
      <div class="row"><input id="cName" placeholder="jump">
        <input id="cLabel" placeholder="label"></div>
      <div class="row"><label>key</label><input id="cKey" placeholder="space">
        <label>pad</label><input id="cPad" placeholder="a"></div>
      <div class="row"><label><input type="checkbox" id="cTouch" checked> on-screen button</label></div>
      <div class="row"><label>at</label>
        <input id="cX" type="number" step="0.01" value="0.85" title="x 0..1">
        <input id="cY" type="number" step="0.01" value="0.78" title="y 0..1">
        <input id="cSize" type="number" step="0.01" value="0.12" title="size"></div>
      <div class="row"><label>clip</label>
        <select id="cClipFile" onchange="clipsOf(this.value)"><option value="">-- model --</option></select></div>
      <div class="row"><select id="cClip"><option value="">-- none --</option></select>
        <input id="cTarget" placeholder="target character (optional)">
        <input id="cSound" placeholder="sound"></div>
      <div class="row">
        <button class="go" style="flex:1" onclick="setControl()">Save control</button>
        <button onclick="doControl()">Test</button></div>
      <div class="row"><label><input type="checkbox" id="cStick" onchange="setStick()"> movement stick</label></div>

      <h2>Effects</h2>
      <div id="fxList"></div>
      <div class="row"><input id="fxName" placeholder="explosion">
        <button onclick="fireEffect()">Test</button></div>
      <div class="row"><label>count</label><input id="fxCount" type="number" value="24">
        <label>life</label><input id="fxLife" type="number" step="0.1" value="0.8"></div>
      <div class="row"><label>speed</label><input id="fxSpeed" type="number" step="0.5" value="3">
        <label>spread</label><input id="fxSpread" type="number" step="0.1" value="1"></div>
      <div class="row"><label>gravity</label><input id="fxGrav" type="number" step="0.5" value="-4">
        <label>size</label><input id="fxSize" type="number" step="0.02" value="0.12"></div>
      <div class="row"><input id="fxFrom" type="color" value="#ffbf33">
        <input id="fxTo" type="color" value="#801a00"></div>
      <button class="go" style="width:100%" onclick="setEffect()">Save effect</button>

      <h2>Variables</h2>
      <div id="varList"></div>
      <div class="row"><input id="vName" placeholder="score">
        <input id="vNum" type="number" step="1" value="0" style="max-width:80px"></div>
      <button style="width:100%" onclick="setVar()">Set</button>
      <h2>State</h2>
      <div class="hint" id="logicState">&mdash;</div>
    </div>
  </div>
</div>
<div id="flash"></div>

<script>
var picked = null;
var tool = 'select';
var pausedNow = false;

function flash(msg, bad){
  var f = document.getElementById('flash');
  f.textContent = msg;
  f.className = bad ? 'on err' : 'on';
  clearTimeout(f.timer);
  f.timer = setTimeout(function(){ f.className = ''; }, 1800);
}
function log(msg){
  var box = document.getElementById('conlog');
  var line = document.createElement('div');
  var t = new Date();
  var pad = function(n){ return (n < 10 ? '0' : '') + n; };
  var stamp = document.createElement('span');
  stamp.className = 't';
  stamp.textContent = pad(t.getHours()) + ':' + pad(t.getMinutes()) + ':' + pad(t.getSeconds());
  line.appendChild(stamp);
  line.appendChild(document.createTextNode(msg));
  box.appendChild(line);
  while (box.children.length > 200) box.removeChild(box.firstChild);
  box.scrollTop = box.scrollHeight;
}
function clearLog(){ document.getElementById('conlog').innerHTML = ''; }
function api(path, params, done){
  var q = [];
  for (var k in params) q.push(encodeURIComponent(k) + '=' + encodeURIComponent(params[k]));
  fetch('/api/' + path + (q.length ? '?' + q.join('&') : ''))
    .then(function(r){ return r.json(); })
    .then(function(d){
      if (d && d.ok === false){ flash(d.error || 'refused', true); log('error: ' + (d.error || path)); }
      if (done) done(d);
    })
    .catch(function(){ flash('engine not answering', true); });
}
function toggleHier(){ document.getElementById('hier').classList.toggle('show'); }
function hex(c){
  var n = Math.max(0, Math.min(255, Math.round(c * 255))).toString(16);
  return n.length < 2 ? '0' + n : n;
}
function snapGrid(){
  if (!document.getElementById('snapOn').checked) return 0;
  return parseFloat(document.getElementById('snapVal').value) || 0;
}

// --- Transform tools (Q/W/E/R) ---
function setTool(name){
  tool = name;
  var tools = ['select', 'move', 'rotate', 'scale'];
  var labels = {select: 'Select', move: 'Move', rotate: 'Rotate', scale: 'Scale'};
  for (var i = 0; i < tools.length; i++){
    document.getElementById('tool' + labels[tools[i]]).className = tools[i] === name ? 'on' : '';
  }
  document.getElementById('gametool').textContent = labels[name];
  var hints = {
    select: 'click an object to select it',
    move: 'drag to move along the ground',
    rotate: 'drag sideways to turn, up/down to tilt',
    scale: 'drag up to grow, down to shrink'
  };
  document.getElementById('tapHint').textContent =
    (picked ? picked + '  -  ' : '') + hints[name];
}

// --- Hierarchy ---
function loadHier(){
  api('rack', {}, function(d){
    if (!d || !d.items) return;
    document.getElementById('worldName').textContent = d.world || '';
    var box = document.getElementById('hierList');
    box.innerHTML = '';
    d.items.forEach(function(it){
      var row = document.createElement('div');
      row.className = 'item' + (it.name === picked ? ' on' : '');
      var pip = document.createElement('span');
      pip.className = 'pip' + (it.body === 'static' ? ' solid' :
                     (it.body && it.body !== '' ? ' moving' : ''));
      row.appendChild(pip);
      var label = document.createElement('span');
      label.textContent = it.name;
      row.appendChild(label);
      var marks = [];
      if (it.imported) marks.push('file');
      if (it.labels) marks.push(it.labels + 'L');
      if (it.motions) marks.push(it.motions + 'M');
      if (it.noises) marks.push(it.noises + 'N');
      if (marks.length){
        var tail = document.createElement('span');
        tail.style.cssText = 'margin-left:auto;color:#9a9a9a;font-size:10px';
        tail.textContent = marks.join(' ');
        row.appendChild(tail);
      }
      row.onclick = function(){ pick(it.name); };
      box.appendChild(row);
    });
  });
}
function createObject(){
  var kind = document.getElementById('createKind').value;
  api('object/create', {kind: kind, x: 0, z: 0}, function(d){
    if (d && d.ok){ log('created ' + d.name); loadHier(); pick(d.name); }
  });
}
function duplicate(){
  if (!need()) return;
  api('object/duplicate', {name: picked}, function(d){
    if (d && d.ok){ log('duplicated ' + picked + ' as ' + d.name); loadHier(); pick(d.name); }
  });
}
function rename(){
  if (!need()) return;
  var to = document.getElementById('dName').value.trim();
  if (!to) return;
  api('object/rename', {name: picked, to: to}, function(d){
    if (d && d.ok){ log('renamed ' + picked + ' to ' + to); picked = to; loadHier(); pick(to); }
  });
}

// --- Transport: Play/Pause/Step ---
function transportPlay(){
  api('transport/play', {}, function(d){
    if (d && d.ok){ log('play'); pollTransport(); }
  });
}
function transportPause(){
  api('transport/pause', {paused: pausedNow ? 0 : 1}, function(d){
    if (d && d.ok){ log(d.paused ? 'paused' : 'resumed'); pollTransport(); }
  });
}
function transportStep(){
  api('transport/step', {}, function(d){ if (d && d.ok) log('stepped one frame'); });
}
function pollTransport(){
  api('transport/state', {}, function(d){
    if (!d) return;
    pausedNow = !!d.paused;
    document.getElementById('btnPause').className = pausedNow ? 'on' : '';
    document.getElementById('btnPlay').className = d.playing ? 'on' : 'go';
    document.getElementById('playstat').textContent =
      d.playing ? (pausedNow ? 'paused' : 'playing') : 'edit';
  });
}

// --- Inspector ---
function pick(name){
  picked = name;
  api('dossier', {name: name}, function(d){
    if (!d || !d.dossier) return;
    var o = d.dossier;
    document.getElementById('empty').style.display = 'none';
    document.getElementById('sheet').style.display = '';
    document.getElementById('dName').value = o.name;
    document.getElementById('dMesh').textContent =
      (o.mesh ? o.mesh : 'built-in shape') + '   - ' + o.span.toFixed(2) + 'm';
    var ids = ['px', 'py', 'pz'], rids = ['rx', 'ry', 'rz'], sids = ['sx', 'sy', 'sz'];
    for (var i = 0; i < 3; i++){
      document.getElementById(ids[i]).value = o.position[i].toFixed(2);
      document.getElementById(rids[i]).value = (o.rotation ? o.rotation[i] : 0).toFixed(1);
      document.getElementById(sids[i]).value = o.scale[i].toFixed(2);
    }
    document.getElementById('tint').value =
      '#' + hex(o.color[0]) + hex(o.color[1]) + hex(o.color[2]);
    document.getElementById('bKind').value = o.body ? o.body.kind : 'off';
    if (o.body){
      document.getElementById('bMass').value = o.body.mass.toFixed(2);
      document.getElementById('bFric').value = o.body.friction.toFixed(2);
      document.getElementById('bBounce').value = o.body.bounce.toFixed(2);
    }
    var lab = document.getElementById('labels');
    lab.innerHTML = '';
    o.labels.forEach(function(t){
      var chip = document.createElement('span');
      chip.className = 'tag';
      chip.textContent = t;
      var x = document.createElement('b');
      x.textContent = 'x';
      x.onclick = function(){ api('unlabel', {name: picked, label: t}, function(){ pick(picked); loadHier(); }); };
      chip.appendChild(x);
      lab.appendChild(chip);
    });
    var bv = document.getElementById('bones');
    bv.innerHTML = '';
    (o.bones || []).forEach(function(b){
      var w = document.createElement('div');
      w.className = 'wire';
      w.style.cursor = 'pointer';
      var span = b.parent ? (b.name + ' <- ' + b.parent) : b.name;
      var left = document.createElement('i');
      left.textContent = span;
      var right = document.createElement('span');
      right.textContent = b.swing.toFixed(1);
      w.appendChild(left);
      w.appendChild(right);
      // Click a bone to load it into the fields, so editing is a tweak
      // rather than retyping the whole thing.
      w.onclick = function(){
        document.getElementById('bName').value = b.name;
        document.getElementById('bParent').value = b.parent;
        document.getElementById('bfx').value = b.from[0].toFixed(3);
        document.getElementById('bfy').value = b.from[1].toFixed(3);
        document.getElementById('bfz').value = b.from[2].toFixed(3);
        document.getElementById('btx').value = b.to[0].toFixed(3);
        document.getElementById('bty').value = b.to[1].toFixed(3);
        document.getElementById('btz').value = b.to[2].toFixed(3);
        document.getElementById('bth').value = b.thickness.toFixed(3);
        document.getElementById('bsw').value = b.swing.toFixed(2);
      };
      bv.appendChild(w);
    });

    var mv = document.getElementById('motions');
    mv.innerHTML = '';
    o.motions.forEach(function(m){
      var w = document.createElement('div');
      w.className = 'wire';
      var left = document.createElement('i');
      left.textContent = m.clip;
      var right = document.createElement('span');
      right.textContent = '<- ' + m.wiring;
      w.appendChild(left);
      w.appendChild(right);
      mv.appendChild(w);
    });
    var nv = document.getElementById('noises');
    nv.innerHTML = '';
    o.noises.forEach(function(n){
      var w = document.createElement('div');
      w.className = 'wire';
      var left = document.createElement('i');
      left.textContent = n.sound;
      var right = document.createElement('span');
      right.textContent = '<- ' + n.wiring;
      w.appendChild(left);
      w.appendChild(right);
      nv.appendChild(w);
    });
    loadClips();
    loadBones(name);
    loadHier();
    setTool(tool);
  });
}
function loadBones(name){
  api('object/bones', {name: name}, function(d){
    var box = document.getElementById('liveBones');
    if (!box) return;
    box.innerHTML = '';
    if (!d || !(d.bones || []).length){
      box.textContent = 'No named bones';
      return;
    }
    (d.bones || []).forEach(function(b){
      var row = document.createElement('div');
      row.className = 'wire';
      var left = document.createElement('i');
      left.textContent = b.name;
      var right = document.createElement('span');
      right.textContent = 'x ' + b.x.toFixed(2) + '  z ' + b.z.toFixed(2);
      row.appendChild(left); row.appendChild(right); box.appendChild(row);
    });
  });
}
function loadClips(){
  if (!picked) return;
  api('object/clips', {name: picked}, function(d){
    if (!d) return;
    document.getElementById('animStat').textContent =
      d.skeleton ? ((d.clips || []).length + ' clip(s) in the model file')
                 : 'no skeleton in this object';
    var sel = document.getElementById('clipSel');
    sel.innerHTML = '';
    (d.clips || []).forEach(function(c){
      var o = document.createElement('option');
      o.value = c;
      o.textContent = c;
      sel.appendChild(o);
    });
    document.getElementById('clipSel').disabled = !(d.clips || []).length;
  });
}
function playClip(){
  if (!need()) return;
  var clip = document.getElementById('clipSel').value;
  if (!clip){ flash('no clip to play', true); return; }
  api('object/play-clip', {name: picked, clip: clip}, function(d){
    if (d && d.ok) log('playing ' + clip + ' on ' + picked);
  });
}
function stopClips(){
  if (!need()) return;
  api('object/stop-clips', {name: picked}, function(d){
    if (d && d.ok) log(d.stopped ? 'stopped clips on ' + picked : 'nothing was playing');
  });
}

function need(){ if (!picked){ flash('select something first', true); return false; } return true; }

function applyTransform(){
  if (!need()) return;
  var c = document.getElementById('tint').value;
  api('place', {name: picked,
    px: document.getElementById('px').value, py: document.getElementById('py').value,
    pz: document.getElementById('pz').value, sx: document.getElementById('sx').value,
    sy: document.getElementById('sy').value, sz: document.getElementById('sz').value},
    function(){
      api('object/euler', {name: picked,
        x: document.getElementById('rx').value, y: document.getElementById('ry').value,
        z: document.getElementById('rz').value}, function(){
          api('paint', {name: picked,
            r: parseInt(c.substr(1, 2), 16) / 255, g: parseInt(c.substr(3, 2), 16) / 255,
            b: parseInt(c.substr(5, 2), 16) / 255}, function(){ log('set transform of ' + picked); pick(picked); });
        });
    });
}
function fitBody(){
  if (!need()) return;
  api('fit-body', {name: picked, kind: document.getElementById('bKind').value,
    mass: document.getElementById('bMass').value, friction: document.getElementById('bFric').value,
    bounce: document.getElementById('bBounce').value}, function(d){
      if (d && d.ok) log('set body of ' + picked);
      pick(picked);
    });
}
function addLabel(){
  if (!need()) return;
  var t = document.getElementById('newLabel').value.trim();
  if (!t) return;
  api('label', {name: picked, label: t}, function(){
    document.getElementById('newLabel').value = '';
    pick(picked);
  });
}
function wireMotion(){
  if (!need()) return;
  api('wire-motion', {name: picked, clip: document.getElementById('mClip').value,
    wiring: document.getElementById('mWire').value, loop: 0}, function(d){
      if (d && d.ok){ log('wired motion on ' + picked); pick(picked); }
    });
}
function wireNoise(){
  if (!need()) return;
  api('wire-noise', {name: picked, sound: document.getElementById('nSound').value,
    wiring: document.getElementById('nWire').value}, function(d){
      if (d && d.ok){ log('wired sound on ' + picked); pick(picked); }
    });
}
function unwire(what){
  if (!need()) return;
  api('unwire', {name: picked, what: what}, function(){ pick(picked); });
}
function setBone(){
  if (!need()) return;
  api('set-bone', {name: picked,
    bone: document.getElementById('bName').value,
    parent: document.getElementById('bParent').value,
    fx: document.getElementById('bfx').value, fy: document.getElementById('bfy').value,
    fz: document.getElementById('bfz').value, tx: document.getElementById('btx').value,
    ty: document.getElementById('bty').value, tz: document.getElementById('btz').value,
    thickness: document.getElementById('bth').value,
    swing: document.getElementById('bsw').value}, function(d){
      if (d && d.ok){ log('set bone on ' + picked); pick(picked); }
    });
}
function defaultRig(){
  if (!need()) return;
  api('default-rig', {name: picked, height: 1.7}, function(d){
    if (d && d.ok){ log('fitted default frame on ' + picked); pick(picked); }
  });
}
function clearRig(){
  if (!need()) return;
  api('clear-rig', {name: picked}, function(){ pick(picked); });
}
function scrap(){
  if (!need()) return;
  api('scrap', {name: picked}, function(d){
    if (d && d.ok){
      log('deleted ' + picked);
      picked = null;
      document.getElementById('sheet').style.display = 'none';
      document.getElementById('empty').style.display = '';
      loadHier();
    }
  });
}
function pull(){
  api('pull', {wiring: document.getElementById('pullWire').value}, function(d){
    if (!d) return;
    log('pulled wire: fired ' + d.fired + (d.sounds.length ? ' sounds: ' + d.sounds.join(',') : ''));
  });
}
function publish(){
  api('publish', {folder: 'published'}, function(d){
    if (d && d.ok) log('published to ' + d.folder + '/ - copy kimia_world in and run play.sh');
  });
}
function bringIn(){
  api('bring-in', {file: document.getElementById('inFile').value,
    size: document.getElementById('inSize').value}, function(d){
      if (d && d.ok){ log('imported as ' + d.name); loadHier(); pick(d.name); }
    });
}

// --- Project ---
function loadLibrary(){
  api('library', {}, function(d){
    if (!d) return;
    var st = document.getElementById('stageList');
    st.innerHTML = '';
    (d.stages || []).forEach(function(name){
      var el = document.createElement('div');
      el.className = 'item' + (name === d.stage ? ' on' : '');
      var label = document.createElement('span');
      label.textContent = name;
      el.appendChild(label);
      if (name !== d.stage){
        var x = document.createElement('span');
        x.textContent = 'x';
        x.style.cssText = 'margin-left:auto;color:#9a9a9a';
        x.onclick = function(ev){
          ev.stopPropagation();
          api('drop-stage', {stage: name}, loadLibrary);
        };
        el.appendChild(x);
      }
      el.onclick = function(){
        api('go-stage', {stage: name}, function(r){
          if (r && r.ok){ picked = null; log('opened scene ' + name); loadHier(); loadLibrary(); }
        });
      };
      st.appendChild(el);
    });

    var bp = document.getElementById('bpList');
    bp.innerHTML = '';
    if (!(d.blueprints || []).length){
      bp.innerHTML = '<div class="hint">Select an object and Keep it, then ' +
        'stamp copies without setting it up again.</div>';
    }
    (d.blueprints || []).forEach(function(name){
      var el = document.createElement('div');
      el.className = 'item';
      var label = document.createElement('span');
      label.textContent = name;
      el.appendChild(label);
      var x = document.createElement('span');
      x.textContent = 'x';
      x.style.cssText = 'margin-left:auto;color:#9a9a9a';
      x.onclick = function(ev){
        ev.stopPropagation();
        api('forget', {blueprint: name}, loadLibrary);
      };
      el.appendChild(x);
      el.onclick = function(){
        api('stamp', {blueprint: name, x: 0, y: 0, z: 0}, function(r){
          if (r && r.ok){ log('stamped ' + r.name); loadHier(); pick(r.name); }
        });
      };
      bp.appendChild(el);
    });
  });
}
function keepBlueprint(){
  if (!need()) return;
  var as = document.getElementById('bpName').value || picked;
  api('keep', {name: picked, as: as}, function(d){
    if (d && d.ok){
      document.getElementById('bpName').value = '';
      log('kept ' + picked + ' as ' + d.name);
      loadLibrary();
    }
  });
}
function addStage(){
  var name = document.getElementById('newStage').value.trim();
  if (!name) return;
  api('add-stage', {stage: name}, function(d){
    if (d && d.ok){ document.getElementById('newStage').value = ''; loadLibrary(); }
  });
}

// --- Files the user dropped into the asset folder ---
var scanned = [];
var deepScan = 0;

function loadAssets(deep){
  deepScan = deep ? 1 : 0;
  document.getElementById('scanHint').textContent = deep ? 'reading models...' : '';
  api('assets', {deep: deep ? 1 : 0}, function(d){
    if (!d) return;
    scanned = d.assets || [];
    document.getElementById('scanHint').textContent =
      scanned.length + ' file' + (scanned.length === 1 ? '' : 's');
    var box = document.getElementById('assetList');
    box.innerHTML = '';
    if (!scanned.length){
      box.innerHTML = '<div class="hint">Copy models, images and sounds into ' +
        'the assets folder, then press List.</div>';
    }
    scanned.forEach(function(a){
      var el = document.createElement('div');
      el.className = 'item';
      var pip = document.createElement('span');
      pip.className = 'pip' + (a.kind === 'model' ? ' solid' : (a.kind === 'texture' ? ' moving' : ''));
      el.appendChild(pip);
      var label = document.createElement('span');
      label.textContent = a.file;
      label.style.cursor = 'pointer';
      label.title = 'open';
      label.onclick = function(){ useAsset(a); };
      el.appendChild(label);
      var tail = [];
      if (a.skeleton) tail.push(a.bones + ' bones');
      if (a.clips && a.clips.length) tail.push(a.clips.length + ' clips');
      if (a.note) tail.push('!');
      if (tail.length){
        var mark = document.createElement('span');
        mark.style.cssText = 'margin-left:auto;color:#9a9a9a;font-size:10px';
        mark.textContent = tail.join(' ');
        el.appendChild(mark);
      }
      var ren = document.createElement('button');
      ren.textContent = 'Rename';
      ren.onclick = function(){ renameAsset(a.file, label); };
      el.appendChild(ren);
      var del = document.createElement('button');
      del.textContent = 'x';
      del.className = 'bad';
      del.title = 'delete';
      del.onclick = function(){ deleteAsset(a.file); };
      el.appendChild(del);
      box.appendChild(el);
    });
    fillClipFiles();
  });
}

// Clicking a file does the obvious thing for its kind: a model comes into
// the scene, an image goes onto whatever is selected.
function useAsset(a){
  if (a.kind === 'model'){
    // Keep the scene portable: `file` is relative to the configured assets
    // folder. `path` is only the server's filesystem location.
    api('bring-in', {file: a.file, size: 1}, function(d){
      if (d && d.ok){ log('imported ' + d.name); loadHier(); pick(d.name); }
    });
    return;
  }
  if (a.kind === 'texture'){
    if (!picked){ flash('select an object first, then click an image', true); return; }
    api('skin', {name: picked, image: a.file}, function(d){
      if (d && d.ok) log('painted ' + a.file + ' onto ' + picked);
    });
    return;
  }
  flash(a.file + ' is a sound - use it in a control or a rule');
}

// Inline rename: the name becomes a field; Enter commits, Escape cancels.
function renameAsset(file, label){
  var input = document.createElement('input');
  input.value = file.indexOf('/') < 0 ? file : file.substr(file.lastIndexOf('/') + 1);
  input.style.cssText = 'flex:1;min-width:40px';
  var done = false;
  function cancel(){
    if (done) return;
    done = true;
    input.parentNode.replaceChild(label, input);
  }
  function commit(){
    if (done) return;
    done = true;
    var to = input.value.trim();
    input.parentNode.replaceChild(label, input);
    if (!to || to === file) return;
    api('asset/rename', {file: file, to: to}, function(d){
      if (d && d.ok) log('renamed ' + file + ' to ' + to);
      loadAssets(deepScan);
    });
  }
  input.onkeydown = function(e){
    if (e.key === 'Enter') commit();
    else if (e.key === 'Escape') cancel();
    e.stopPropagation();
  };
  input.onblur = cancel;
  label.parentNode.replaceChild(input, label);
  input.focus();
  input.select();
}
function deleteAsset(file){
  if (!confirm('Delete ' + file + '?')) return;
  api('asset/delete', {file: file}, function(d){
    if (d && d.ok){ log('deleted ' + file); loadAssets(deepScan); }
  });
}
// Upload posts the raw bytes; the name rides in the query. No overwrite:
// a taken name is refused, rename or delete it first.
function uploadAsset(){
  var picker = document.getElementById('upFile');
  picker.onchange = function(){
    if (!picker.files || !picker.files.length) return;
    var f = picker.files[0];
    log('uploading ' + f.name + ' (' + f.size + ' bytes)...');
    fetch('/api/asset/upload?name=' + encodeURIComponent(f.name), {method: 'POST', body: f})
      .then(function(r){ return r.json(); })
      .then(function(d){
        picker.value = '';
        if (d && d.ok){ log('uploaded ' + d.file); loadAssets(deepScan); }
        else {
          flash((d && d.error) || 'upload refused', true);
          log('upload refused: ' + ((d && d.error) || '?'));
        }
      })
      .catch(function(){ picker.value = ''; flash('engine not answering', true); });
  };
  picker.click();
}

// --- Rules ---
var pickedRule = -1;

function showRules(){
  document.getElementById('rulesSheet').classList.add('show');
  loadRules();
  loadPanels();
  loadEffects();
  loadControls();
}
function hideRules(){ document.getElementById('rulesSheet').classList.remove('show'); }

function loadRules(){
  api('rules', {}, function(d){
    if (!d || !d.rules) return;
    var box = document.getElementById('ruleList');
    box.innerHTML = '';
    if (!d.rules.length){
      box.innerHTML = '<div class="hint">No rules yet. A game is a list of ' +
        '"when this happens, do that".</div>';
    }
    d.rules.forEach(function(r){
      var el = document.createElement('div');
      el.className = 'rule' + (r.index === pickedRule ? ' on' : '') + (r.enabled ? '' : ' off');
      var head = document.createElement('div');
      head.textContent = r.reads;
      el.appendChild(head);
      var tools = document.createElement('div');
      tools.className = 'tools';
      function tool(label, fn){
        var b = document.createElement('button');
        b.textContent = label;
        b.onclick = function(ev){ ev.stopPropagation(); fn(); };
        tools.appendChild(b);
      }
      tool(r.enabled ? 'off' : 'on', function(){
        api('toggle-rule', {index: r.index, on: r.enabled ? 0 : 1}, loadRules); });
      tool('^', function(){ api('move-rule', {index: r.index, dir: 'up'}, loadRules); });
      tool('v', function(){ api('move-rule', {index: r.index, dir: 'down'}, loadRules); });
      tool('x', function(){ api('drop-rule', {index: r.index}, function(){
        pickedRule = -1; loadRules(); }); });
      el.appendChild(tools);
      el.onclick = function(){
        pickedRule = r.index;
        document.getElementById('pickedRule').textContent = r.name || ('#' + r.index);
        loadRules();
      };
      box.appendChild(el);
    });

    var vs = document.getElementById('varList');
    vs.innerHTML = '';
    (d.variables || []).forEach(function(v){
      var el = document.createElement('div');
      el.className = 'wire';
      var left = document.createElement('i');
      left.textContent = v.name;
      var right = document.createElement('span');
      right.textContent = v.isText ? v.text : v.number.toFixed(2);
      el.appendChild(left);
      el.appendChild(right);
      el.style.cursor = 'pointer';
      el.onclick = function(){ api('drop-var', {variable: v.name}, loadRules); };
      vs.appendChild(el);
    });

    var state = d.finished ? (d.won ? 'game won' : 'game lost') : 'running';
    if (d.message) state += '  -  "' + d.message + '"';
    document.getElementById('logicState').textContent = state;
  });
}

function addRule(){
  api('add-rule', {rulename: document.getElementById('rName').value || 'rule',
    trigger: document.getElementById('rTrigger').value,
    subject: document.getElementById('rSubject').value,
    other: document.getElementById('rOther').value,
    number: document.getElementById('rNumber').value}, function(d){
      if (d && d.ok){
        pickedRule = d.index;
        document.getElementById('rName').value = '';
        log('rule added - now give it an action');
        loadRules();
      }
    });
}
function needRule(){
  if (pickedRule < 0){ flash('pick a rule first', true); return false; }
  return true;
}
function addCondition(){
  if (!needRule()) return;
  api('add-condition', {index: pickedRule, variable: document.getElementById('cVar').value,
    compare: document.getElementById('cCmp').value,
    number: document.getElementById('cNum').value}, function(d){
      if (d && d.ok){ log('condition added'); loadRules(); }
    });
}
function addAction(){
  if (!needRule()) return;
  api('add-action', {index: pickedRule, act: document.getElementById('aAct').value,
    target: document.getElementById('aTarget').value,
    text: document.getElementById('aText').value,
    number: document.getElementById('aNum').value,
    ax: document.getElementById('aax').value, ay: document.getElementById('aay').value,
    az: document.getElementById('aaz').value}, function(d){
      if (d && d.ok){ log('action added'); loadRules(); }
    });
}
function loadPanels(){
  api('panels', {}, function(d){
    if (!d) return;
    var box = document.getElementById('panelList');
    box.innerHTML = '';
    if (!(d.panels || []).length){
      box.innerHTML = '<div class="hint">Nothing on screen yet. A label ' +
        'showing {score}, or a health bar, is a good start.</div>';
    }
    (d.panels || []).forEach(function(p){
      var el = document.createElement('div');
      el.className = 'wire';
      el.style.cursor = 'pointer';
      var what = p.kind === 'bar' ? ('bar of ' + p.variable) : (p.text || p.kind);
      var left = document.createElement('i');
      left.textContent = p.name;
      var right = document.createElement('span');
      right.textContent = what;
      el.appendChild(left);
      el.appendChild(right);
      el.onclick = function(){
        // Load it back into the fields so editing is a tweak.
        document.getElementById('pName').value = p.name;
        document.getElementById('pKind').value = p.kind;
        document.getElementById('pText').value = p.text;
        document.getElementById('pVar').value = p.variable;
        document.getElementById('pMax').value = p.maximum;
        document.getElementById('pEvent').value = p.event;
        document.getElementById('pX').value = p.x.toFixed(3);
        document.getElementById('pY').value = p.y.toFixed(3);
        document.getElementById('pW').value = p.w.toFixed(3);
        document.getElementById('pH').value = p.h.toFixed(3);
      };
      var x = document.createElement('b');
      x.textContent = 'x';
      x.style.cssText = 'cursor:pointer;margin-left:8px;color:#9a9a9a';
      x.onclick = function(ev){
        ev.stopPropagation();
        api('drop-panel', {panel: p.name}, loadPanels);
      };
      el.appendChild(x);
      box.appendChild(el);
    });
  });
}
function setPanel(){
  var c = document.getElementById('pColor').value;
  var b = document.getElementById('pBack').value;
  var hexPart = function(v, at){ return parseInt(v.substr(at, 2), 16) / 255; };
  api('set-panel', {panel: document.getElementById('pName').value,
    kind: document.getElementById('pKind').value,
    text: document.getElementById('pText').value,
    variable: document.getElementById('pVar').value,
    maximum: document.getElementById('pMax').value,
    event: document.getElementById('pEvent').value,
    x: document.getElementById('pX').value, y: document.getElementById('pY').value,
    w: document.getElementById('pW').value, h: document.getElementById('pH').value,
    r: hexPart(c, 1), g: hexPart(c, 3), b: hexPart(c, 5),
    br: hexPart(b, 1), bg: hexPart(b, 3), bb: hexPart(b, 5)}, function(d){
      if (d && d.ok){ log('panel placed'); loadPanels(); }
    });
}

function loadEffects(){
  api('effects', {}, function(d){
    if (!d) return;
    var box = document.getElementById('fxList');
    box.innerHTML = '';
    if (!(d.effects || []).length){
      box.innerHTML = '<div class="hint">No effects yet. An explosion or a ' +
        'puff of dust makes a game read as a game.</div>';
    }
    (d.effects || []).forEach(function(fx){
      var el = document.createElement('div');
      el.className = 'wire';
      el.style.cursor = 'pointer';
      var left = document.createElement('i');
      left.textContent = fx.name;
      var right = document.createElement('span');
      right.textContent = fx.count + ' x ' + fx.life.toFixed(1) + 's';
      el.appendChild(left);
      el.appendChild(right);
      el.onclick = function(){
        document.getElementById('fxName').value = fx.name;
        document.getElementById('fxCount').value = fx.count;
        document.getElementById('fxLife').value = fx.life;
        document.getElementById('fxSpeed').value = fx.speed;
        document.getElementById('fxSpread').value = fx.spread;
        document.getElementById('fxGrav').value = fx.gravity;
        document.getElementById('fxSize').value = fx.size;
      };
      box.appendChild(el);
    });
  });
}
function setEffect(){
  var f = document.getElementById('fxFrom').value;
  var t = document.getElementById('fxTo').value;
  var part = function(v, at){ return parseInt(v.substr(at, 2), 16) / 255; };
  api('set-effect', {effect: document.getElementById('fxName').value,
    count: document.getElementById('fxCount').value,
    life: document.getElementById('fxLife').value,
    speed: document.getElementById('fxSpeed').value,
    spread: document.getElementById('fxSpread').value,
    gravity: document.getElementById('fxGrav').value,
    size: document.getElementById('fxSize').value,
    r: part(f, 1), g: part(f, 3), b: part(f, 5),
    r2: part(t, 1), g2: part(t, 3), b2: part(t, 5)}, function(d){
      if (d && d.ok){ log('effect saved'); loadEffects(); }
    });
}
function fireEffect(){
  api('fire-effect', {effect: document.getElementById('fxName').value, x: 0, y: 1, z: 0},
    function(d){ if (d && d.ok) log(d.live + ' particles in flight'); });
}

function fillClipFiles(){
  var sel = document.getElementById('cClipFile');
  if (!sel) return;
  var keep = sel.value;
  sel.innerHTML = '<option value="">-- model --</option>';
  scanned.forEach(function(a){
    if (a.kind !== 'model' || !a.clips || !a.clips.length) return;
    var o = document.createElement('option');
    // Controls store the asset-relative name, never an absolute server path.
    o.value = a.file;
    o.textContent = a.file + ' (' + a.clips.length + ')';
    sel.appendChild(o);
  });
  sel.value = keep;
  clipsOf(sel.value);
}
function clipsOf(path){
  var sel = document.getElementById('cClip');
  if (!sel) return;
  sel.innerHTML = '<option value="">-- none --</option>';
  scanned.forEach(function(a){
    if (a.file !== path) return;
    (a.clips || []).forEach(function(c){
      var o = document.createElement('option');
      o.value = c;
      o.textContent = c;
      sel.appendChild(o);
    });
  });
}

// --- Controls ---
function loadControls(){
  api('controls', {}, function(d){
    if (!d) return;
    document.getElementById('cStick').checked = !!d.stick;
    var box = document.getElementById('ctrlList');
    box.innerHTML = '';
    if (!(d.controls || []).length){
      box.innerHTML = '<div class="hint">No controls yet. One action can be ' +
        'a key, a screen button and a pad button at once.</div>';
    }
    (d.controls || []).forEach(function(c){
      var el = document.createElement('div');
      el.className = 'wire';
      el.style.cursor = 'pointer';
      var how = (c.bindings || []).map(function(b){
        return b.source === 'touch' ? 'screen' : (b.source + ':' + b.code);
      }).join(' ');
      var left = document.createElement('i');
      left.textContent = c.name + (c.clip ? ' -> ' + c.clip : '');
      var right = document.createElement('span');
      right.textContent = how || 'unbound';
      el.appendChild(left);
      el.appendChild(right);
      el.onclick = function(){
        document.getElementById('cName').value = c.name;
        document.getElementById('cLabel').value = c.label;
        document.getElementById('cX').value = c.x.toFixed(2);
        document.getElementById('cY').value = c.y.toFixed(2);
        document.getElementById('cSize').value = c.size.toFixed(2);
        document.getElementById('cSound').value = c.sound;
        document.getElementById('cClipFile').value = c.clipFile;
        clipsOf(c.clipFile);
        document.getElementById('cClip').value = c.clip;
        document.getElementById('cTarget').value = c.target || '';
        var key = '', pad = '', touch = false;
        (c.bindings || []).forEach(function(b){
          if (b.source === 'key') key = b.code;
          if (b.source === 'pad') pad = b.code;
          if (b.source === 'touch') touch = true;
        });
        document.getElementById('cKey').value = key;
        document.getElementById('cPad').value = pad;
        document.getElementById('cTouch').checked = touch;
      };
      var x = document.createElement('b');
      x.textContent = 'x';
      x.style.cssText = 'cursor:pointer;margin-left:8px;color:#9a9a9a';
      x.onclick = function(ev){
        ev.stopPropagation();
        api('drop-control', {control: c.name}, loadControls);
      };
      el.appendChild(x);
      box.appendChild(el);
    });
  });
}
function setControl(){
  api('set-control', {control: document.getElementById('cName').value,
    label: document.getElementById('cLabel').value,
    key: document.getElementById('cKey').value,
    pad: document.getElementById('cPad').value,
    touch: document.getElementById('cTouch').checked ? 1 : 0,
    x: document.getElementById('cX').value, y: document.getElementById('cY').value,
    size: document.getElementById('cSize').value,
    clipfile: document.getElementById('cClipFile').value,
    clip: document.getElementById('cClip').value,
    target: document.getElementById('cTarget').value,
    sound: document.getElementById('cSound').value}, function(d){
      if (d && d.ok){ log('control saved'); loadControls(); }
    });
}
function doControl(){
  api('do', {control: document.getElementById('cName').value}, function(d){
    if (d && d.ok) log('fired control');
  });
}
function setStick(){
  api('stick', {on: document.getElementById('cStick').checked ? 1 : 0}, function(){});
}

function setVar(){
  api('set-var', {variable: document.getElementById('vName').value,
    number: document.getElementById('vNum').value}, function(d){
      if (d && d.ok) loadRules();
    });
}

// --- Touching the picture ---
// A tap selects; a drag edits with the active tool. The same handlers
// serve mouse and touch, because the editor has to work on a phone and
// on a desktop without two code paths.
(function(){
  var view = document.getElementById('view');
  var dragging = false, lastX = 0, lastY = 0, moved = 0, downAt = 0;

  // The image is scaled to fit, so a screen pixel is not a frame pixel.
  function toFrame(ev){
    var r = view.getBoundingClientRect();
    var p = (ev.touches && ev.touches[0]) ? ev.touches[0] : ev;
    return {
      x: (p.clientX - r.left) / r.width * (view.naturalWidth || r.width),
      y: (p.clientY - r.top) / r.height * (view.naturalHeight || r.height)
    };
  }

  function begin(ev){
    var p = toFrame(ev);
    dragging = true; lastX = p.x; lastY = p.y; moved = 0; downAt = Date.now();
    ev.preventDefault();
  }
  function move(ev){
    if (!dragging) return;
    var p = toFrame(ev);
    var dx = p.x - lastX, dy = p.y - lastY;
    moved += Math.abs(dx) + Math.abs(dy);
    // Only edit once the finger has really moved, or every tap would
    // nudge the object slightly.
    if (picked && moved > 6 && tool !== 'select'){
      if (tool === 'move'){
        api('drag', {name: picked, fromx: lastX, fromy: lastY, tox: p.x, toy: p.y,
                     grid: snapGrid()}, function(d){
          if (d && d.ok && d.position){
            document.getElementById('px').value = d.position[0].toFixed(2);
            document.getElementById('py').value = d.position[1].toFixed(2);
            document.getElementById('pz').value = d.position[2].toFixed(2);
          }
        });
      } else if (tool === 'rotate'){
        // Sideways turns the table, up/down tilts; snapping steps 15 degrees.
        var dyaw = dx * 0.4, dpitch = -dy * 0.4;
        if (document.getElementById('snapOn').checked){
          dyaw = Math.round(dyaw / 15) * 15;
          dpitch = Math.round(dpitch / 15) * 15;
        }
        if (dyaw || dpitch){
          api('object/rotate', {name: picked, dyaw: dyaw, dpitch: dpitch}, function(d){
            if (d && d.ok && d.rotation){
              document.getElementById('rx').value = d.rotation[0].toFixed(1);
              document.getElementById('ry').value = d.rotation[1].toFixed(1);
              document.getElementById('rz').value = d.rotation[2].toFixed(1);
            }
          });
        }
      } else if (tool === 'scale'){
        var factor = 1 - dy * 0.004;
        if (factor > 0.05 && factor < 20){
          api('object/scale', {name: picked, factor: factor}, function(d){
            if (d && d.ok && d.scale){
              document.getElementById('sx').value = d.scale[0].toFixed(2);
              document.getElementById('sy').value = d.scale[1].toFixed(2);
              document.getElementById('sz').value = d.scale[2].toFixed(2);
            }
          });
        }
      }
      lastX = p.x; lastY = p.y;
    }
    ev.preventDefault();
  }
  function end(ev){
    if (!dragging) return;
    dragging = false;
    // A short press that barely moved is a TAP: select what is under it.
    if (moved <= 6 && Date.now() - downAt < 600){
      api('tap', {x: lastX, y: lastY}, function(d){
        if (!d) return;
        if (d.name){
          pick(d.name);
        } else {
          document.getElementById('tapHint').textContent = 'nothing there';
        }
      });
    }
    if (ev.cancelable) ev.preventDefault();
  }

  view.addEventListener('mousedown', begin);
  view.addEventListener('mousemove', move);
  window.addEventListener('mouseup', end);
  view.addEventListener('touchstart', begin, {passive: false});
  view.addEventListener('touchmove', move, {passive: false});
  view.addEventListener('touchend', end, {passive: false});
})();

// Keyboard: Q/W/E/R switch tools, Ctrl+D duplicates, Delete removes.
// Typing in a field never triggers these.
document.addEventListener('keydown', function(e){
  var tag = (e.target && e.target.tagName) || '';
  if (tag === 'INPUT' || tag === 'SELECT' || tag === 'TEXTAREA') return;
  if ((e.ctrlKey || e.metaKey) && (e.key === 'd' || e.key === 'D')){
    e.preventDefault();
    duplicate();
    return;
  }
  if (e.key === 'Delete' || e.key === 'Backspace'){ scrap(); return; }
  var k = (e.key || '').toLowerCase();
  if (k === 'q') setTool('select');
  else if (k === 'w') setTool('move');
  else if (k === 'e') setTool('rotate');
  else if (k === 'r') setTool('scale');
  else if (k === ' '){ e.preventDefault(); transportPlay(); }
});

setInterval(function(){
  document.getElementById('view').src = '/frame.jpg?t=' + Date.now();
}, 500);
setInterval(function(){
  api('pulse', {}, function(d){
    if (!d) return;
    var s = d.stats || '';
    if (d.clips && d.clips.length) s += '   > ' + d.clips.join(' ');
    document.getElementById('conStat').textContent = s;
  });
  pollTransport();
}, 1000);
if (window.innerWidth <= 900) document.getElementById('hierBtn').style.display = '';
setTool('select');
loadHier();
loadLibrary();
loadAssets(0);
pollTransport();
log('KIMIA editor ready');
</script>
</body>
</html>)BENCH";
}

}  // namespace studio
}  // namespace kimia

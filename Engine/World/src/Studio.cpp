#include <kimia/AssetPipeline.h>
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
f64 entitySpan(const EntityData& entity) {
  const Vec3& s = entity.transform.scale;
  f64 largest = std::max(std::abs(s.x), std::max(std::abs(s.y), std::abs(s.z)));
  if (entity.meshFile.empty()) return largest;
  std::string error;
  auto loaded = assets::loadMesh(entity.meshFile, error);
  if (!loaded.has_value() || loaded->mesh.positions.empty()) return largest;
  Vec3 lo = loaded->mesh.positions[0];
  Vec3 hi = lo;
  for (const Vec3& p : loaded->mesh.positions) {
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
std::string dossierJson(const EntityData& entity) {
  std::string out = "{";
  out += "\"name\":" + quoted(entity.name);
  out += ",\"mesh\":" + quoted(entity.meshFile);
  out += ",\"position\":" + vec3Json(entity.transform.position);
  out += ",\"scale\":" + vec3Json(entity.transform.scale);
  // A raw scale multiplier is meaningless for an imported model: after
  // bring-in auto-fits the file, "3" means three times the original, not
  // three units across. The Bench shows this measured size instead.
  out += ",\"span\":" + number(entitySpan(entity));
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
    return "{\"ok\":true,\"dossier\":" + dossierJson(*entity) + "}";
  }

  // Everything carrying a label — the point of labels is addressing a
  // GROUP, so the Bench has to be able to show that group.
  if (path == "/api/labelled") {
    return "{\"ok\":true,\"items\":" + stringsJson(editor.entitiesWithTag(param(params, "label"))) + "}";
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
    out += ",\"clips\":" + stringsJson(editor.playingAnimations());
    out += ",\"stats\":" + quoted(editor.statsLine());
    return out + "}";
  }

  return errorJson("unknown request: " + path);
}


std::string benchPage() {
  // One self-contained document: no external files, so the Bench works
  // offline on a phone exactly as it does on a desktop.
  return R"BENCH(<!doctype html>
<html lang="fa" dir="rtl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>KIMIA Workbench</title>
<style>
:root{
  --steel:#1b1f24; --steel2:#22272e; --edge:#333b45; --ink:#e8edf2;
  --dim:#8b97a5; --brass:#d9a441; --weld:#4bb3a5; --hot:#e2574c;
}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;background:var(--steel);color:var(--ink);
  font:14px/1.5 ui-monospace,"DejaVu Sans Mono",Menlo,monospace;overflow:hidden}
#bench{display:grid;height:100vh;
  grid-template-columns:230px 1fr 300px;
  grid-template-rows:44px 1fr 26px;
  grid-template-areas:"top top top" "rack stage dossier" "strip strip strip"}
@media(max-width:900px){
  #bench{grid-template-columns:1fr;
    grid-template-rows:44px 38vh 1fr 26px;
    grid-template-areas:"top" "stage" "dossier" "strip"}
  #rack{display:none}
  #rack.show{display:block;position:fixed;inset:44px 0 26px 0;z-index:20}
}
#top{grid-area:top;display:flex;align-items:center;gap:10px;padding:0 12px;
  background:var(--steel2);border-bottom:1px solid var(--edge)}
.brand{font-weight:700;letter-spacing:.14em;color:var(--brass)}
.brand small{color:var(--dim);font-weight:400;letter-spacing:0}
#rack{grid-area:rack;background:var(--steel2);border-left:1px solid var(--edge);
  overflow:auto;padding:8px}
#stage{grid-area:stage;position:relative;background:#0e1114;display:flex;
  align-items:center;justify-content:center;overflow:hidden}
#stage img{max-width:100%;max-height:100%;image-rendering:pixelated}
#dossier{grid-area:dossier;background:var(--steel2);border-right:1px solid var(--edge);
  overflow:auto;padding:10px}
#strip{grid-area:strip;background:#12161a;border-top:1px solid var(--edge);
  color:var(--dim);font-size:11px;padding:4px 12px;white-space:nowrap;overflow:hidden}
h2{font-size:11px;letter-spacing:.18em;color:var(--dim);margin:14px 0 6px;
  text-transform:uppercase;font-weight:600}
h2:first-child{margin-top:0}
.row{display:flex;gap:6px;align-items:center;margin-bottom:6px}
.row label{color:var(--dim);min-width:52px;font-size:12px}
button{background:#2c333b;color:var(--ink);border:1px solid var(--edge);
  border-radius:5px;padding:6px 10px;cursor:pointer;font:inherit;font-size:12px}
button:hover{border-color:var(--brass)}
button.go{background:var(--weld);border-color:var(--weld);color:#08201d;font-weight:700}
button.bad{background:#3a2320;border-color:#5c332e;color:#f0b5ae}
input,select{background:#141a1f;color:var(--ink);border:1px solid var(--edge);
  border-radius:5px;padding:5px 7px;font:inherit;font-size:12px;width:100%;min-width:0}
input[type=color]{padding:2px;height:30px}
.item{padding:6px 8px;border:1px solid transparent;border-radius:5px;cursor:pointer;
  display:flex;align-items:center;gap:6px;font-size:12px}
.item:hover{background:#2a3039}
.item.on{background:#2d3742;border-color:var(--brass)}
.pip{width:7px;height:7px;border-radius:50%;background:#4a545f;flex:none}
.pip.solid{background:var(--brass)}
.pip.moving{background:var(--weld)}
.tag{display:inline-flex;align-items:center;gap:4px;background:#2c333b;
  border:1px solid var(--edge);border-radius:11px;padding:2px 8px;font-size:11px;margin:0 0 4px 4px}
.tag b{cursor:pointer;color:var(--dim)}
.tag b:hover{color:var(--hot)}
.grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:4px}
.wire{background:#1a2026;border:1px solid var(--edge);border-radius:5px;
  padding:5px 7px;margin-bottom:4px;font-size:11px;display:flex;justify-content:space-between}
.wire span{color:var(--brass)}
.hint{color:var(--dim);font-size:11px;line-height:1.6}
#flash{position:fixed;bottom:34px;left:50%;transform:translateX(-50%);
  background:#12161a;border:1px solid var(--brass);border-radius:6px;
  padding:7px 14px;font-size:12px;opacity:0;transition:opacity .2s;pointer-events:none;z-index:50}
#flash.on{opacity:1}
#flash.err{border-color:var(--hot);color:#f0b5ae}
</style>
</head>
<body>
<div id="bench">
  <div id="top">
    <div class="brand">KIMIA <small>Workbench</small></div>
    <button onclick="toggleRack()" id="rackBtn" style="display:none">Rack</button>
    <div style="flex:1"></div>
    <span class="hint" id="worldName">-</span>
    <button onclick="location.href='/'">Play &rsaquo;</button>
  </div>

  <div id="rack">
    <h2>Rack</h2>
    <div id="rackList"></div>
    <h2>Bring in</h2>
    <div class="row"><input id="inFile" placeholder="assets/thing.obj"></div>
    <div class="row"><label>size</label><input id="inSize" type="number" value="1" step="0.1"></div>
    <button class="go" style="width:100%" onclick="bringIn()">Bring in</button>
  </div>

  <div id="stage"><img id="view" alt="world"></div>

  <div id="dossier">
    <div id="empty" class="hint">Pick something from the Rack to open its Dossier.</div>
    <div id="sheet" style="display:none">
      <h2>Dossier</h2>
      <div class="row"><b id="dName"></b></div>
      <div class="hint" id="dMesh"></div>

      <h2>Placement</h2>
      <div class="grid3">
        <input id="px" type="number" step="0.1" title="x">
        <input id="py" type="number" step="0.1" title="y">
        <input id="pz" type="number" step="0.1" title="z">
      </div>
      <div class="grid3" style="margin-top:4px">
        <input id="sx" type="number" step="0.1" title="width">
        <input id="sy" type="number" step="0.1" title="height">
        <input id="sz" type="number" step="0.1" title="depth">
      </div>
      <div class="row" style="margin-top:6px">
        <input id="tint" type="color">
        <button class="go" onclick="place()">Set</button>
      </div>

      <h2>Fittings &mdash; Body</h2>
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
      <button class="go" style="width:100%" onclick="fitBody()">Bolt on</button>

      <h2>Labels</h2>
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
      <div class="hint">Pull a wire to fire it here, without leaving the Bench.</div>

      <h2>&nbsp;</h2>
      <button class="bad" style="width:100%" onclick="scrap()">Scrap this object</button>
    </div>
  </div>

  <div id="strip">booting&hellip;</div>
</div>
<div id="flash"></div>

<script>
var picked = null;

function flash(msg, bad){
  var f = document.getElementById('flash');
  f.textContent = msg;
  f.className = bad ? 'on err' : 'on';
  clearTimeout(f.timer);
  f.timer = setTimeout(function(){ f.className = ''; }, 1800);
}
function api(path, params, done){
  var q = [];
  for (var k in params) q.push(encodeURIComponent(k) + '=' + encodeURIComponent(params[k]));
  fetch('/api/' + path + (q.length ? '?' + q.join('&') : ''))
    .then(function(r){ return r.json(); })
    .then(function(d){
      if (d && d.ok === false) flash(d.error || 'refused', true);
      if (done) done(d);
    })
    .catch(function(){ flash('engine not answering', true); });
}
function toggleRack(){ document.getElementById('rack').classList.toggle('show'); }
function hex(c){
  var n = Math.max(0, Math.min(255, Math.round(c * 255))).toString(16);
  return n.length < 2 ? '0' + n : n;
}

function loadRack(){
  api('rack', {}, function(d){
    if (!d || !d.items) return;
    document.getElementById('worldName').textContent = d.world || '';
    var box = document.getElementById('rackList');
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
        tail.style.cssText = 'margin-right:auto;color:#8b97a5;font-size:10px';
        tail.textContent = marks.join(' ');
        row.appendChild(tail);
      }
      row.onclick = function(){ pick(it.name); };
      box.appendChild(row);
    });
  });
}

function pick(name){
  picked = name;
  api('dossier', {name: name}, function(d){
    if (!d || !d.dossier) return;
    var o = d.dossier;
    document.getElementById('empty').style.display = 'none';
    document.getElementById('sheet').style.display = '';
    document.getElementById('dName').textContent = o.name;
    document.getElementById('dMesh').textContent =
      (o.mesh ? o.mesh : 'built-in shape') + '   \u2194 ' + o.span.toFixed(2) + 'm';
    var ids = ['px','py','pz'], sids = ['sx','sy','sz'];
    for (var i = 0; i < 3; i++){
      document.getElementById(ids[i]).value = o.position[i].toFixed(2);
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
      x.textContent = '\u00d7';
      x.onclick = function(){ api('unlabel', {name: picked, label: t}, function(){ pick(picked); loadRack(); }); };
      chip.appendChild(x);
      lab.appendChild(chip);
    });
    var bv = document.getElementById('bones');
    bv.innerHTML = '';
    (o.bones || []).forEach(function(b){
      var w = document.createElement('div');
      w.className = 'wire';
      w.style.cursor = 'pointer';
      var span = b.parent ? (b.name + ' \u2190 ' + b.parent) : b.name;
      w.innerHTML = '<i>' + span + '</i><span>' + b.swing.toFixed(1) + '</span>';
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
      w.innerHTML = '<i>' + m.clip + '</i><span>&larr; ' + m.wiring + '</span>';
      mv.appendChild(w);
    });
    var nv = document.getElementById('noises');
    nv.innerHTML = '';
    o.noises.forEach(function(n){
      var w = document.createElement('div');
      w.className = 'wire';
      w.innerHTML = '<i>' + n.sound + '</i><span>&larr; ' + n.wiring + '</span>';
      nv.appendChild(w);
    });
    loadRack();
  });
}

function need(){ if (!picked) { flash('pick something first', true); return false; } return true; }

function place(){
  if (!need()) return;
  var c = document.getElementById('tint').value;
  api('place', {name: picked,
    px: document.getElementById('px').value, py: document.getElementById('py').value,
    pz: document.getElementById('pz').value, sx: document.getElementById('sx').value,
    sy: document.getElementById('sy').value, sz: document.getElementById('sz').value},
    function(){
      api('paint', {name: picked,
        r: parseInt(c.substr(1,2),16)/255, g: parseInt(c.substr(3,2),16)/255,
        b: parseInt(c.substr(5,2),16)/255}, function(){ flash('placed'); pick(picked); });
    });
}
function fitBody(){
  if (!need()) return;
  api('fit-body', {name: picked, kind: document.getElementById('bKind').value,
    mass: document.getElementById('bMass').value, friction: document.getElementById('bFric').value,
    bounce: document.getElementById('bBounce').value}, function(d){
      if (d && d.ok) flash('bolted on');
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
      if (d && d.ok) { flash('wired'); pick(picked); }
    });
}
function wireNoise(){
  if (!need()) return;
  api('wire-noise', {name: picked, sound: document.getElementById('nSound').value,
    wiring: document.getElementById('nWire').value}, function(d){
      if (d && d.ok) { flash('wired'); pick(picked); }
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
      if (d && d.ok) { flash('bone set'); pick(picked); }
    });
}
function defaultRig(){
  if (!need()) return;
  api('default-rig', {name: picked, height: 1.7}, function(d){
    if (d && d.ok) { flash('default frame fitted \u2014 now edit it'); pick(picked); }
  });
}
function clearRig(){
  if (!need()) return;
  api('clear-rig', {name: picked}, function(){ pick(picked); });
}
function pull(){
  api('pull', {wiring: document.getElementById('pullWire').value}, function(d){
    if (!d) return;
    flash('fired ' + d.fired + (d.sounds.length ? '  sounds: ' + d.sounds.join(',') : ''));
  });
}
function bringIn(){
  api('bring-in', {file: document.getElementById('inFile').value,
    size: document.getElementById('inSize').value}, function(d){
      if (d && d.ok) { flash('brought in as ' + d.name); loadRack(); pick(d.name); }
    });
}
function scrap(){
  if (!need()) return;
  api('scrap', {name: picked}, function(d){
    if (d && d.ok){
      picked = null;
      document.getElementById('sheet').style.display = 'none';
      document.getElementById('empty').style.display = '';
      loadRack();
    }
  });
}

setInterval(function(){
  document.getElementById('view').src = '/frame.png?t=' + Date.now();
}, 500);
setInterval(function(){
  api('pulse', {}, function(d){
    if (!d) return;
    var s = d.stats || '';
    if (d.clips && d.clips.length) s += '   \u25b8 ' + d.clips.join(' ');
    document.getElementById('strip').textContent = s;
  });
}, 1000);
if (window.innerWidth <= 900) document.getElementById('rackBtn').style.display = '';
loadRack();
</script>
</body>
</html>)BENCH";
}

}  // namespace studio
}  // namespace kimia

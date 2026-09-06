// KIMIA World — the option-driven editor / object builder (spec section 8).
//
//   kimia_world [--port N] [--world <file.kimia>] [--assets DIR] [--profiles DIR]
//
// Start with an EMPTY ground and build your game with menus only: add a
// player, a ball, blocks, walls, goals — each object asks a few plain
// questions («دقیق باشه یا فانتزی؟») — then manage them (move/delete/color)
// and press PLAY. Worlds save as SceneIO-v1-compatible text.
#include <kimia/Audio.h>
#include <kimia/BitmapFont.h>
#include <kimia/Engine.h>
#include <kimia/Image.h>
#include <kimia/MathUtils.h>
#include <kimia/Mesh.h>
#include <kimia/Renderer.h>
#include <kimia/WebViewer.h>
#include <kimia/AssetPipeline.h>
#include <kimia/OrbitCamera.h>
#include <kimia/Hud.h>
#include <kimia/Picking.h>
#include <kimia/Skeleton.h>
#include <kimia/Studio.h>
#include <kimia/Version.h>
#include <kimia/World.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

using kimia::Engine;
using kimia::EngineOptions;
using kimia::EntityData;
using kimia::Image;
using kimia::Key;
using kimia::Mat4;
using kimia::MeshData;
using kimia::ObjectKind;
using kimia::RenderScene;
using kimia::Renderer;
using kimia::Vec3;
using kimia::WorldEditor;
using kimia::clamp;
using kimia::f64;
using kimia::i32;
using kimia::u8;
using kimia::u16;
using kimia::usize;

namespace {

std::atomic<bool> running{true};

void onSignal(int) { running.store(false); }

const Vec3 kGhostColor{1.0, 0.85, 0.2};
const Vec3 kSelectionColor{1.0, 0.9, 0.25};
const Vec3 kHudText{1.0, 1.0, 1.0};
const Vec3 kHudBackdrop{0.0, 0.0, 0.0};
const Vec3 kPowerFill{1.0, 0.55, 0.1};
const Vec3 kPowerBack{0.15, 0.15, 0.15};
constexpr i32 kHudScale = 2;   // 10x14 pixel glyphs
constexpr i32 kHudMargin = 8;

// The on-frame HUD: game lines top-left, the power meter bottom-centre.
// Drawn into the captured frame, so it looks the same on GL and software.
void drawHud(Image& image, const WorldEditor& editor) {
  const std::vector<std::string> lines = editor.hudLines();
  if (!lines.empty()) {
    i32 widest = 0;
    for (const std::string& line : lines) widest = std::max(widest, kimia::font::textWidth(line, kHudScale));
    const i32 lineStep = kimia::font::textHeight(kHudScale) + kHudScale * 2;
    const i32 boxWidth = widest + kHudMargin * 2;
    const i32 boxHeight = static_cast<i32>(lines.size()) * lineStep + kHudMargin * 2 - kHudScale * 2;
    kimia::font::fillRect(image, kHudMargin, kHudMargin, boxWidth, boxHeight, kHudBackdrop, 0.55);
    for (usize i = 0; i < lines.size(); ++i) {
      kimia::font::drawText(image, kHudMargin * 2, kHudMargin * 2 + static_cast<i32>(i) * lineStep, lines[i],
                            kHudText, kHudScale);
    }
  }
  const f64 power = editor.hudPower();
  if (power >= 0.0) {
    const i32 barWidth = std::min(240, image.width - kHudMargin * 2);
    const i32 barHeight = 14;
    const i32 labelHeight = kimia::font::textHeight(kHudScale);
    const i32 x = (image.width - barWidth) / 2;
    const i32 y = image.height - kHudMargin - barHeight;
    kimia::font::fillRect(image, x - 4, y - 8 - labelHeight, barWidth + 8, barHeight + labelHeight + 12, kHudBackdrop,
                          0.55);
    kimia::font::drawText(image, x, y - 4 - labelHeight, "POWER", kHudText, kHudScale);
    kimia::font::drawBar(image, x, y, barWidth, barHeight, power, kPowerFill, kPowerBack);
  }
}

// Sound cues are procedural (no asset files): registered once with the web
// server, cued by name when the world reports an event.
void registerSounds(kimia::web::Server& server) {
  using kimia::AudioBuffer;
  server.registerSound("shot", AudioBuffer::thock(0.12, 1400.0).encodeWAV());
  server.registerSound("kick", AudioBuffer::thock(0.16, 700.0).encodeWAV());
  server.registerSound("holed",
                       AudioBuffer::concat(AudioBuffer::tone(660.0, 0.12), AudioBuffer::tone(990.0, 0.25)).encodeWAV());
  server.registerSound("goal", AudioBuffer::tone(440.0, 0.5, 0.6, 880.0).encodeWAV());
  server.registerSound(
      "round", AudioBuffer::concat(AudioBuffer::concat(AudioBuffer::tone(523.25, 0.15), AudioBuffer::tone(659.25, 0.15)),
                                   AudioBuffer::tone(783.99, 0.35))
                   .encodeWAV());
  // Stage 28. A referee's whistle is a shrill held note; a tackle is a
  // duller, lower thock than a clean kick; a completed trick is a bright
  // little rising flourish.
  server.registerSound("whistle", AudioBuffer::tone(2100.0, 0.30, 0.5, 2400.0).encodeWAV());
  server.registerSound("tackle", AudioBuffer::thock(0.20, 320.0).encodeWAV());
  server.registerSound("trick",
                       AudioBuffer::concat(AudioBuffer::tone(880.0, 0.09), AudioBuffer::tone(1318.5, 0.16))
                           .encodeWAV());
}

const char* soundFor(WorldEditor::GameEvent event) {
  switch (event) {
    case WorldEditor::GameEvent::Shot: return "shot";
    case WorldEditor::GameEvent::Kick: return "kick";
    case WorldEditor::GameEvent::Holed: return "holed";
    case WorldEditor::GameEvent::Goal: return "goal";
    case WorldEditor::GameEvent::RoundOver: return "round";
    case WorldEditor::GameEvent::Whistle: return "whistle";
    case WorldEditor::GameEvent::Tackle: return "tackle";
    case WorldEditor::GameEvent::Trick: return "trick";
  }
  return "shot";
}

// Shortest signed angle from `from` to `to` (radians).
f64 angleDelta(f64 from, f64 to) {
  f64 delta = std::fmod(to - from + kimia::kPi, 2.0 * kimia::kPi);
  if (delta < 0.0) delta += 2.0 * kimia::kPi;
  return delta - kimia::kPi;
}

// A single-entity goal (scale.x = width, scale.y = height) drawn as two
// posts and a crossbar.
void addGoalShape(RenderScene& scene, const EntityData& entity, const MeshData& cube) {
  const f64 width = entity.transform.scale.x;
  const f64 half = entity.transform.scale.y * 0.5;
  const Vec3 at = entity.transform.position;
  const Vec3 color = entity.color;
  scene.objects.push_back(
      {&cube, Mat4::translation(Vec3{at.x - width * 0.5 + 0.06, at.y, at.z}) *
                  Mat4::scaling(Vec3{0.12, entity.transform.scale.y, 0.12}),
       color, entity.roughness});
  scene.objects.push_back(
      {&cube, Mat4::translation(Vec3{at.x + width * 0.5 - 0.06, at.y, at.z}) *
                  Mat4::scaling(Vec3{0.12, entity.transform.scale.y, 0.12}),
       color, entity.roughness});
  scene.objects.push_back(
      {&cube, Mat4::translation(Vec3{at.x, at.y + half, at.z}) * Mat4::scaling(Vec3{width + 0.12, 0.12, 0.12}),
       color, entity.roughness});
}

void addSelectionMarkers(RenderScene& scene, const EntityData& entity, const MeshData& cube) {
  const Vec3 half = entity.transform.scale * 0.5;
  const Vec3 at = entity.transform.position;
  const f64 marker = 0.06;
  for (i32 sx = -1; sx <= 1; sx += 2) {
    for (i32 sy = -1; sy <= 1; sy += 2) {
      for (i32 sz = -1; sz <= 1; sz += 2) {
        const Vec3 corner{at.x + half.x * static_cast<f64>(sx), at.y + half.y * static_cast<f64>(sy),
                          at.z + half.z * static_cast<f64>(sz)};
        scene.objects.push_back(
            {&cube, Mat4::translation(corner) * Mat4::scaling(Vec3{marker, marker, marker}),
             kSelectionColor, 0.9});
      }
    }
  }
}

void addGhostShape(RenderScene& scene, const WorldEditor& editor, const MeshData& cube,
                   const MeshData& sphere) {
  const Vec3 ghost = editor.ghostPosition();
  const f64 size = editor.ghostSize();
  switch (editor.ghostKind()) {
    case ObjectKind::Player: {
      scene.objects.push_back({&cube, Mat4::translation(Vec3{ghost.x, 0.5, ghost.z}) *
                                          Mat4::scaling(Vec3{0.6, 1.0, 0.6}),
                               kGhostColor, 0.9});
      scene.objects.push_back({&cube, Mat4::translation(Vec3{ghost.x, 1.15, ghost.z}) *
                                          Mat4::scaling(Vec3{0.3, 0.3, 0.3}),
                               kGhostColor, 0.9});
      break;
    }
    case ObjectKind::Ball: {
      const f64 radius = editor.world().ball.radius;
      scene.objects.push_back({&sphere, Mat4::translation(Vec3{ghost.x, radius, ghost.z}) *
                                            Mat4::scaling(Vec3{radius, radius, radius}),
                               kGhostColor, 0.9});
      break;
    }
    case ObjectKind::Block:
    case ObjectKind::Crate:
    case ObjectKind::Model:
      scene.objects.push_back({&cube, Mat4::translation(Vec3{ghost.x, size * 0.5, ghost.z}) *
                                          Mat4::scaling(Vec3{size, size, size}),
                               kGhostColor, 0.9});
      break;
    case ObjectKind::Wall:
      scene.objects.push_back(
          {&cube, Mat4::translation(Vec3{ghost.x, 0.5, ghost.z}) *
                      Mat4::scaling(editor.ghostAxisZ() ? Vec3{0.5, 1.0, size} : Vec3{size, 1.0, 0.5}),
           kGhostColor, 0.9});
      break;
    case ObjectKind::Goal: {
      EntityData preview;
      preview.transform.position = Vec3{ghost.x, kimia::kWorldGoalHeight * 0.5, ghost.z};
      preview.transform.scale = Vec3{size, kimia::kWorldGoalHeight, 0.12};
      preview.color = kGhostColor;
      preview.roughness = 0.9;
      addGoalShape(scene, preview, cube);
      break;
    }
    case ObjectKind::Hole: {
      // The cup preview: a flat disc (a squashed sphere), drawn slightly
      // above the ground so it is visible while placing.
      const f64 radius = kimia::kWorldHoleRadius;
      scene.objects.push_back({&sphere, Mat4::translation(Vec3{ghost.x, 0.03, ghost.z}) *
                                            Mat4::scaling(Vec3{radius, 0.03, radius}),
                               kGhostColor, 0.9});
      break;
    }
    default:
      break;
  }
}

// Shot mode: a chain of small markers along the aim direction on the ground.
// The chain grows with the charge, like the reference golf's indicator.
void addAimIndicator(RenderScene& scene, const WorldEditor& editor, const MeshData& cube) {
  if (!editor.shotMode() || !editor.playing() || !editor.ballAtRest()) return;
  const Vec3 from = editor.ballPosition();
  const Vec3 direction = editor.aimDirection();
  const f64 reach = 1.0 + (editor.charging() ? editor.power() : 0.0) * 4.0;
  const i32 count = 6;
  for (i32 i = 1; i <= count; ++i) {
    const f64 t = static_cast<f64>(i) / static_cast<f64>(count);
    const Vec3 at = from + direction * (reach * t);
    const f64 marker = 0.05 + 0.02 * (1.0 - t);
    scene.objects.push_back(
        {&cube, Mat4::translation(Vec3{at.x, marker * 0.5, at.z}) * Mat4::scaling(Vec3{marker, marker, marker}),
         kGhostColor, 0.9});
  }
}

// The squads (stage 21) were spawned in physics but never actually DRAWN,
// which nobody noticed while they stood still. Now that stage 27 has them
// running about, an invisible opposition makes a match unplayable. Each
// character is a coloured box: our side in blue, theirs in red.
// Lays a set of limb segments into the scene as stretched cubes.
void addLimbs(RenderScene& scene, const MeshData& cube, const std::vector<kimia::FigureLimb>& limbs,
              const Vec3& color) {
  for (const kimia::FigureLimb& limb : limbs) {
    const Vec3 along = limb.to - limb.from;
    const f64 length = along.length();
    if (length < 1e-4) continue;
    const Vec3 middle = limb.from + along * 0.5;
    const Vec3 up{0.0, 1.0, 0.0};
    const Vec3 dir = along * (1.0 / length);
    const f64 dot = up.x * dir.x + up.y * dir.y + up.z * dir.z;
    Mat4 orient;
    if (dot < 0.9999) {
      if (dot < -0.9999) {
        orient = Mat4::rotationX(3.14159265358979323846);
      } else {
        const Vec3 axis{up.y * dir.z - up.z * dir.y, up.z * dir.x - up.x * dir.z, up.x * dir.y - up.y * dir.x};
        orient = kimia::Quat::fromAxisAngle(axis, std::acos(dot)).toMat4();
      }
    }
    scene.objects.push_back({&cube,
                             Mat4::translation(middle) * orient *
                                 Mat4::scaling(Vec3{limb.thickness, length, limb.thickness}),
                             color, 1.0, nullptr});
  }
}

// One rig shared by everyone: it is a fixed shape, so building it per
// character per frame would be waste.
const kimia::Skeleton& figureRig() {
  static const kimia::Skeleton rig = kimia::makeFigureRig(1.7);
  return rig;
}

// Draws one posed figure as a set of limb segments. Each segment is a
// stretched cube laid along the bone, which is all the software rasteriser
// needs and reads far better than the single box these used to be.
void addFigure(RenderScene& scene, const MeshData& cube, const kimia::Skeleton& rig,
               const kimia::FigureMotion& motion, const Vec3& at, f64 facing, const Vec3& color) {
  static std::vector<kimia::Transform3D> pose;
  static std::vector<kimia::FigureLimb> limbs;
  kimia::poseFigure(rig, motion, pose);
  kimia::figureLimbs(rig, pose, at, facing, limbs);
  addLimbs(scene, cube, limbs, color);
}

// Draws a character using bones the user drew themselves.
void addCustomFigure(RenderScene& scene, const MeshData& cube, const std::vector<kimia::CustomBone>& bones,
                     const kimia::FigureMotion& motion, const Vec3& at, f64 facing, const Vec3& color) {
  static std::vector<kimia::FigureLimb> limbs;
  kimia::customFigureLimbs(bones, motion, at, facing, limbs);
  addLimbs(scene, cube, limbs, color);
}

// The bones a squad member should use, or null for the engine's figure.
// A character's rig is read off the scene entity that represents it: the
// human uses "Player", and everyone else falls back to a "Squad" entity if
// the world defines one, so a whole team can be re-boned in one go.
const std::vector<kimia::CustomBone>* customRigFor(const WorldEditor& editor, kimia::u32 id) {
  static std::vector<kimia::CustomBone> converted;
  const char* wanted = id == kimia::kPrimaryCharacter ? "Player" : "Squad";
  const kimia::EntityData* entity = editor.entity(wanted);
  if (entity == nullptr && id == kimia::kPrimaryCharacter) return nullptr;
  if (entity == nullptr || entity->rig.empty()) {
    entity = editor.entity("Player");
    if (entity == nullptr || entity->rig.empty()) return nullptr;
  }
  converted.clear();
  converted.reserve(entity->rig.size());
  for (const kimia::RigBone& bone : entity->rig) {
    kimia::CustomBone out;
    out.name = bone.name;
    out.parent = bone.parent;
    out.from = bone.from;
    out.to = bone.to;
    out.thickness = bone.thickness;
    out.swing = bone.swing;
    converted.push_back(out);
  }
  return &converted;
}

void addSquads(RenderScene& scene, const WorldEditor& editor, const MeshData& cube) {
  if (!editor.playing() || editor.squadCount() <= 1U) return;
  const Vec3 ourColor{0.25, 0.45, 0.95};
  const Vec3 theirColor{0.90, 0.25, 0.25};
  const Vec3 keeperColor{0.95, 0.85, 0.20};  // the keeper stands out
  for (const kimia::u32 id : editor.squadIds()) {
    // The human is already drawn as the Player entity in the scene.
    if (id == kimia::kPrimaryCharacter) continue;
    const Vec3 at = editor.squadPosition(id);
    const kimia::u32 team = editor.squadTeam(id);
    Vec3 color = team == 1U ? ourColor : theirColor;
    const bool down = editor.arenaMode() && editor.downed(id);
    if (down) {
      color = Vec3{0.45, 0.45, 0.45};
    } else if (!editor.arenaMode() && id == editor.aiKeeper(team)) {
      color = keeperColor;
    }
    // A jointed figure, not a sliding box (stage 33).
    kimia::FigureMotion motion;
    motion.speed = editor.squadSpeed(id);
    motion.time = editor.figureClock();
    motion.airborne = editor.squadAirborne(id);
    motion.downed = down;
    // The feet belong on the floor: the body position is its centre.
    const Vec3 feet{at.x, at.y - kimia::kWorldPlayerRadius - 0.15, at.z};
    // A character with bones of its own uses them (stage 35). The engine's
    // figure is only the fallback for anyone who has not drawn one.
    const std::vector<kimia::CustomBone>* own = customRigFor(editor, id);
    if (own != nullptr) {
      addCustomFigure(scene, cube, *own, motion, feet, editor.squadFacing(id), color);
    } else {
      addFigure(scene, cube, figureRig(), motion, feet, editor.squadFacing(id), color);
    }
  }
}

// Course: a small flag pole on the cup being played, so the player can see
// which cup is next (the others are plain discs). Nothing on a finished round.
void addCurrentCupFlag(RenderScene& scene, const WorldEditor& editor, const MeshData& cube) {
  if (!editor.holeScoring() || !editor.playing() || editor.roundOver()) return;
  const kimia::EntityData* cup = editor.world().scene.get(editor.world().scene.find(editor.currentHoleName()));
  if (cup == nullptr) return;
  const Vec3 base = cup->transform.position;
  const f64 poleHeight = 1.2;
  scene.objects.push_back({&cube, Mat4::translation(Vec3{base.x, poleHeight * 0.5, base.z}) *
                                      Mat4::scaling(Vec3{0.04, poleHeight, 0.04}),
                           Vec3{0.92, 0.92, 0.92}, 0.6});
  scene.objects.push_back({&cube, Mat4::translation(Vec3{base.x + 0.16, poleHeight - 0.12, base.z}) *
                                      Mat4::scaling(Vec3{0.3, 0.2, 0.02}),
                           Vec3{0.9, 0.15, 0.1}, 0.8});
}

}  // namespace

namespace {

void printUsage() {
  std::printf(
      "%s — the option-driven game maker (everything is menus)\n"
      "\n"
      "usage: kimia_world [options]\n"
      "  --port N          web port to serve the game on (default 8080)\n"
      "  --world FILE      world file to save/load (default my_world.kimia)\n"
      "  --assets DIR      OBJ/FBX files you can place in a scene (default assets)\n"
      "  --profiles DIR    *.kimiaprofile game files (default profiles;\n"
      "                    the built-in games always work without this)\n"
      "  --branding DIR    folder with kimia-intro.mp4 / kimia-logo.png\n"
      "                    (default: Branding next to the app or the build)\n"
      "  --no-intro        do not play the intro film\n"
      "  --version         print the engine version and exit\n"
      "  --help            print this text and exit\n"
      "\n"
      "then open http://127.0.0.1:<port> in a browser and tap the menus.\n",
      kimia::kEngineVersionString);
}

}  // namespace

int main(int argc, char** argv) {
  int port = 8080;
  std::string worldPath = "my_world.kimia";
  std::string assetsDir = "assets";
  std::string profilesDir = "profiles";
  std::string brandingDir;  // empty = look in Branding, ../Branding, ../../Branding
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--port" && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if (arg == "--world" && i + 1 < argc) {
      worldPath = argv[++i];
    } else if (arg == "--assets" && i + 1 < argc) {
      assetsDir = argv[++i];
    } else if (arg == "--profiles" && i + 1 < argc) {
      profilesDir = argv[++i];
    } else if (arg == "--branding" && i + 1 < argc) {
      brandingDir = argv[++i];
    } else if (arg == "--no-intro") {
      brandingDir = "-";  // a folder that cannot exist: skips the film
    } else if (arg == "--version") {
      std::printf("%s\n", kimia::kEngineVersionString);
      return 0;
    } else if (arg == "--help" || arg == "-h") {
      printUsage();
      return 0;
    } else {
      // Never a silent no-op: a typo like `--porrt 9000` used to be ignored
      // and the game quietly ran on the wrong port. Say so and stop.
      std::printf("kimia_world: unknown or incomplete option: %s\n\n", arg.c_str());
      printUsage();
      return 2;
    }
  }

  WorldEditor editor;
  // The Workbench API runs on the server's accept thread while this loop
  // is updating the world, so both sides take this lock. Without it a
  // request landing mid-update would be a genuine data race.
  std::mutex editorMutex;
  editor.setWorldPath(worldPath);
  editor.setImportDirectory(assetsDir);
  editor.setProfileDirectory(profilesDir);  // built-ins + *.kimiaprofile files

  EngineOptions options;
  options.headless = true;
  options.enableWeb = true;
  options.webPort = static_cast<u16>(port);
  options.windowTitle = "KIMIA World";
  Engine engine;
  if (!engine.initialize(options)) {
    std::printf("engine init failed\n");
    return 1;
  }
  if (engine.server() == nullptr) {
    std::printf("web server failed to start\n");
    return 1;
  }

  // Physical-keyboard keymap: 1-6 pick options, arrows move, Shift = fine,
  // r/b actions.
  const char* keymapJs =
      "var km={'1':'t:num1','2':'t:num2','3':'t:num3','4':'t:num4','5':'t:num5','6':'t:num6',"
      "'7':'t:num7','8':'t:num8','9':'t:num9',"
      "'r':'t:r','b':'t:b','j':'t:j',' ':'h:space','ArrowUp':'h:up','ArrowDown':'h:down','ArrowLeft':'h:left',"
      "'ArrowRight':'h:right','Shift':'h:shift',"
      "'c':'h:c','q':'h:q','e':'h:e','p':'t:p'};\n"
      "function kmd(e,down){var m=km[e.key];if(!m)return;e.preventDefault();"
      "if(m[0]==='h')post('key='+m.slice(2)+'&down='+(down?1:0));else if(down)post('tap='+m.slice(2));}\n"
      "window.addEventListener('keydown',function(e){kmd(e,true);});\n"
      "window.addEventListener('keyup',function(e){kmd(e,false);});";

  engine.server()->stop();
  engine.server()->start(options.webPort, kimia::web::makePageHtml(
      "KIMIA World", {}, keymapJs,
      "everything is menus: tap 1-9 for the options, arrows move, Shift = fine, r resets, b opens the menu, "
      "Space = jump (or hold to charge a shot), hold c = dribble, hold q/e = curl, p = pass"));
  // The intro film, if the Branding folder shipped with this build.
  const bool intro = kimia::web::loadIntroFrom(*engine.server(), brandingDir);
  std::printf("KIMIA World %s serving on port %d | GL: %s | games: %d\n", kimia::kEngineVersion,
              static_cast<i32>(engine.server()->port()), engine.glAvailable() ? "yes" : "no (software)",
              static_cast<i32>(editor.profileCount()));
  std::printf("intro: %s\n", intro ? "yes" : "no (no Branding/kimia-intro.mp4)");

  Renderer renderer;
  std::string rendererError;
  if (engine.glAvailable() && !renderer.initialize(rendererError)) {
    std::printf("renderer init failed: %s\n", rendererError.c_str());
  }
  registerSounds(*engine.server());

  // --- KIMIA Workbench (stage 32) ---
  // The editor page and the API behind it. The server hands requests to
  // the studio layer, which asks the WorldEditor real questions — so
  // every decision stays in the engine where it is tested, and the page
  // is only ever a view of it.
  //
  // The handler runs on the server's accept thread while the main loop is
  // updating the world, so it takes the same lock the frame loop uses.
  engine.server()->setPage("/bench", kimia::studio::benchPage());
  engine.server()->setApiHandler(
      [&editor, &editorMutex](const std::string& path, const std::map<std::string, std::string>& params) {
        std::lock_guard<std::mutex> lock(editorMutex);
        return kimia::studio::handleApi(editor, path, params);
      });

  const MeshData cubeMesh = kimia::makeCube(1.0);
  const MeshData planeMesh = kimia::makePlane(1.0, 1.0);
  const MeshData sphereMesh = kimia::makeSphere(16, 8);
  const i32 width = 640;
  const i32 height = 480;

  std::signal(SIGINT, onSignal);
  std::map<std::string, kimia::MeshData> loadedMeshes;  // meshFile -> mesh
  // Diffuse textures, keyed by the same mesh file (stage 34). An entry with
  // an empty image means "this model has no texture" — cached too, so a
  // model without one is not re-examined every frame.
  std::map<std::string, kimia::Image> loadedTextures;
  kimia::OrbitCamera orbitCamera;  // arrow keys orbit, q/e zoom, c resets
  // The distance the player chose by hand. A broadcast camera moves
  // orbitCamera.distance around every frame, so the manual zoom is
  // remembered here and used as the resting point to work from.
  f64 restingCameraDistance = orbitCamera.distance;
  const auto frameStart = std::chrono::steady_clock::now();
  auto lastTime = frameStart;
  const std::chrono::microseconds frameBudget(33333);  // ~30 fps over the web

  while (running.load() && !editor.quitRequested()) {
    const auto now = std::chrono::steady_clock::now();
    f64 dt = static_cast<f64>(std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count()) /
             1000000.0;
    lastTime = now;
    dt = clamp(dt, 0.0, 0.1);

    if (!engine.poll()) break;
    kimia::InputState& input = engine.input();

    if (input.pressed(Key::Escape)) break;

    // Guards the world while this frame reads and steps it. It is released
    // before the server is touched below: the server takes its own lock,
    // and the API handler takes this one, so holding both at once here
    // would be a lock-order inversion. It deadlocked the first time it ran.
    std::unique_lock<std::mutex> editorLock(editorMutex);
    if (input.pressed(Key::Num1)) editor.choose(0);
    if (input.pressed(Key::Num2)) editor.choose(1);
    if (input.pressed(Key::Num3)) editor.choose(2);
    if (input.pressed(Key::Num4)) editor.choose(3);
    if (input.pressed(Key::Num5)) editor.choose(4);
    if (input.pressed(Key::Num6)) editor.choose(5);
    if (input.pressed(Key::Num7)) editor.choose(6);
    if (input.pressed(Key::Num8)) editor.choose(7);
    if (input.pressed(Key::Num9)) editor.choose(8);
    if (input.pressed(Key::R)) editor.resetBall();
    if (input.pressed(Key::B)) editor.backToMenu();
    if (editor.shotMode()) {
      // Golf-style: hold Space (the «شوت» pad) to charge, release to shoot.
      editor.setShootHeld(input.down(Key::Space));
    } else {
      editor.setShootHeld(false);
      if (input.pressed(Key::J) || input.pressed(Key::Space)) editor.jumpPressed();
    }
    // Ball control (stage 23): hold C to dribble, hold Q/E to curl the next
    // strike left/right, tap P to pass to the nearest team-mate ahead.
    editor.setDribbleHeld(input.down(Key::C));
    f64 curl = 0.0;
    if (input.down(Key::Q)) curl -= 1.0;
    if (input.down(Key::E)) curl += 1.0;
    if (curl != 0.0) editor.setCurl(curl);
    if (input.pressed(Key::P)) editor.pass();

    // --- Keys for the rules and the wiring ---
    // Every letter key is reported to the logic by name, so a rule saying
    // "when key k" works with no engine change, and a component wired to
    // "k" fires at the same moment.
    static const std::pair<Key, const char*> kNamedKeys[] = {
        {Key::A, "a"}, {Key::B, "b"}, {Key::C, "c"}, {Key::D, "d"}, {Key::E, "e"}, {Key::F, "f"},
        {Key::G, "g"}, {Key::H, "h"}, {Key::I, "i"}, {Key::J, "j"}, {Key::K, "k"}, {Key::L, "l"},
        {Key::M, "m"}, {Key::N, "n"}, {Key::O, "o"}, {Key::P, "p"}, {Key::Q, "q"}, {Key::R, "r"},
        {Key::S, "s"}, {Key::T, "t"}, {Key::U, "u"}, {Key::V, "v"}, {Key::W, "w"}, {Key::X, "x"},
        {Key::Y, "y"}, {Key::Z, "z"}, {Key::Space, "space"}, {Key::Up, "up"}, {Key::Down, "down"},
        {Key::Left, "left"}, {Key::Right, "right"}, {Key::Return, "return"},
    };
    std::vector<std::string> pressedNames;
    std::vector<std::string> heldNames;
    for (const auto& binding : kNamedKeys) {
      if (input.pressed(binding.first)) {
        pressedNames.push_back(binding.second);
        editor.fireTrigger(binding.second);  // components wired to this key
      }
      if (input.down(binding.first)) heldNames.push_back(binding.second);
    }
    editor.setLogicKeys(pressedNames, heldNames);
    // Skill moves (stage 26): tap N to nutmeg, O to roulette, U to juggle.
    // They are taps because you commit to them — there is no holding back
    // half way through a nutmeg.
    // Arena mode (stage 30): hold F to fire, tap R to reload. The football
    // keys below are harmless there — there is no ball to kick.
    if (editor.arenaMode()) {
      editor.setFireHeld(input.down(Key::F));
      if (input.pressed(Key::R)) editor.reload();
    }
    if (input.pressed(Key::N)) editor.startTrick(WorldEditor::Trick::Nutmeg);
    if (input.pressed(Key::O)) editor.startTrick(WorldEditor::Trick::Roulette);
    if (input.pressed(Key::U)) editor.startTrick(WorldEditor::Trick::Juggle);

    f64 moveX = 0.0;
    f64 moveZ = 0.0;
    if (input.down(Key::Left)) moveX -= 1.0;
    if (input.down(Key::Right)) moveX += 1.0;
    if (input.down(Key::Up)) moveZ -= 1.0;
    if (input.down(Key::Down)) moveZ += 1.0;
    editor.setMoveInput(moveX, moveZ);
    editor.setFineMove(input.down(Key::Shift));
    if (editor.cameraControlled()) {
      // Orbit the camera with the arrows (and the mouse on desktop); the
      // same pads drive the ghost/player in placing, moving and playing.
      orbitCamera.orbit((moveX * 1.1 + input.lookX * 0.006) * dt,
                        (-moveZ * 0.9 + input.lookY * 0.006) * dt);
      if (input.pressed(Key::Q)) orbitCamera.zoom(1.0 / 1.2);
      if (input.pressed(Key::E)) orbitCamera.zoom(1.2);
      if (input.pressed(Key::C)) orbitCamera.reset();
      restingCameraDistance = orbitCamera.distance;  // remember the hand-set zoom
    } else {
      orbitCamera.orbit(input.lookX * 0.006, input.lookY * 0.006);
    }

    editor.update(dt);
    for (const WorldEditor::GameEvent event : editor.drainEvents()) {
      engine.server()->playSound(soundFor(event));
      // A component bound to "goal" or "kick" in the editor fires here,
      // without any game code knowing that component exists (stage 31).
      editor.fireTrigger(WorldEditor::eventTriggerName(event));
    }
    // Sounds queued by those components.
    for (const std::string& sound : editor.drainTriggeredSounds()) engine.server()->playSound(sound);

    // --- Build the frame ---
    const kimia::EnvironmentColors colors = kimia::environmentColors(editor.world().environment);
    RenderScene scene;
    editor.world().scene.forEach([&](kimia::EntityHandle, const EntityData& entity) {
      const ObjectKind kind = kimia::objectKindForName(entity.name);
      if (kind == ObjectKind::Goal && !kimia::isLegacyGoalPart(entity.name)) {
        addGoalShape(scene, entity, cubeMesh);
        return;
      }
      const MeshData* mesh = &cubeMesh;
      if (entity.mesh == kimia::MeshKind::plane) mesh = &planeMesh;
      if (entity.mesh == kimia::MeshKind::sphere) mesh = &sphereMesh;
      const kimia::Image* texture = nullptr;
      if (!entity.meshFile.empty()) {
        // Model entity: load the OBJ/FBX once, then draw it every frame.
        auto found = loadedMeshes.find(entity.meshFile);
        if (found == loadedMeshes.end()) {
          std::string loadError;
          auto loaded = kimia::assets::loadMesh(entity.meshFile, loadError);
          if (loaded.has_value()) {
            found = loadedMeshes.emplace(entity.meshFile, std::move(loaded->mesh)).first;
          }
        }
        if (found != loadedMeshes.end()) mesh = &found->second;
        else return;  // mesh missing/unreadable: skip this entity

        // Its texture, once (stage 34). The importer has always pulled the
        // diffuse map's path out of the .mtl or the FBX materials, but
        // nothing ever loaded the image — so every model rendered as a
        // flat colour however carefully it was textured.
        auto skin = loadedTextures.find(entity.meshFile);
        if (skin == loadedTextures.end()) {
          kimia::Image image;
          std::string assetError;
          auto asset = kimia::assets::loadMeshAsset(entity.meshFile, assetError);
          if (asset.has_value()) {
            for (const kimia::MaterialData& material : asset->materials) {
              if (material.texturePath.empty()) continue;
              auto loadedImage = kimia::assets::loadImage(material.texturePath, assetError);
              if (loadedImage.has_value()) {
                image = std::move(*loadedImage);
                break;
              }
            }
          }
          skin = loadedTextures.emplace(entity.meshFile, std::move(image)).first;
        }
        if (skin->second.width > 0 && skin->second.height > 0) texture = &skin->second;
      }
      // Crates follow the physics bodies while playing, and the player
      // entity follows the character controller so the play character is
      // actually visible where the physics puts it (including mid-jump).
      const bool playCharacter = kind == ObjectKind::Player && editor.playing();
      const Vec3 position =
          kind == ObjectKind::Crate
              ? editor.cratePosition(entity.name)
              : (playCharacter ? editor.playerPosition() : entity.transform.position);
      const Vec3 scale = entity.mesh == kimia::MeshKind::sphere ? entity.transform.scale * 0.5
                                                                : entity.transform.scale;
      const Mat4 model = Mat4::translation(position) * Mat4::scaling(scale);
      scene.objects.push_back({mesh, model, entity.color, entity.roughness, texture});
      if (kind == ObjectKind::Player) {
        // A little head so the player reads as a character.
        scene.objects.push_back(
            {&cubeMesh, Mat4::translation(position + Vec3{0.0, 0.65, 0.0}) *
                            Mat4::scaling(Vec3{0.3, 0.3, 0.3}),
             entity.color, entity.roughness});
      }
    });
    // The ball follows the physics body.
    const f64 ballRadius = editor.world().ball.radius;
    scene.objects.push_back(
        {&sphereMesh, Mat4::translation(editor.ballPosition()) * Mat4::scaling(Vec3{ballRadius, ballRadius, ballRadius}),
         editor.world().ball.color, 0.3});
    // Ghost preview while placing, selection markers while managing, the
    // aim chain in shot mode.
    if (editor.placing()) addGhostShape(scene, editor, cubeMesh, sphereMesh);
    addSquads(scene, editor, cubeMesh);
    // Arena tracer: a thin line along the last shot, so a firefight is
    // readable instead of invisible.
    if (editor.arenaMode() && editor.playing()) {
      const Vec3 from = editor.lastShotFrom();
      const Vec3 to = editor.lastShotTo();
      const Vec3 along = to - from;
      const f64 length = along.length();
      if (length > 0.01) {
        const i32 beads = 12;
        for (i32 i = 1; i <= beads; ++i) {
          const f64 t = static_cast<f64>(i) / static_cast<f64>(beads + 1);
          const Vec3 at = from + along * t;
          scene.objects.push_back({&cubeMesh, Mat4::translation(at) * Mat4::scaling(Vec3{0.05, 0.05, 0.05}),
                                   Vec3{1.0, 0.9, 0.4}, 0.9});
        }
      }
    }
    addAimIndicator(scene, editor, cubeMesh);
    addCurrentCupFlag(scene, editor, cubeMesh);
    if (editor.selectingObject() && editor.selectedEntity() != nullptr) {
      addSelectionMarkers(scene, *editor.selectedEntity(), cubeMesh);
    }

    // Camera: above the ghost while placing/moving, above the ball in play,
    // an overview of the field otherwise; the orbit offset persists.
    // The engine decides where to look and how far back to stand (stage
    // 28), so the same framing is testable and identical on every path.
    if (editor.cameraFollowsAim()) {
      // Chase camera: ease around behind the aim. A look drag still peeks
      // around; the camera settles back on its own.
      orbitCamera.yaw +=
          angleDelta(orbitCamera.yaw, editor.aimYaw()) * std::min(1.0, kimia::kCameraFollowRate * dt);
    }
    orbitCamera.center = editor.cameraTarget();
    // A broadcast camera pulls back as the play spreads out; ease toward it
    // so the zoom never snaps.
    const f64 wantedDistance = editor.cameraDistance(restingCameraDistance);
    orbitCamera.distance += (wantedDistance - orbitCamera.distance) * std::min(1.0, kimia::kCameraFollowRate * dt);
    const Vec3 eye = orbitCamera.eye();
    scene.cameraPosition = eye;
    scene.view = Mat4::lookAt(eye, orbitCamera.target(), Vec3{0.0, 1.0, 0.0});
    scene.projection = Mat4::perspective(kimia::radians(60.0), static_cast<f64>(width) / static_cast<f64>(height),
                                         0.1, 100.0);
    scene.lightDirection = Vec3{-0.4, -0.8, -0.4};

    // Hand the camera to the engine so a tap on the picture can be turned
    // into an object. The app owns the camera; the engine owns the
    // decision about what was hit, where it can be tested.
    {
      kimia::pick::Viewport viewport;
      viewport.view = scene.view;
      viewport.projection = scene.projection;
      viewport.eye = eye;
      viewport.width = width;
      viewport.height = height;
      editor.setViewport(viewport);
    }

    Image image;
    if (renderer.ready()) {
      renderer.render(scene, width, height);
      if (!renderer.captureImage(width, height, image)) image = Image{};
    }
    if (image.isEmpty()) kimia::renderSoftware(scene, width, height, colors.clear, image);
    drawHud(image, editor);
    // The interface the USER laid out, over the engine's own corner text.
    // Drawn second so a panel can deliberately sit on top of it.
    kimia::drawHud(image, editor.hud(), editor.logic());
    std::vector<u8> png = image.encodePNG();

    // --- Menu (buttons the user sees) ---
    kimia::web::Menu menu;
    menu.title = editor.menuTitle();
    const std::vector<std::string> labels = editor.optionLabels();
    for (usize i = 0; i < labels.size(); ++i) {
      menu.taps.push_back({labels[i], "num" + std::to_string(i + 1U)});
    }
    for (const auto& pad : editor.holdPad()) menu.holds.push_back({pad.first, pad.second});
    for (const auto& pad : editor.tapPad()) menu.taps.push_back({pad.first, pad.second});

    const std::string stats = editor.statsLine();
    editorLock.unlock();  // never hold the world lock while calling the server
    engine.server()->publishFrame(std::move(png), stats);
    engine.server()->setMenu(menu);
    engine.endFrame();

    const auto elapsed = std::chrono::steady_clock::now() - now;
    const auto left = frameBudget - std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    if (left.count() > 0) std::this_thread::sleep_for(left);
  }

  std::printf("bye | %s\n", editor.statsLine().c_str());
  return 0;
}

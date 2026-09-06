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
#include <kimia/Version.h>
#include <kimia/World.h>

#include <algorithm>
#include <atomic>
#include <chrono>
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
constexpr f64 kChaseFollowRate = 6.0;  // 1/s: how fast the camera swings behind the aim

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
}

const char* soundFor(WorldEditor::GameEvent event) {
  switch (event) {
    case WorldEditor::GameEvent::Shot: return "shot";
    case WorldEditor::GameEvent::Kick: return "kick";
    case WorldEditor::GameEvent::Holed: return "holed";
    case WorldEditor::GameEvent::Goal: return "goal";
    case WorldEditor::GameEvent::RoundOver: return "round";
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
      "'ArrowRight':'h:right','Shift':'h:shift'};\n"
      "function kmd(e,down){var m=km[e.key];if(!m)return;e.preventDefault();"
      "if(m[0]==='h')post('key='+m.slice(2)+'&down='+(down?1:0));else if(down)post('tap='+m.slice(2));}\n"
      "window.addEventListener('keydown',function(e){kmd(e,true);});\n"
      "window.addEventListener('keyup',function(e){kmd(e,false);});";

  engine.server()->stop();
  engine.server()->start(options.webPort, kimia::web::makePageHtml(
      "KIMIA World", {}, keymapJs,
      "everything is menus: tap 1-9 for the options, arrows move, Shift = fine, r resets, b opens the menu, "
      "Space = jump (or hold to charge a shot)"));
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

  const MeshData cubeMesh = kimia::makeCube(1.0);
  const MeshData planeMesh = kimia::makePlane(1.0, 1.0);
  const MeshData sphereMesh = kimia::makeSphere(16, 8);
  const i32 width = 640;
  const i32 height = 480;

  std::signal(SIGINT, onSignal);
  std::map<std::string, kimia::MeshData> loadedMeshes;  // meshFile -> mesh
  kimia::OrbitCamera orbitCamera;  // arrow keys orbit, q/e zoom, c resets
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
    } else {
      orbitCamera.orbit(input.lookX * 0.006, input.lookY * 0.006);
    }

    editor.update(dt);
    for (const WorldEditor::GameEvent event : editor.drainEvents()) engine.server()->playSound(soundFor(event));

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
      scene.objects.push_back({mesh, model, entity.color, entity.roughness});
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
    addAimIndicator(scene, editor, cubeMesh);
    addCurrentCupFlag(scene, editor, cubeMesh);
    if (editor.selectingObject() && editor.selectedEntity() != nullptr) {
      addSelectionMarkers(scene, *editor.selectedEntity(), cubeMesh);
    }

    // Camera: above the ghost while placing/moving, above the ball in play,
    // an overview of the field otherwise; the orbit offset persists.
    Vec3 target = Vec3{0.0, 0.2, 0.0};
    if (editor.placing() || editor.movingObject()) {
      target = Vec3{editor.ghostPosition().x, 0.2, editor.ghostPosition().z};
    } else if (editor.playing()) {
      target = editor.ballPosition();
      if (editor.chaseCameraActive()) {
        // Chase camera: swing behind the ball, looking along the aim. A look
        // drag still peeks around; the camera eases back behind the aim.
        orbitCamera.yaw += angleDelta(orbitCamera.yaw, editor.aimYaw()) * std::min(1.0, kChaseFollowRate * dt);
      }
    } else if (editor.selectingObject() && editor.selectedEntity() != nullptr) {
      target = editor.selectedEntity()->transform.position;
    }
    orbitCamera.center = target;
    const Vec3 eye = orbitCamera.eye();
    scene.cameraPosition = eye;
    scene.view = Mat4::lookAt(eye, orbitCamera.target(), Vec3{0.0, 1.0, 0.0});
    scene.projection = Mat4::perspective(kimia::radians(60.0), static_cast<f64>(width) / static_cast<f64>(height),
                                         0.1, 100.0);
    scene.lightDirection = Vec3{-0.4, -0.8, -0.4};

    Image image;
    if (renderer.ready()) {
      renderer.render(scene, width, height);
      if (!renderer.captureImage(width, height, image)) image = Image{};
    }
    if (image.isEmpty()) kimia::renderSoftware(scene, width, height, colors.clear, image);
    drawHud(image, editor);
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

    engine.server()->publishFrame(std::move(png), editor.statsLine());
    engine.server()->setMenu(menu);
    engine.endFrame();

    const auto elapsed = std::chrono::steady_clock::now() - now;
    const auto left = frameBudget - std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    if (left.count() > 0) std::this_thread::sleep_for(left);
  }

  std::printf("bye | %s\n", editor.statsLine().c_str());
  return 0;
}

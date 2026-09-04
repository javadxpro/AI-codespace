// GolfGame — the reference golf game (spec section 7).
//
//   kimia_golf [--edit] [--course <file>] [--demo] [--web [--port N]]
//              [--lowfx] [--headless-demo]
//
// EDIT <-> AIM -> CHARGE -> ROLL -> SUNK/OUT. Web pad: hold a/d to aim,
// hold SPACE to charge (release to shoot). Builder: 1/2/3 tool, WASD/arrows
// move the ghost (Shift = fine), Q/E wall length, R axis, Enter place,
// U undo, S save, L load, F play/edit.
#include <kimia/Engine.h>
#include <kimia/Golf.h>
#include <kimia/Image.h>
#include <kimia/MathUtils.h>
#include <kimia/Mesh.h>
#include <kimia/Renderer.h>
#include <kimia/WebViewer.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

using kimia::BuilderTool;
using kimia::Engine;
using kimia::EngineOptions;
using kimia::GolfGame;
using kimia::GolfMode;
using kimia::Image;
using kimia::Key;
using kimia::Mat4;
using kimia::MeshData;
using kimia::RenderScene;
using kimia::Renderer;
using kimia::Vec3;
using kimia::clamp;
using kimia::f64;
using kimia::i32;
using kimia::u8;
using kimia::u16;

namespace {

std::atomic<bool> running{true};

void onSignal(int) { running.store(false); }

}  // namespace

int main(int argc, char** argv) {
  bool editFlag = false;
  bool demoFlag = false;
  bool webFlag = false;
  bool lowFx = false;
  bool headlessDemo = false;
  std::string coursePath = "my_course.kimia";
  int port = 8080;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--edit") {
      editFlag = true;
    } else if (arg == "--course" && i + 1 < argc) {
      coursePath = argv[++i];
    } else if (arg == "--demo") {
      demoFlag = true;
    } else if (arg == "--web") {
      webFlag = true;
    } else if (arg == "--port" && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if (arg == "--lowfx") {
      lowFx = true;
    } else if (arg == "--headless-demo") {
      headlessDemo = true;
    }
  }

  GolfGame game;
  {
    std::FILE* probe = std::fopen(coursePath.c_str(), "rb");
    if (probe != nullptr) {
      std::fclose(probe);
      std::string error;
      if (game.loadCourse(coursePath, error)) {
        std::printf("course loaded: %s\n", coursePath.c_str());
      } else {
        std::printf("course load failed (%s); using default\n", error.c_str());
      }
    } else {
      std::printf("no course file; using default course\n");
    }
  }

  EngineOptions options;
  options.headless = webFlag || headlessDemo;
  options.enableWeb = webFlag;
  options.webPort = static_cast<u16>(port);
  options.windowTitle = "KIMIA Golf";
  Engine engine;
  if (!engine.initialize(options)) {
    std::printf("engine init failed\n");
    return 1;
  }

  const std::vector<kimia::web::PadButton> pad = {
      {"aim left", "a", true},   {"aim right", "d", true},  {"SHOOT", "space", true},
      {"wall", "num1", false},   {"tee", "num2", false},    {"hole", "num3", false},
      {"place", "return", false}, {"len -", "q", false},     {"len +", "e", false},
      {"axis", "r", false},      {"undo", "u", false},      {"save", "s", false},
      {"load", "l", false},      {"play/edit", "f", false}, {"quit", "escape", false},
  };
  // Physical-keyboard keymap (spec section 7).
  const char* keymapJs =
      "var km={a:'h:a',d:'h:d',' ':'h:space',Enter:'t:return',q:'t:q',e:'t:e',r:'t:r',"
      "u:'t:u',s:'t:s',l:'t:l',f:'t:f','1':'t:num1','2':'t:num2','3':'t:num3'};\n"
      "function kmd(e,down){var m=km[e.key];if(!m)return;e.preventDefault();"
      "if(m[0]==='h')post('key='+m.slice(2)+'&down='+(down?1:0));else if(down)post('tap='+m.slice(2));}\n"
      "window.addEventListener('keydown',function(e){kmd(e,true);});\n"
      "window.addEventListener('keyup',function(e){kmd(e,false);});";

  if (engine.server() == nullptr && webFlag) {
    std::printf("web server failed to start\n");
    return 1;
  }
  if (engine.server() != nullptr) {
    engine.server()->stop();
    engine.server()->start(options.webPort, kimia::web::makePageHtml(
        "KIMIA Golf", pad, keymapJs,
        "hold a/d to aim, hold SPACE to charge, release to shoot | builder: 1/2/3 tool, "
        "WASD/arrows move ghost, Shift fine, Q/E length, R axis, Enter place, U undo, S save, "
        "L load, F play/edit"));
    std::printf("KIMIA Golf serving on port %d | GL: %s\n", static_cast<i32>(engine.server()->port()),
                engine.glAvailable() ? "yes" : "no (software)");
  } else if (!headlessDemo && engine.window() == nullptr) {
    std::printf("no display backend available; rerun with --web (WebViewer)\n");
    return 1;
  }

  if (editFlag) {
    if (game.mode() != GolfMode::Edit) game.togglePlayEdit();  // start in EDIT
  } else if (game.mode() == GolfMode::Edit) {
    game.togglePlayEdit();  // start in AIM
  }

  Renderer renderer;
  std::string rendererError;
  if (engine.glAvailable() && !renderer.initialize(rendererError)) {
    std::printf("renderer init failed: %s\n", rendererError.c_str());
  }

  const MeshData cubeMesh = kimia::makeCube(1.0);
  const MeshData planeMesh = kimia::makePlane(1.0, 1.0);
  const MeshData sphereMesh = kimia::makeSphere(16, 8);
  const i32 width = lowFx ? 320 : 640;
  const i32 height = lowFx ? 240 : 480;

  std::signal(SIGINT, onSignal);

  if (headlessDemo) {
    game.launchDemoShot();
    std::printf("%s\n", game.statsLine().c_str());
    bool sunkSeen = false;
    GolfMode lastMode = game.mode();
    for (i32 i = 0; i < 3600 && running.load(); ++i) {
      game.update(1.0 / 60.0);
      if (game.mode() == GolfMode::Sunk) sunkSeen = true;
      if (game.mode() != lastMode) {
        std::printf("%s\n", game.statsLine().c_str());
        lastMode = game.mode();
      }
      if (game.mode() == GolfMode::Aim) break;
    }
    std::printf("RESULT %s\n", sunkSeen ? "SUNK" : "OUT");
    return 0;
  }

  if (demoFlag) game.launchDemoShot();

  f64 inspectYaw = 0.0;
  f64 inspectPitch = 0.0;
  const auto frameStart = std::chrono::steady_clock::now();
  auto lastTime = frameStart;
  const std::chrono::microseconds frameBudget(webFlag ? 33333 : 16666);

  while (running.load()) {
    const auto now = std::chrono::steady_clock::now();
    f64 dt = static_cast<f64>(std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count()) / 1000000.0;
    lastTime = now;
    dt = clamp(dt, 0.0, 0.1);

    if (!engine.poll()) break;
    kimia::InputState& input = engine.input();

    if (input.pressed(Key::Escape)) break;
    if (input.pressed(Key::Num1)) game.setTool(BuilderTool::Wall);
    if (input.pressed(Key::Num2)) game.setTool(BuilderTool::Tee);
    if (input.pressed(Key::Num3)) game.setTool(BuilderTool::Hole);
    if (input.pressed(Key::Return)) {
      if (game.place()) std::printf("placed | %s\n", game.statsLine().c_str());
    }
    if (input.pressed(Key::Q)) game.adjustWallLength(-0.5);
    if (input.pressed(Key::E)) game.adjustWallLength(0.5);
    if (input.pressed(Key::R)) game.toggleWallAxis();
    if (input.pressed(Key::U)) {
      if (game.undo()) std::printf("undo | %s\n", game.statsLine().c_str());
    }
    if (input.pressed(Key::S)) {
      std::string error;
      if (game.saveCourse(coursePath, error)) {
        std::printf("saved %s | walls %zu\n", coursePath.c_str(), game.wallCount());
      } else {
        std::printf("save failed: %s\n", error.c_str());
      }
    }
    if (input.pressed(Key::L)) {
      std::string error;
      if (game.loadCourse(coursePath, error)) {
        std::printf("loaded %s | walls %zu\n", coursePath.c_str(), game.wallCount());
      } else {
        std::printf("load failed: %s\n", error.c_str());
      }
    }
    if (input.pressed(Key::F)) {
      game.togglePlayEdit();
      std::printf("mode | %s\n", game.statsLine().c_str());
    }

    const bool editing = game.mode() == GolfMode::Edit;
    const bool fine = input.down(Key::Shift);
    if (editing) {
      f64 dx = 0.0;
      f64 dz = 0.0;
      if (input.down(Key::A) || input.down(Key::Left)) dx -= 1.0;
      if (input.down(Key::D) || input.down(Key::Right)) dx += 1.0;
      if (input.down(Key::W) || input.down(Key::Up)) dz -= 1.0;
      if (input.down(Key::S) || input.down(Key::Down)) dz += 1.0;
      if (dx != 0.0 || dz != 0.0) game.moveGhost(dx, dz, fine);
    } else {
      if (input.down(Key::A)) game.aimLeft(dt);
      if (input.down(Key::D)) game.aimRight(dt);
      if (input.pressed(Key::Space)) game.chargeBegin();
      if (input.released(Key::Space)) game.chargeEnd();
      inspectYaw += input.lookX * 0.006;
      inspectPitch += input.lookY * 0.004;
    }
    inspectYaw = clamp(inspectYaw, -0.9, 0.9);
    inspectPitch = clamp(inspectPitch, -0.5, 0.5);
    inspectYaw *= 0.9;
    inspectPitch *= 0.9;

    game.update(dt);

    // --- Build the frame ---
    RenderScene scene;
    const Vec3 ball = game.ballPosition();
    const Vec3 hole = game.holePosition();
    game.scene().forEach([&](kimia::EntityHandle, const kimia::EntityData& entity) {
      if (entity.name == "Ball") return;  // the physics ball is drawn separately
      const MeshData* mesh = &cubeMesh;
      if (entity.mesh == kimia::MeshKind::plane) mesh = &planeMesh;
      if (entity.mesh == kimia::MeshKind::sphere) mesh = &sphereMesh;
      const Mat4 model = Mat4::translation(entity.transform.position) * Mat4::scaling(entity.transform.scale);
      scene.objects.push_back({mesh, model, entity.color, entity.roughness});
    });
    // The ball (resting in the cup when sunk).
    {
      const Vec3 at = game.ballInHole() ? Vec3{hole.x, 0.13, hole.z} : ball;
      scene.objects.push_back(
          {&sphereMesh, Mat4::translation(at) * Mat4::scaling(Vec3{0.12, 0.12, 0.12}),
           Vec3{0.95, 0.95, 0.92}, 0.3});
    }
    // Aim indicator: a dashed chain of slabs along the shot direction.
    // The chain grows while charging, showing the power.
    if (game.mode() == GolfMode::Aim || game.mode() == GolfMode::Charge) {
      const f64 yaw = game.aimYaw();
      const Vec3 dir{-std::sin(yaw), 0.0, -std::cos(yaw)};
      const Mat4 rotate = Mat4::rotationY(yaw);
      i32 count = 8;
      if (game.mode() == GolfMode::Charge) count = 3 + static_cast<i32>(game.power() * 13.0);
      for (i32 k = 0; k < count; ++k) {
        const Vec3 marker = ball + dir * (0.7 + 0.62 * static_cast<f64>(k));
        scene.objects.push_back(
            {&cubeMesh, Mat4::translation(marker) * rotate * Mat4::scaling(Vec3{0.12, 0.04, 0.5}),
             Vec3{1.0, 0.9, 0.25}, 0.9});
      }
    }
    // Builder ghost.
    if (game.mode() == GolfMode::Edit) {
      const Vec3 ghost = game.ghostPosition();
      Vec3 scale{0.5, 1.0, game.wallLength()};
      Vec3 position{ghost.x, 0.5, ghost.z};
      if (game.tool() == BuilderTool::Tee) {
        scale = Vec3{0.4, 0.02, 0.4};
        position = Vec3{ghost.x, 0.01, ghost.z};
      } else if (game.tool() == BuilderTool::Hole) {
        scale = Vec3{0.56, 0.02, 0.56};
        position = Vec3{ghost.x, 0.01, ghost.z};
      } else if (!game.wallAxisZ()) {
        scale = Vec3{game.wallLength(), 1.0, 0.5};
      }
      scene.objects.push_back(
          {&cubeMesh, Mat4::translation(position) * Mat4::scaling(scale), Vec3{0.95, 0.9, 0.35}, 0.9});
    }

    // Camera.
    Vec3 eye;
    Vec3 target;
    if (game.mode() == GolfMode::Edit) {
      const Vec3 ghost = game.ghostPosition();
      target = Vec3{ghost.x, 0.2, ghost.z};
      eye = Vec3{ghost.x, 7.0, ghost.z + 6.0};
    } else {
      const f64 camYaw = game.aimYaw() + inspectYaw;
      const Vec3 forward{-std::sin(camYaw), 0.0, -std::cos(camYaw)};
      const f64 dist = game.mode() == GolfMode::Roll ? 3.2 : 4.4;
      if (game.ballInHole()) {
        target = Vec3{hole.x, 0.1, hole.z};
        eye = target - forward * dist + Vec3{0.0, 1.6 + inspectPitch, 0.0};
      } else {
        target = ball;
        eye = ball - forward * dist + Vec3{0.0, 1.5 + inspectPitch, 0.0};
      }
    }
    scene.cameraPosition = eye;
    scene.view = Mat4::lookAt(eye, target, Vec3{0.0, 1.0, 0.0});
    scene.projection = Mat4::perspective(kimia::radians(60.0), static_cast<f64>(width) / static_cast<f64>(height),
                                         0.1, 100.0);
    scene.lightDirection = Vec3{-0.4, -0.8, -0.4};

    Image image;
    std::vector<u8> png;
    if (renderer.ready()) {
      renderer.render(scene, width, height);
      if (!renderer.capturePNG(width, height, png)) png.clear();
    }
    if (png.empty()) {
      kimia::renderSoftware(scene, width, height, Vec3{0.05, 0.05, 0.06}, image);
      png = image.encodePNG();
    }
    if (engine.server() != nullptr) {
      engine.server()->publishFrame(std::move(png), game.statsLine());
    } else if (engine.window() != nullptr) {
      engine.window()->present(image);
    }

    engine.endFrame();

    if (webFlag) {
      const auto elapsed = std::chrono::steady_clock::now() - now;
      const auto left = frameBudget - std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
      if (left.count() > 0) std::this_thread::sleep_for(left);
    }
  }

  std::printf("bye | %s\n", game.statsLine().c_str());
  return 0;
}

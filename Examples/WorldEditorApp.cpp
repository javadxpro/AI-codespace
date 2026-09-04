// KIMIA World — the option-driven editor / object builder (spec section 8).
//
//   kimia_world [--port N] [--world <file.kimia>]
//
// Start with an EMPTY ground and build your game with menus only: add a
// player, a ball, blocks, walls, goals — each object asks a few plain
// questions («دقیق باشه یا فانتزی؟») — then manage them (move/delete/color)
// and press PLAY. Worlds save as SceneIO-v1-compatible text.
#include <kimia/Engine.h>
#include <kimia/Image.h>
#include <kimia/MathUtils.h>
#include <kimia/Mesh.h>
#include <kimia/Renderer.h>
#include <kimia/WebViewer.h>
#include <kimia/AssetPipeline.h>
#include <kimia/World.h>

#include <atomic>
#include <chrono>
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
    case ObjectKind::Ball:
      scene.objects.push_back({&sphere, Mat4::translation(Vec3{ghost.x, 0.35, ghost.z}) *
                                            Mat4::scaling(Vec3{0.24, 0.24, 0.24}),
                               kGhostColor, 0.9});
      break;
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
    default:
      break;
  }
}

}  // namespace

int main(int argc, char** argv) {
  int port = 8080;
  std::string worldPath = "my_world.kimia";
  std::string assetsDir = "assets";
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--port" && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if (arg == "--world" && i + 1 < argc) {
      worldPath = argv[++i];
    } else if (arg == "--assets" && i + 1 < argc) {
      assetsDir = argv[++i];
    }
  }

  WorldEditor editor;
  editor.setWorldPath(worldPath);
  editor.setImportDirectory(assetsDir);

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
      "'7':'t:num7','8':'t:num8',"
      "'r':'t:r','b':'t:b','ArrowUp':'h:up','ArrowDown':'h:down','ArrowLeft':'h:left',"
      "'ArrowRight':'h:right','Shift':'h:shift'};\n"
      "function kmd(e,down){var m=km[e.key];if(!m)return;e.preventDefault();"
      "if(m[0]==='h')post('key='+m.slice(2)+'&down='+(down?1:0));else if(down)post('tap='+m.slice(2));}\n"
      "window.addEventListener('keydown',function(e){kmd(e,true);});\n"
      "window.addEventListener('keyup',function(e){kmd(e,false);});";

  engine.server()->stop();
  engine.server()->start(options.webPort, kimia::web::makePageHtml(
      "KIMIA World", {}, keymapJs,
      "everything is menus: tap 1-8 for the options, arrows move, Shift = fine, r resets, b opens the menu"));
  std::printf("KIMIA World serving on port %d | GL: %s\n", static_cast<i32>(engine.server()->port()),
              engine.glAvailable() ? "yes" : "no (software)");

  Renderer renderer;
  std::string rendererError;
  if (engine.glAvailable() && !renderer.initialize(rendererError)) {
    std::printf("renderer init failed: %s\n", rendererError.c_str());
  }

  const MeshData cubeMesh = kimia::makeCube(1.0);
  const MeshData planeMesh = kimia::makePlane(1.0, 1.0);
  const MeshData sphereMesh = kimia::makeSphere(16, 8);
  const i32 width = 640;
  const i32 height = 480;

  std::signal(SIGINT, onSignal);
  std::map<std::string, kimia::MeshData> loadedMeshes;  // meshFile -> mesh
  f64 cameraYaw = 0.0;
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
    if (input.pressed(Key::R)) editor.resetBall();
    if (input.pressed(Key::B)) editor.backToMenu();

    f64 moveX = 0.0;
    f64 moveZ = 0.0;
    if (input.down(Key::Left)) moveX -= 1.0;
    if (input.down(Key::Right)) moveX += 1.0;
    if (input.down(Key::Up)) moveZ -= 1.0;
    if (input.down(Key::Down)) moveZ += 1.0;
    editor.setMoveInput(moveX, moveZ);
    editor.setFineMove(input.down(Key::Shift));
    cameraYaw += input.lookX * 0.006;
    cameraYaw = clamp(cameraYaw, -1.2, 1.2);

    editor.update(dt);

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
      // Crates follow the physics bodies while playing.
      const Vec3 position =
          kind == ObjectKind::Crate ? editor.cratePosition(entity.name) : entity.transform.position;
      const Mat4 model = Mat4::translation(position) * Mat4::scaling(entity.transform.scale);
      scene.objects.push_back({mesh, model, entity.color, entity.roughness});
      if (kind == ObjectKind::Player) {
        // A little head so the player reads as a character.
        scene.objects.push_back(
            {&cubeMesh, Mat4::translation(entity.transform.position + Vec3{0.0, 0.65, 0.0}) *
                            Mat4::scaling(Vec3{0.3, 0.3, 0.3}),
             entity.color, entity.roughness});
      }
    });
    // The ball follows the physics body.
    const f64 ballRadius = editor.world().ball.radius;
    scene.objects.push_back(
        {&sphereMesh, Mat4::translation(editor.ballPosition()) * Mat4::scaling(Vec3{ballRadius, ballRadius, ballRadius}),
         editor.world().ball.color, 0.3});
    // Ghost preview while placing, selection markers while managing.
    if (editor.placing()) addGhostShape(scene, editor, cubeMesh, sphereMesh);
    if (editor.selectingObject() && editor.selectedEntity() != nullptr) {
      addSelectionMarkers(scene, *editor.selectedEntity(), cubeMesh);
    }

    // Camera: above the ghost while placing/moving, above the ball in play,
    // an overview of the field otherwise.
    Vec3 target = Vec3{0.0, 0.2, 0.0};
    if (editor.placing() || editor.movingObject()) {
      target = Vec3{editor.ghostPosition().x, 0.2, editor.ghostPosition().z};
    } else if (editor.playing()) {
      target = editor.ballPosition();
    } else if (editor.selectingObject() && editor.selectedEntity() != nullptr) {
      target = editor.selectedEntity()->transform.position;
    }
    const Vec3 eye = target + Vec3{std::sin(cameraYaw) * 6.0, 3.6, std::cos(cameraYaw) * 6.0};
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
      kimia::renderSoftware(scene, width, height, colors.clear, image);
      png = image.encodePNG();
    }

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

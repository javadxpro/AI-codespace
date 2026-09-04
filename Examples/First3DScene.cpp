// First3DScene — lit cube+plane+sphere with an orbiting camera.
//
//   kimia_first3d [--frames N] [--capture path.png] [--web] [--port N] [--window]
//
// Renders with GL when available, otherwise with the software rasterizer.
// With --web the frames are served by the WebViewer on the given port.
#include <kimia/Engine.h>
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
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using kimia::Engine;
using kimia::EngineOptions;
using kimia::Image;
using kimia::Mat4;
using kimia::MeshData;
using kimia::RenderScene;
using kimia::Renderer;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::u8;

namespace {

std::atomic<bool> running{true};

void onSignal(int) { running.store(false); }

bool writeFile(const std::string& path, const std::vector<u8>& data) {
  std::FILE* file = std::fopen(path.c_str(), "wb");
  if (file == nullptr) return false;
  std::fwrite(data.data(), 1, data.size(), file);
  std::fclose(file);
  return true;
}

kimia::RenderScene buildScene(f64 time) {
  const f64 yaw = time * 0.5;
  const f64 radius = 4.5;
  kimia::RenderScene scene;
  scene.cameraPosition = kimia::Vec3{std::cos(yaw) * radius, 1.8, std::sin(yaw) * radius};
  scene.view = kimia::Mat4::lookAt(scene.cameraPosition, kimia::Vec3{0.0, 0.0, 0.0}, kimia::Vec3{0.0, 1.0, 0.0});
  scene.projection = kimia::Mat4::perspective(kimia::radians(60.0), 4.0 / 3.0, 0.1, 100.0);
  scene.lightDirection = kimia::Vec3{-0.4, -0.8, -0.4};
  scene.ambient = 0.25;
  return scene;
}

}  // namespace

int main(int argc, char** argv) {
  i32 frameCount = 120;
  std::string capturePath;
  bool enableWeb = false;
  int port = 8080;
  bool windowMode = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--frames" && i + 1 < argc) {
      frameCount = std::atoi(argv[++i]);
    } else if (arg == "--capture" && i + 1 < argc) {
      capturePath = argv[++i];
    } else if (arg == "--web") {
      enableWeb = true;
    } else if (arg == "--port" && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if (arg == "--window") {
      windowMode = true;
    }
  }

  kimia::EngineOptions options;
  options.headless = !windowMode;
  options.enableWeb = enableWeb;
  options.webPort = static_cast<kimia::u16>(port);
  kimia::Engine engine;
  engine.initialize(options);

  const kimia::MeshData plane = kimia::makePlane(6.0, 6.0);
  const kimia::MeshData cube = kimia::makeCube(1.0);
  const kimia::MeshData sphere = kimia::makeSphere(16, 8);

  kimia::Renderer renderer;
  std::string error;
  if (engine.glAvailable() && !renderer.initialize(error)) {
    std::printf("renderer init failed: %s\n", error.c_str());
  }

  std::printf("KIMIA First3DScene | GL: %s | window: %s | web: %s\n", engine.glAvailable() ? "yes" : "no",
              engine.window() != nullptr ? "yes" : "no", engine.server() != nullptr ? "yes" : "no");

  std::signal(SIGINT, onSignal);
  const i32 width = 640;
  const i32 height = 480;
  i32 frame = 0;
  std::vector<kimia::web::PadButton> pad = {
      {"left", "a", true}, {"right", "d", true}, {"reset", "r", false},
  };
  std::string page = kimia::web::makePageHtml("KIMIA First3DScene", pad, "", "orbit with the pad; a/d rotate, r resets");
  if (engine.server() != nullptr) {
    // Re-serve the configured page (server started with a default page).
    engine.server()->stop();
    engine.server()->start(options.webPort, page);
  }

  while (running.load() && (enableWeb || frame < frameCount)) {
    if (!engine.poll()) break;

    const f64 time = static_cast<f64>(frame) * 0.03;
    kimia::RenderScene scene = buildScene(time);
    scene.objects.push_back({&plane, kimia::Mat4::translation(kimia::Vec3{0.0, -0.5, 0.0}),
                             kimia::Vec3{0.22, 0.45, 0.24}, 0.95});
    scene.objects.push_back({&cube,
                             kimia::Mat4::translation(kimia::Vec3{0.0, 0.0, 0.0}) * kimia::Mat4::rotationY(time),
                             kimia::Vec3{0.85, 0.2, 0.2}, 0.4});
    scene.objects.push_back({&sphere,
                             kimia::Mat4::translation(kimia::Vec3{1.4, -0.1, 0.8}) * kimia::Mat4::scaling(kimia::Vec3{0.6, 0.6, 0.6}),
                             kimia::Vec3{0.92, 0.92, 0.88}, 0.25});

    kimia::Image image;
    std::vector<u8> png;
    if (renderer.ready()) {
      renderer.render(scene, width, height);
      if (!renderer.capturePNG(width, height, png)) png.clear();
    }
    if (png.empty()) {
      kimia::renderSoftware(scene, width, height, kimia::Vec3{0.05, 0.05, 0.06}, image);
      png = image.encodePNG();
    }
    if (engine.window() != nullptr && !image.isEmpty()) engine.window()->present(image);
    if (engine.server() != nullptr) {
      engine.server()->publishFrame(std::move(png),
                                    "KIMIA First3DScene | frame " + std::to_string(frame) + " | GL " +
                                        (renderer.ready() ? "on" : "off") + " | software " +
                                        (renderer.ready() ? "off" : "on"));
    }
    if (!capturePath.empty() && frame == frameCount - 1) {
      if (writeFile(capturePath, png)) std::printf("captured %s\n", capturePath.c_str());
    }
    ++frame;
    if (enableWeb) std::this_thread::sleep_for(std::chrono::milliseconds(33));
    engine.endFrame();
  }
  std::printf("done: %d frames\n", frame);
  engine.shutdown();
  return 0;
}

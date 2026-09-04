// RemoteView — renders the demo scene headlessly and serves it through the
// WebViewer. The touch pad orbits the camera live (the web input pipeline).
//
//   kimia_remote [--port N]
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
#include <string>
#include <thread>
#include <vector>

using kimia::Engine;
using kimia::EngineOptions;
using kimia::Image;
using kimia::Key;
using kimia::Mat4;
using kimia::MeshData;
using kimia::RenderScene;
using kimia::Renderer;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::u8;
using kimia::u64;

namespace {

std::atomic<bool> running{true};

void onSignal(int) { running.store(false); }

}  // namespace

int main(int argc, char** argv) {
  int port = 8080;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
  }

  kimia::EngineOptions options;
  options.headless = true;
  options.enableWeb = true;
  options.webPort = static_cast<kimia::u16>(port);
  options.windowTitle = "KIMIA RemoteView";
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

  if (engine.server() == nullptr) {
    std::printf("web server failed to start\n");
    return 1;
  }
  const std::vector<kimia::web::PadButton> pad = {
      {"left", "a", true},      {"right", "d", true},       {"pitch up", "w", true},
      {"pitch down", "s", true}, {"zoom in", "z", true},     {"zoom out", "x", true},
      {"reset", "r", false},
  };
  engine.server()->stop();
  engine.server()->start(options.webPort, kimia::web::makePageHtml(
      "KIMIA RemoteView", pad, "", "drag the pad to orbit; a/d/w/s rotate; z/x zoom; r resets"));

  std::printf("KIMIA RemoteView serving on port %d | GL: %s\n", engine.server()->port(),
              engine.glAvailable() ? "yes" : "no (software)");

  std::signal(SIGINT, onSignal);
  const i32 width = 640;
  const i32 height = 480;
  f64 yaw = 0.0;
  f64 pitch = 0.45;
  f64 distance = 4.5;
  u64 frame = 0;
  while (running.load()) {
    if (!engine.poll()) break;
    kimia::InputState& input = engine.input();

    yaw += static_cast<f64>(input.lookX) * 0.005;
    pitch += static_cast<f64>(input.lookY) * 0.005;
    distance *= (1.0 - input.zoom * 0.05);
    if (input.down(kimia::Key::A)) yaw += 0.03;
    if (input.down(kimia::Key::D)) yaw -= 0.03;
    if (input.down(kimia::Key::W)) pitch = std::min(pitch + 0.02, 1.45);
    if (input.down(kimia::Key::S)) pitch = std::max(pitch - 0.02, -1.45);
    if (input.down(kimia::Key::Z)) distance *= 0.985;
    if (input.down(kimia::Key::X)) distance *= 1.015;
    if (input.pressed(kimia::Key::R)) {
      yaw = 0.0;
      pitch = 0.45;
      distance = 4.5;
    }
    distance = kimia::clamp(distance, 2.0, 12.0);
    pitch = kimia::clamp(pitch, -1.45, 1.45);

    const kimia::Vec3 cameraPosition{std::cos(yaw) * std::cos(pitch) * distance, std::sin(pitch) * distance + 0.6,
                                     std::sin(yaw) * std::cos(pitch) * distance};
    kimia::RenderScene scene;
    scene.cameraPosition = cameraPosition;
    scene.view = kimia::Mat4::lookAt(cameraPosition, kimia::Vec3{0.0, 0.0, 0.0}, kimia::Vec3{0.0, 1.0, 0.0});
    scene.projection = kimia::Mat4::perspective(kimia::radians(60.0), 4.0 / 3.0, 0.1, 100.0);
    scene.lightDirection = kimia::Vec3{-0.4, -0.8, -0.4};
    scene.objects.push_back({&plane, kimia::Mat4::translation(kimia::Vec3{0.0, -0.5, 0.0}),
                             kimia::Vec3{0.22, 0.45, 0.24}, 0.95});
    scene.objects.push_back({&cube, kimia::Mat4::rotationY(static_cast<f64>(frame) * 0.01),
                             kimia::Vec3{0.85, 0.2, 0.2}, 0.4});
    scene.objects.push_back({&sphere, kimia::Mat4::translation(kimia::Vec3{1.4, -0.1, 0.8}) *
                                         kimia::Mat4::scaling(kimia::Vec3{0.6, 0.6, 0.6}),
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
    engine.server()->publishFrame(std::move(png),
                                  "KIMIA RemoteView | frame " + std::to_string(frame) + " | camera yaw " +
                                      std::to_string(yaw) + " pitch " + std::to_string(pitch) + " dist " +
                                      std::to_string(distance) + " | " + (renderer.ready() ? "GL" : "software"));
    ++frame;
    engine.endFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(33));
  }
  std::printf("stopped after %llu frames\n", static_cast<unsigned long long>(frame));
  engine.shutdown();
  return 0;
}

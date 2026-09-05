// HelloWindow — trivial SDL/GL smoke app. Shows (or blits) one rendered
// frame of a cube, reports what backends are available, and exits.
#include <kimia/Engine.h>
#include <kimia/Image.h>
#include <kimia/MathUtils.h>
#include <kimia/Mesh.h>
#include <kimia/Renderer.h>

#include <cstdio>
#include <string>
#include <vector>

int main() {
  kimia::EngineOptions options;
  options.headless = false;
  options.windowTitle = "KIMIA HelloWindow";
  options.windowWidth = 640;
  options.windowHeight = 480;

  kimia::Engine engine;
  engine.initialize(options);

  std::printf("KIMIA HelloWindow | GL: %s | window: %s\n", engine.glAvailable() ? "yes" : "no",
              engine.window() != nullptr ? "yes" : "no");

  // One software-rendered frame (works everywhere, GL or not).
  const kimia::MeshData cube = kimia::makeCube(1.0);
  kimia::RenderScene scene;
  scene.view = kimia::Mat4::lookAt(kimia::Vec3{0.0, 1.2, 4.0}, kimia::Vec3{0.0, 0.0, 0.0}, kimia::Vec3{0.0, 1.0, 0.0});
  scene.projection = kimia::Mat4::perspective(kimia::radians(60.0), 4.0 / 3.0, 0.1, 100.0);
  scene.cameraPosition = kimia::Vec3{0.0, 1.2, 4.0};
  scene.objects.push_back({&cube, kimia::Mat4{}, kimia::Vec3{0.9, 0.25, 0.2}, 0.4});

  kimia::Image image;
  if (!kimia::renderSoftware(scene, 640, 480, kimia::Vec3{0.05, 0.05, 0.06}, image)) {
    std::printf("render failed\n");
    return 1;
  }
  if (engine.window() != nullptr) {
    engine.window()->present(image);
    engine.poll();
  }
  std::printf("hello window OK (640x480 frame rendered)\n");
  engine.shutdown();
  return 0;
}

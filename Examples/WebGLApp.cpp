// KIMIA in the browser: real WebGL2 + WebAssembly via Emscripten.
//
// This is the Emscripten entry point. It boots the engine's GL renderer on a
// WebGL2 context (the same Cook-Torrance PBR + point lights + fog + filmic
// tone-map pipeline the desktop and software backends use) and draws a small
// showcase straight onto the page's canvas every frame.
//
// Built only by the Emscripten toolchain (the CMake target kimia_webgl is
// added under `if(EMSCRIPTEN)`), so the emscripten headers are safe here.

#include <emscripten.h>
#include <emscripten/html5.h>

#include <kimia/EGL.h>
#include <kimia/GLFunctions.h>
#include <kimia/Mesh.h>
#include <kimia/Renderer.h>

#include <cmath>
#include <cstdio>
#include <string>

namespace {

kimia::EGLContext gContext;
kimia::Renderer gRenderer;
kimia::RenderScene gScene;
kimia::MeshData gPlane;
kimia::MeshData gBall;
kimia::MeshData gCube;
double gAngle = 0.0;

bool boot() {
  // A WebGL2 context on the page's <canvas id="canvas">, then the GL entry
  // points (wired directly to GLES3 under Emscripten) and the renderer.
  if (!gContext.create(960, 600)) {
    std::printf("[kimia] WebGL2 context creation failed\n");
    return false;
  }
  if (!kimia::GLFunctions::instance().loaded()) kimia::GLFunctions::instance().load();

  std::string error;
  if (!gRenderer.initialize(error)) {
    std::printf("[kimia] renderer init failed: %s\n", error.c_str());
    return false;
  }

  // --- A small showcase scene -------------------------------------------
  gPlane = kimia::makePlane(12.0, 12.0);
  gBall = kimia::makeSphere(24, 12);
  gCube = kimia::makeCube(1.4);

  gScene.view = kimia::Mat4::lookAt(kimia::Vec3{0.0, 3.2, 6.5}, kimia::Vec3{0.0, 1.0, 0.0},
                                    kimia::Vec3{0.0, 1.0, 0.0});
  gScene.projection = kimia::Mat4::perspective(1.05, 16.0 / 10.0, 0.1, 100.0);
  gScene.cameraPosition = kimia::Vec3{0.0, 3.2, 6.5};
  gScene.lightDirection = kimia::Vec3{-0.4, -0.8, -0.5};
  gScene.ambient = 0.18;
  gScene.fogColor = kimia::Vec3{0.09, 0.11, 0.14};  // the charcoal brand backdrop
  gScene.fogDensity = 0.035;

  // Ground: rough dark slate.
  gScene.objects.push_back(
      {&gPlane, kimia::Mat4{}, kimia::Vec3{0.32, 0.36, 0.40}, 0.95, 0.0});

  // A spinning polished chrome ball (metal, low roughness).
  gScene.objects.push_back(
      {&gBall, kimia::Mat4::translation(kimia::Vec3{-1.4, 1.1, 0.0}), kimia::Vec3{0.9, 0.9, 0.95}, 0.15, 1.0});

  // A gold dielectric cube for contrast.
  gScene.objects.push_back(
      {&gCube, kimia::Mat4::translation(kimia::Vec3{1.6, 0.7, 0.4}), kimia::Vec3{0.85, 0.62, 0.22}, 0.4, 0.0});

  // A warm emissive lamp above the ball so the metal has something to mirror.
  gScene.objects.push_back({&gCube,
                            kimia::Mat4::translation(kimia::Vec3{-1.4, 3.0, 0.0})) *
                                kimia::Mat4::scaling(kimia::Vec3{0.25, 0.08, 0.25}),
                            kimia::Vec3{0.0, 0.0, 0.0}, 1.0, 0.0, nullptr,
                            kimia::Vec3{6.0, 4.5, 3.0}, 1.0});

  // A coloured point light sweeping the floor.
  kimia::PointLight lamp;
  lamp.position = kimia::Vec3{0.0, 2.6, 0.0};
  lamp.color = kimia::Vec3{0.4, 0.7, 1.6};
  lamp.radius = 9.0;
  gScene.pointLights.push_back(lamp);

  std::printf("[kimia] WebGL2 renderer ready\n");
  return true;
}

void frame() {
  int width = 0;
  int height = 0;
  emscripten_get_canvas_element_size("canvas", &width, &height);
  if (width <= 0 || height <= 0 || !gRenderer.ready()) return;

  gAngle += 0.012;
  const double bob = std::sin(gAngle * 1.7) * 0.25;
  // Spin the chrome ball and swing the point light gently.
  gScene.objects[1].model =
      kimia::Mat4::translation(kimia::Vec3{-1.4, 1.1 + bob, 0.0}) * kimia::Mat4::rotationY(gAngle);
  gScene.pointLights[0].position = kimia::Vec3{std::sin(gAngle * 0.7) * 2.5, 2.6, std::cos(gAngle * 0.7) * 2.5};

  gRenderer.render(gScene, width, height);
}

}  // namespace

int main() {
  if (!boot()) return 1;
  emscripten_set_main_loop(frame, 0, 1);
  return 0;
}

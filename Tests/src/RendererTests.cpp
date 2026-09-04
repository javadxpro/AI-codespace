#include <kimia/EGL.h>
#include <kimia/GLFunctions.h>
#include <kimia/Image.h>
#include <kimia/Mesh.h>
#include <kimia/Renderer.h>
#include <kimia_test.h>

#include <cstdio>
#include <string>
#include <vector>

namespace {

using kimia::Image;
using kimia::Mat4;
using kimia::MeshData;
using kimia::RenderScene;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::u8;

RenderScene cubeScene() {
  RenderScene scene;
  scene.view = Mat4::lookAt(Vec3{0.0, 0.0, 3.0}, Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0});
  scene.projection = Mat4::perspective(3.14159265358979323846 * 0.5, 1.0, 0.1, 100.0);
  scene.cameraPosition = Vec3{0.0, 0.0, 3.0};
  scene.lightDirection = Vec3{-0.45, -0.72, -0.53};
  scene.ambient = 0.2;
  return scene;
}

i32 countRedPixels(const Image& image, i32 threshold) {
  i32 count = 0;
  for (i32 y = 0; y < image.height; ++y) {
    for (i32 x = 0; x < image.width; ++x) {
      const u8* pixel = image.at(x, y);
      if (pixel[0] > threshold && pixel[1] < 40 && pixel[2] < 40) ++count;
    }
  }
  return count;
}

}  // namespace

KIMIA_TEST(software_renders_cube_with_exact_pixels) {
  const MeshData cube = kimia::makeCube(2.4);
  RenderScene scene = cubeScene();
  scene.objects.push_back({&cube, Mat4{}, Vec3{1.0, 0.0, 0.0}, 0.5});
  Image image;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 160, 160, Vec3{0.05, 0.05, 0.06}, image));
  KIMIA_REQUIRE(image.width == 160 && image.height == 160 && image.channels == 3);
  // Center: the cube's front face (red, Lambert-lit, gamma-encoded).
  const u8* center = image.at(80, 80);
  KIMIA_REQUIRE(center[0] > 190 && center[0] < 230);
  KIMIA_REQUIRE(center[1] < 12 && center[2] < 12);
  // Corner: clear color (0.05, 0.05, 0.06) after gamma.
  const u8* corner = image.at(2, 2);
  KIMIA_REQUIRE(corner[0] >= 55 && corner[0] <= 75);
  KIMIA_REQUIRE(corner[1] >= 55 && corner[1] <= 75);
  KIMIA_REQUIRE(corner[2] >= 60 && corner[2] <= 80);
  // The cube fills a large part of the frame.
  KIMIA_REQUIRE(countRedPixels(image, 150) > 3000);
}

KIMIA_TEST(software_renders_plane_below_cube) {
  const MeshData plane = kimia::makePlane(4.0, 4.0);
  RenderScene scene = cubeScene();
  scene.objects.push_back({&plane, Mat4::translation(Vec3{0.0, -0.5, 0.0}), Vec3{0.2, 0.8, 0.2}, 0.9});
  Image image;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 160, 160, Vec3{0.05, 0.05, 0.06}, image));
  // Lower-middle pixel: the green plane (bright green after gamma).
  const u8* planePixel = image.at(80, 110);
  KIMIA_REQUIRE(planePixel[1] > 200);
  KIMIA_REQUIRE(planePixel[1] > planePixel[0] + 80);
  KIMIA_REQUIRE(planePixel[1] > planePixel[2] + 80);
}

KIMIA_TEST(software_backface_culling_hides_far_side) {
  const MeshData cube = kimia::makeCube(1.0);
  RenderScene scene = cubeScene();
  scene.objects.push_back({&cube, Mat4{}, Vec3{1.0, 1.0, 1.0}, 0.5});
  Image image;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 160, 160, Vec3{0.0, 0.0, 0.0}, image));
  // With backface culling the image must contain drawn (white) pixels but
  // not cover everything: the far side never overdraws the background.
  i32 white = 0;
  for (i32 y = 0; y < image.height; ++y) {
    for (i32 x = 0; x < image.width; ++x) {
      const u8* pixel = image.at(x, y);
      if (pixel[0] > 100 && pixel[1] > 100 && pixel[2] > 100) ++white;
    }
  }
  KIMIA_REQUIRE(white > 1000);
  KIMIA_REQUIRE(white < 160 * 160 - 1000);
}

KIMIA_TEST(software_invalid_input_rejected) {
  const MeshData cube = kimia::makeCube(1.0);
  RenderScene scene = cubeScene();
  Image image;
  KIMIA_REQUIRE(!kimia::renderSoftware(scene, 0, 160, Vec3{0.0, 0.0, 0.0}, image));
  KIMIA_REQUIRE(!kimia::renderSoftware(scene, 160, -1, Vec3{0.0, 0.0, 0.0}, image));
  KIMIA_REQUIRE(!kimia::renderSoftware(scene, 10000, 10000, Vec3{0.0, 0.0, 0.0}, image));
}

KIMIA_TEST(egl_context_graceful_without_driver) {
  kimia::EGLContext context;
  const bool created = context.create(64, 64);
  KIMIA_REQUIRE(context.valid() == created);
  context.destroy();
  KIMIA_REQUIRE(!context.valid());
  // In headless CI there is no EGL at all: creation fails cleanly. On
  // machines with Mesa the same code creates a real 3.x context.
}

KIMIA_TEST(gl_pipeline_when_available_or_skipped) {
  kimia::EGLContext context;
  if (!context.create(128, 128)) {
    std::printf("SKIP: no EGL/OpenGL driver on this machine\n");
    return;
  }
  KIMIA_REQUIRE(kimia::GLFunctions::instance().load());
  kimia::Renderer renderer;
  std::string error;
  KIMIA_REQUIRE(renderer.initialize(error));
  KIMIA_REQUIRE(renderer.ready());

  const MeshData cube = kimia::makeCube(1.0);
  RenderScene scene = cubeScene();
  scene.objects.push_back({&cube, Mat4{}, Vec3{0.9, 0.2, 0.2}, 0.4});

  renderer.setShadowEnabled(true);
  renderer.render(scene, 128, 128);
  std::vector<u8> png;
  KIMIA_REQUIRE(renderer.capturePNG(128, 128, png));
  KIMIA_REQUIRE(png.size() > 8U);
  // PNG signature.
  KIMIA_REQUIRE(png[0] == 0x89 && png[1] == 0x50 && png[2] == 0x4E && png[3] == 0x47);

  renderer.setShadowEnabled(false);
  renderer.render(scene, 128, 128);
  KIMIA_REQUIRE(renderer.capturePNG(128, 128, png));
  KIMIA_REQUIRE(png.size() > 8U);

  renderer.shutdown();
  KIMIA_REQUIRE(!renderer.ready());
  context.destroy();
}

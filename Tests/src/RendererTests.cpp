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

KIMIA_TEST(software_clips_floor_at_the_near_plane) {
  // Golf-style view: the camera stands INSIDE the floor's extent, so the
  // floor crosses the near plane. It must be clipped, not dropped.
  const MeshData plane = kimia::makePlane(30.0, 30.0);
  RenderScene scene;
  scene.view = Mat4::lookAt(Vec3{0.0, 1.5, 11.4}, Vec3{0.0, 0.0, 7.0}, Vec3{0.0, 1.0, 0.0});
  scene.projection = Mat4::perspective(3.14159265358979323846 * 0.5, 4.0 / 3.0, 0.1, 100.0);
  scene.cameraPosition = Vec3{0.0, 1.5, 11.4};
  scene.lightDirection = Vec3{-0.4, -0.8, -0.4};
  scene.ambient = 0.2;
  scene.objects.push_back({&plane, Mat4{}, Vec3{0.22, 0.45, 0.24}, 0.95});
  // A ball on the floor in front of the camera: its depth must survive the
  // huge clipped floor triangle (regression for z/w depth interpolation).
  const MeshData sphere = kimia::makeSphere(16, 8);
  scene.objects.push_back(
      {&sphere, Mat4::translation(Vec3{0.0, 0.14, 7.0}) * Mat4::scaling(Vec3{0.12, 0.12, 0.12}),
       Vec3{0.95, 0.95, 0.92}, 0.3});
  Image image;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 320, 240, Vec3{0.05, 0.05, 0.06}, image));
  // The bottom of the frame is the floor right in front of the camera.
  const u8* bottom = image.at(160, 239);
  KIMIA_REQUIRE(bottom[1] > bottom[0] + 15);
  KIMIA_REQUIRE(bottom[1] > bottom[2] + 15);
  const u8* corner = image.at(10, 239);
  KIMIA_REQUIRE(corner[1] > corner[0] + 15);
  KIMIA_REQUIRE(corner[1] > corner[2] + 15);
  i32 white = 0;
  for (i32 y = 0; y < image.height; ++y) {
    for (i32 x = 0; x < image.width; ++x) {
      const u8* pixel = image.at(x, y);
      if (pixel[0] > 200 && pixel[1] > 200 && pixel[2] > 200) ++white;
    }
  }
  KIMIA_REQUIRE(white > 10);
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

// --- Stage 34: textures in the software rasteriser ---

namespace {

// An 8x8 checkerboard: unmistakable when it is sampled, and unmistakable
// when it is not.
kimia::Image checkerTexture() {
  kimia::Image texture;
  texture.width = 8;
  texture.height = 8;
  texture.channels = 3;
  texture.pixels.assign(8U * 8U * 3U, 0U);
  for (kimia::i32 y = 0; y < 8; ++y) {
    for (kimia::i32 x = 0; x < 8; ++x) {
      const kimia::u8 value = ((x + y) % 2 == 0) ? 20U : 240U;
      const kimia::usize index = (static_cast<kimia::usize>(y) * 8U + static_cast<kimia::usize>(x)) * 3U;
      texture.pixels[index] = value;
      texture.pixels[index + 1U] = value;
      texture.pixels[index + 2U] = value;
    }
  }
  return texture;
}

// A flat-on view of a quad, lit almost entirely by ambient so the shading
// does not muddy what the texture is doing.
kimia::RenderScene quadScene(const kimia::MeshData& quad, const kimia::Image* texture) {
  kimia::RenderScene scene;
  scene.view = kimia::Mat4::lookAt(Vec3{0.0, 3.0, 4.0}, Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0});
  scene.projection = kimia::Mat4::perspective(1.0, 1.0, 0.1, 100.0);
  scene.cameraPosition = Vec3{0.0, 3.0, 4.0};
  scene.lightDirection = Vec3{0.0, -1.0, -0.2};
  scene.ambient = 0.9;
  scene.objects.push_back({&quad, kimia::Mat4{}, Vec3{1.0, 1.0, 1.0}, 0.5, texture});
  return scene;
}

// How many distinct non-black grey levels the image contains.
kimia::usize distinctGreys(const kimia::Image& image) {
  std::vector<kimia::u8> seen;
  for (kimia::usize i = 0; i + 2U < image.pixels.size(); i += 3U) {
    const kimia::u8 value = image.pixels[i];
    if (value == 0U) continue;
    bool known = false;
    for (const kimia::u8 other : seen) {
      if (other == value) known = true;
    }
    if (!known) seen.push_back(value);
  }
  return seen.size();
}

}  // namespace

KIMIA_TEST(renderer_draws_a_texture_instead_of_a_flat_colour) {
  // The importer has always pulled a diffuse map's path out of a .mtl or
  // an FBX, but nothing ever loaded or drew it: every model rendered as a
  // flat colour however carefully it was textured.
  const kimia::MeshData quad = kimia::makePlane(4.0, 4.0);
  const kimia::Image texture = checkerTexture();

  kimia::Image plain;
  KIMIA_REQUIRE(kimia::renderSoftware(quadScene(quad, nullptr), 160, 160, Vec3{0.0, 0.0, 0.0}, plain));
  kimia::Image textured;
  KIMIA_REQUIRE(kimia::renderSoftware(quadScene(quad, &texture), 160, 160, Vec3{0.0, 0.0, 0.0}, textured));

  // Untextured, the whole quad is one shade. Textured, it is two.
  KIMIA_REQUIRE(distinctGreys(plain) == 1U);
  KIMIA_REQUIRE(distinctGreys(textured) == 2U);
  // And the two images really are different pictures.
  kimia::usize differing = 0U;
  for (kimia::usize i = 0; i < plain.pixels.size(); ++i) {
    if (plain.pixels[i] != textured.pixels[i]) ++differing;
  }
  KIMIA_REQUIRE(differing > 1000U);
}

KIMIA_TEST(renderer_texture_is_perspective_correct) {
  // Interpolating u directly instead of u/w is the classic texturing bug:
  // the picture looks plausible head-on and warps as a surface recedes.
  //
  // The signature is band SPACING, not band count. On a strip running away
  // from the camera with a striped texture mapped along it, correct maths
  // crowds the far stripes together so fewer of them are distinguishable;
  // affine interpolation spreads all of them out evenly.
  //
  // (Two earlier attempts at this test — counting bands per row, and
  // finding a seam — both passed with the correction REMOVED, so they
  // proved nothing. This one was checked by breaking the code on purpose.)
  kimia::Image stripes;
  stripes.width = 16;
  stripes.height = 1;
  stripes.channels = 3;
  stripes.pixels.assign(16U * 3U, 0U);
  for (kimia::i32 i = 0; i < 16; ++i) {
    const kimia::u8 value = (i % 2 == 0) ? 20U : 240U;
    stripes.pixels[static_cast<kimia::usize>(i) * 3U] = value;
    stripes.pixels[static_cast<kimia::usize>(i) * 3U + 1U] = value;
    stripes.pixels[static_cast<kimia::usize>(i) * 3U + 2U] = value;
  }

  // A long ground strip with U running along the receding axis.
  kimia::MeshData strip;
  strip.name = "strip";
  strip.positions = {Vec3{-2.0, 0.0, 0.0}, Vec3{-2.0, 0.0, -40.0}, Vec3{2.0, 0.0, -40.0}, Vec3{2.0, 0.0, 0.0}};
  strip.normals.assign(4U, Vec3{0.0, 1.0, 0.0});
  strip.uvs = {kimia::Vec2{0.0, 0.0}, kimia::Vec2{1.0, 0.0}, kimia::Vec2{1.0, 1.0}, kimia::Vec2{0.0, 1.0}};
  strip.indices = {0U, 2U, 1U, 0U, 3U, 2U};

  kimia::RenderScene scene;
  scene.view = kimia::Mat4::lookAt(Vec3{0.0, 1.0, 3.0}, Vec3{0.0, 0.0, -20.0}, Vec3{0.0, 1.0, 0.0});
  scene.projection = kimia::Mat4::perspective(1.0, 1.0, 0.1, 200.0);
  scene.cameraPosition = Vec3{0.0, 1.0, 3.0};
  scene.lightDirection = Vec3{0.0, -1.0, 0.0};
  scene.ambient = 1.0;
  scene.objects.push_back({&strip, kimia::Mat4{}, Vec3{1.0, 1.0, 1.0}, 0.5, &stripes});

  kimia::Image image;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 200, 200, Vec3{0.0, 0.0, 0.0}, image));

  // Count the stripes visible down the middle of the strip.
  kimia::i32 bands = 0;
  kimia::i32 previous = -1;
  kimia::i32 drawnRows = 0;
  for (kimia::i32 y = 0; y < image.height; ++y) {
    const kimia::u8* pixel = image.at(image.width / 2, y);
    if (pixel[0] == 0U) continue;
    ++drawnRows;
    const kimia::i32 shade = pixel[0] > 128U ? 1 : 0;
    if (shade != previous) ++bands;
    previous = shade;
  }
  KIMIA_REQUIRE(drawnRows > 20);
  // Sixteen stripes exist, but perspective compresses the distant ones
  // into fewer than sixteen distinguishable bands. Affine interpolation
  // measures exactly 16 here; correct interpolation measures 10.
  KIMIA_REQUIRE(bands > 2);
  KIMIA_REQUIRE(bands < 14);
}

KIMIA_TEST(renderer_texture_is_tinted_by_the_object_colour) {
  // A white object shows the image unchanged; a coloured one tints it, so
  // team colours still work on a textured model.
  const kimia::MeshData quad = kimia::makePlane(4.0, 4.0);
  const kimia::Image texture = checkerTexture();

  kimia::RenderScene red = quadScene(quad, &texture);
  red.objects[0].color = Vec3{1.0, 0.0, 0.0};
  kimia::Image image;
  KIMIA_REQUIRE(kimia::renderSoftware(red, 120, 120, Vec3{0.0, 0.0, 0.0}, image));

  // Every drawn pixel keeps its red and loses its green and blue.
  bool sawRed = false;
  for (kimia::usize i = 0; i + 2U < image.pixels.size(); i += 3U) {
    if (image.pixels[i] == 0U) continue;
    KIMIA_REQUIRE(image.pixels[i + 1U] == 0U);
    KIMIA_REQUIRE(image.pixels[i + 2U] == 0U);
    sawRed = true;
  }
  KIMIA_REQUIRE(sawRed);
}

KIMIA_TEST(renderer_survives_a_texture_it_cannot_use) {
  // A mesh with no UVs, or an empty image, must fall back to flat colour
  // rather than reading past the end of anything.
  const kimia::MeshData cube = kimia::makeCube(1.0);
  const kimia::Image texture = checkerTexture();

  kimia::Image image;
  kimia::RenderScene scene = quadScene(cube, &texture);
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 100, 100, Vec3{0.0, 0.0, 0.0}, image));

  // An empty texture attached to a UV'd mesh is equally harmless.
  const kimia::MeshData quad = kimia::makePlane(2.0, 2.0);
  const kimia::Image empty;
  kimia::Image second;
  KIMIA_REQUIRE(kimia::renderSoftware(quadScene(quad, &empty), 100, 100, Vec3{0.0, 0.0, 0.0}, second));
}

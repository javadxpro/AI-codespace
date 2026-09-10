#include <kimia/ColorPipeline.h>
#include <kimia/EGL.h>
#include <kimia/GLFunctions.h>
#include <kimia/Image.h>
#include <kimia/Mesh.h>
#include <kimia/Pbr.h>
#include <kimia/Renderer.h>
#include <kimia_test.h>

#include <cstdio>
#include <string>
#include <vector>

namespace {

using kimia::Image;
using kimia::Mat4;
using kimia::MeshData;
using kimia::PointLight;
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

KIMIA_TEST(display_pipeline_srgb_and_aces_are_exact) {
  using kimia::acesToneMap;
  using kimia::displayEncode;
  using kimia::srgbDecode;
  using kimia::srgbEncode;
  const f64 eps = 1e-4;

  // sRGB transfer: endpoints and a known mid value.
  KIMIA_REQUIRE(srgbEncode(0.0) == 0.0);
  KIMIA_REQUIRE(srgbEncode(1.0) == 1.0);
  KIMIA_REQUIRE(std::abs(srgbEncode(0.5) - 0.735357) < eps);
  KIMIA_REQUIRE(std::abs(srgbEncode(0.003) - 0.03876) < eps);  // linear segment

  // Round-trip: encode then decode returns the input.
  const f64 values[] = {0.0, 0.02, 0.2, 0.5, 0.8, 1.0};
  for (const f64 v : values) KIMIA_REQUIRE(std::abs(srgbDecode(srgbEncode(v)) - v) < eps);

  // ACES tone map: 0 stays 0, mid < 1 < full, and it never exceeds 1.
  KIMIA_REQUIRE(acesToneMap(0.0) == 0.0);
  const f64 a1 = acesToneMap(1.0);
  KIMIA_REQUIRE(std::abs(a1 - 0.803797) < eps);
  KIMIA_REQUIRE(acesToneMap(0.5) < a1);
  KIMIA_REQUIRE(acesToneMap(4.0) <= 1.0);

  // The display encode composes exactly: tone map then sRGB.
  const Vec3 one{1.0, 0.5, 0.25};
  const Vec3 expected{kimia::srgbEncode(acesToneMap(1.0)), kimia::srgbEncode(acesToneMap(0.5)),
                      kimia::srgbEncode(acesToneMap(0.25))};
  const Vec3 got = displayEncode(one);
  KIMIA_REQUIRE(std::abs(got.x - expected.x) < eps);
  KIMIA_REQUIRE(std::abs(got.y - expected.y) < eps);
  KIMIA_REQUIRE(std::abs(got.z - expected.z) < eps);
}

KIMIA_TEST(pbr_cook_torrance_math_matches_reference) {
  using kimia::cookTorrance;
  using kimia::fresnelF0;
  using kimia::fresnelSchlick;
  using kimia::ggxDistribution;
  using kimia::roughnessAlpha;
  using kimia::smithG1;
  const f64 eps = 1e-3;
  const Vec3 white{1.0, 1.0, 1.0};

  // Fresnel F0: 4% for a dielectric, the albedo for a full metal.
  KIMIA_REQUIRE(std::abs(fresnelF0(white, 0.0).x - 0.04) < eps);
  KIMIA_REQUIRE(std::abs(fresnelF0(Vec3{0.5, 0.6, 0.7}, 1.0).x - 0.5) < eps);
  KIMIA_REQUIRE(std::abs(fresnelF0(Vec3{0.5, 0.6, 0.7}, 1.0).z - 0.7) < eps);

  // Schlick: 1.0 at grazing, F0 at normal incidence.
  KIMIA_REQUIRE(std::abs(fresnelSchlick(0.0, Vec3{0.04, 0.04, 0.04}).x - 1.0) < eps);
  KIMIA_REQUIRE(std::abs(fresnelSchlick(1.0, Vec3{0.04, 0.04, 0.04}).x - 0.04) < eps);

  // GGX D at normal incidence is 1/(pi*alpha^2) = 4/pi for alpha 0.5.
  KIMIA_REQUIRE(std::abs(ggxDistribution(1.0, 0.5) - 4.0 / 3.14159265358979323846) < eps);
  KIMIA_REQUIRE(std::abs(smithG1(1.0, 0.5) - 1.0) < eps);
  KIMIA_REQUIRE(std::abs(roughnessAlpha(0.2) - 0.04) < eps);

  // A facing white dielectric under a unit light stays near 1.0 linear
  // (diffuse ~0.96 + a little specular), so existing scenes keep their look.
  const Vec3 facing{0.0, 0.0, 1.0};
  const f64 dielectric = cookTorrance(white, 0.5, 0.0, facing, facing, facing, white).x;
  KIMIA_REQUIRE(dielectric > 1.0 && dielectric < 1.2);

  // Full metal has no diffuse lobe, so facing it is all (stronger) specular.
  const f64 metal = cookTorrance(white, 0.5, 1.0, facing, facing, facing, white).x;
  KIMIA_REQUIRE(metal > dielectric);

  // A light behind the surface contributes nothing.
  KIMIA_REQUIRE(cookTorrance(white, 0.5, 0.0, facing, facing, Vec3{0.0, 0.0, -1.0}, white).x == 0.0);
}

KIMIA_TEST(software_renders_metallic_differently_from_dielectric) {
  const MeshData cube = kimia::makeCube(1.0);
  RenderScene scene = cubeScene();

  scene.objects.push_back({&cube, Mat4{}, Vec3{0.9, 0.7, 0.2}, 0.35, 0.0});
  Image dielectric;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, dielectric));

  scene.objects[0].metallic = 1.0;
  Image metal;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, metal));

  // The same gold cube lit the same way must shade differently once it is a
  // conductor (no diffuse lobe, Fresnel tinted by the albedo).
  const u8* d = dielectric.at(60, 60);
  const u8* m = metal.at(60, 60);
  KIMIA_REQUIRE(std::abs(static_cast<i32>(d[0]) - static_cast<i32>(m[0])) +
                    std::abs(static_cast<i32>(d[1]) - static_cast<i32>(m[1])) +
                    std::abs(static_cast<i32>(d[2]) - static_cast<i32>(m[2])) >
                30);
}

KIMIA_TEST(software_fog_fades_distance_to_the_fog_colour) {
  const MeshData cube = kimia::makeCube(1.0);
  RenderScene scene = cubeScene();
  scene.ambient = 0.0;

  // A red cube with no fog: the centre reads red, not blue.
  scene.objects.push_back({&cube, Mat4{}, Vec3{1.0, 0.0, 0.0}, 1.0});
  Image clear;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, clear));
  const u8* noFog = clear.at(60, 60);
  KIMIA_REQUIRE(noFog[0] > noFog[2] + 60);

  // The same cube inside dense BLUE fog: the centre is swallowed by the
  // fog colour regardless of the red albedo.
  scene.fogColor = Vec3{0.0, 0.0, 1.0};
  scene.fogDensity = 3.0;
  Image foggy;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, foggy));
  const u8* fog = foggy.at(60, 60);
  KIMIA_REQUIRE(fog[2] > 180);
  KIMIA_REQUIRE(fog[2] > fog[0] + 60);
}

KIMIA_TEST(software_emissive_glows_without_any_light) {
  const MeshData cube = kimia::makeCube(1.0);
  RenderScene scene = cubeScene();
  scene.ambient = 0.0;
  // Key light straight UP: the front face (normal +Z) receives nothing.
  scene.lightDirection = Vec3{0.0, 1.0, 0.0};

  // A black cube with no emissive is pitch dark.
  scene.objects.push_back({&cube, Mat4{}, Vec3{0.0, 0.0, 0.0}, 1.0});
  Image dark;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, dark));
  const u8* d = dark.at(60, 60);
  KIMIA_REQUIRE(d[0] < 10 && d[1] < 10 && d[2] < 10);

  // Give it an emissive term and it glows red with no light at all.
  scene.objects[0].emissive = Vec3{1.0, 0.0, 0.0};
  Image glow;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, glow));
  const u8* g = glow.at(60, 60);
  KIMIA_REQUIRE(g[0] > 200);
  KIMIA_REQUIRE(g[1] < 15 && g[2] < 15);
}

KIMIA_TEST(software_transparency_blends_with_the_background) {
  const MeshData cube = kimia::makeCube(1.0);
  RenderScene scene = cubeScene();
  scene.ambient = 0.0;

  // A red cube fully opaque.
  scene.objects.push_back({&cube, Mat4{}, Vec3{1.0, 0.0, 0.0}, 1.0});
  Image opaque;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, opaque));
  const u8* oc = opaque.at(60, 60);

  // The same cube at 50% over a black background reads as darker red: the
  // blend sits clearly between the opaque red and the background.
  scene.objects[0].alpha = 0.5;
  Image translucent;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, translucent));
  const u8* tc = translucent.at(60, 60);

  KIMIA_REQUIRE(oc[0] > 150);
  KIMIA_REQUIRE(tc[0] > 40 && tc[0] < oc[0] - 20);
  KIMIA_REQUIRE(tc[0] > tc[1] + 20);
}

KIMIA_TEST(software_point_light_lights_and_falls_off) {
  // A small floor keeps the triangle centroid close to the screen centre,
  // where flat shading samples the point light.
  const MeshData plane = kimia::makePlane(2.0, 2.0);
  RenderScene scene;
  scene.view = Mat4::lookAt(Vec3{0.0, 3.0, 0.0}, Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 0.0, -1.0});
  scene.projection = Mat4::perspective(3.14159265358979323846 * 0.5, 1.0, 0.1, 100.0);
  scene.cameraPosition = Vec3{0.0, 3.0, 0.0};
  scene.lightDirection = Vec3{0.0, 1.0, 0.0};  // key light points UP: nothing on the floor
  scene.ambient = 0.0;
  scene.objects.push_back({&plane, Mat4{}, Vec3{1.0, 1.0, 1.0}, 1.0});

  // A light straight above the floor's centre lights it...
  PointLight lamp;
  lamp.position = Vec3{0.0, 1.5, 0.0};
  lamp.color = Vec3{1.5, 1.5, 1.5};
  lamp.radius = 3.0;
  scene.pointLights.push_back(lamp);

  Image lit;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, lit));
  const u8* center = lit.at(60, 60);
  KIMIA_REQUIRE(center[0] > 120 && center[1] > 120 && center[2] > 120);

  // ...but with a radius too small to reach the floor, the floor stays
  // black (the key light is neutralized and ambient is 0).
  scene.pointLights[0].radius = 0.5;
  Image dark;
  KIMIA_REQUIRE(kimia::renderSoftware(scene, 120, 120, Vec3{0.0, 0.0, 0.0}, dark));
  const u8* darkCenter = dark.at(60, 60);
  KIMIA_REQUIRE(darkCenter[0] < 10 && darkCenter[1] < 10 && darkCenter[2] < 10);
}

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
  // Lower-middle pixel: the green plane (bright green after tone map+sRGB).
  const u8* planePixel = image.at(80, 110);
  KIMIA_REQUIRE(planePixel[1] > 200);
  KIMIA_REQUIRE(planePixel[1] > planePixel[0] + 60);
  KIMIA_REQUIRE(planePixel[1] > planePixel[2] + 60);
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
  scene.objects.push_back({&quad, kimia::Mat4{}, Vec3{1.0, 1.0, 1.0}, 0.5, 0.0, texture});
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

  // Untextured, the quad is at most one shade per triangle. Textured, the
  // checker's grey levels appear on top of that shading (PBR specular also
  // varies per triangle now, so we assert at-least rather than exactly).
  KIMIA_REQUIRE(distinctGreys(plain) >= 1U && distinctGreys(plain) <= 2U);
  KIMIA_REQUIRE(distinctGreys(textured) >= 2U);
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
  scene.objects.push_back({&strip, kimia::Mat4{}, Vec3{1.0, 1.0, 1.0}, 0.5, 0.0, &stripes});

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

  // The red tint dominates: Cook-Torrance adds a faint WHITE dielectric
  // specular (F0 = 4% regardless of albedo) so green/blue are a small
  // residual, but the red energy stays many times larger overall.
  kimia::u64 sumR = 0, sumG = 0, sumB = 0;
  for (kimia::usize i = 0; i + 2U < image.pixels.size(); i += 3U) {
    if (image.pixels[i] == 0U) continue;
    sumR += image.pixels[i];
    sumG += image.pixels[i + 1U];
    sumB += image.pixels[i + 2U];
  }
  KIMIA_REQUIRE(sumR > 0U);
  KIMIA_REQUIRE(sumR > sumG * 5U);
  KIMIA_REQUIRE(sumR > sumB * 5U);
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

#include <kimia/Raytracer.h>
#include <kimia/World.h>
#include <kimia_test.h>

#include <cmath>
#include <string>

namespace {

using kimia::EntityData;
using kimia::MeshKind;
using kimia::Vec3;
using kimia::WorldData;
using kimia::f64;
using kimia::i32;
using kimia::u8;
using kimia::raytrace::RaytraceCamera;
using kimia::raytrace::RaytraceScene;
using kimia::raytrace::RaytraceSettings;
using kimia::raytrace::buildFromWorld;
using kimia::raytrace::render;

bool near(f64 a, f64 b, f64 tolerance = 1e-9) { return std::abs(a - b) <= tolerance; }

WorldData groundWorld() {
  WorldData world;
  kimia::buildEmptyWorldScene(world);
  return world;
}

WorldData ballWorld() {
  // The ball entity placed at (1, 0, 0) instead of the default rest spot:
  // against the default-rest world the image must change where the ball is.
  WorldData world = groundWorld();
  EntityData ball;
  ball.name = "Ball";
  ball.mesh = MeshKind::sphere;
  ball.transform.position = Vec3{1.0, 0.12, 0.0};
  ball.transform.scale = Vec3{0.24, 0.24, 0.24};  // diameter; rendered as 0.12 radius
  ball.color = Vec3{0.95, 0.95, 0.92};
  ball.roughness = 0.3;
  world.scene.create(ball);
  return world;
}

WorldData blockWorld() {
  WorldData world = groundWorld();
  EntityData block;
  block.name = "Block_1";
  block.mesh = MeshKind::cube;
  block.transform.position = Vec3{0.0, 1.0, 0.0};
  block.transform.scale = Vec3{2.0, 2.0, 2.0};
  block.color = Vec3{0.6, 0.6, 0.6};
  block.roughness = 0.5;
  world.scene.create(block);
  return world;
}

RaytraceSettings smallSettings() {
  RaytraceSettings settings;
  settings.width = 96;
  settings.height = 64;
  settings.samplesPerPixel = 8;
  settings.maxBounces = 2;
  return settings;
}

RaytraceCamera defaultCamera() {
  RaytraceCamera camera;
  camera.eye = Vec3{0.0, 3.6, 6.2};
  camera.target = Vec3{0.0, 0.2, 0.0};
  return camera;
}

kimia::Image renderWorld(const WorldData& world, const RaytraceSettings& settings,
                         const RaytraceCamera& camera) {
  RaytraceScene scene;
  std::string error;
  KIMIA_REQUIRE(buildFromWorld(world, scene, error));
  kimia::Image image;
  KIMIA_REQUIRE(render(scene, settings, camera, image));
  return image;
}

f64 luminance(const kimia::Image& image, i32 x, i32 y) {
  const u8* pixel = image.at(x, y);
  return 0.2126 * static_cast<f64>(pixel[0]) + 0.7152 * static_cast<f64>(pixel[1]) +
         0.0722 * static_cast<f64>(pixel[2]);
}

}  // namespace

KIMIA_TEST(raytrace_sky_and_floor_have_real_colors) {
  const WorldData world = groundWorld();
  const kimia::Image image = renderWorld(world, smallSettings(), defaultCamera());
  // Top of the frame is sky: blue dominates red.
  const u8* sky = image.at(image.width / 2, 2);
  KIMIA_REQUIRE(sky[2] > sky[0]);
  KIMIA_REQUIRE(sky[2] > sky[1]);
  // Bottom center is the lit green floor: green dominates.
  const u8* floor = image.at(image.width / 2, image.height - 3);
  KIMIA_REQUIRE(floor[1] > floor[0]);
  KIMIA_REQUIRE(floor[1] > floor[2]);
}

KIMIA_TEST(raytrace_is_deterministic) {
  const WorldData world = blockWorld();
  const RaytraceSettings settings = smallSettings();
  const kimia::Image first = renderWorld(world, settings, defaultCamera());
  const kimia::Image second = renderWorld(world, settings, defaultCamera());
  KIMIA_REQUIRE(first.pixels.size() == second.pixels.size());
  KIMIA_REQUIRE(first.pixels == second.pixels);  // byte-identical
}

KIMIA_TEST(raytrace_ball_changes_the_image) {
  const RaytraceSettings settings = smallSettings();
  RaytraceCamera camera;
  camera.eye = Vec3{0.0, 1.5, 3.0};  // close up: the ball is clearly visible
  camera.target = Vec3{0.0, 0.3, 0.0};
  // One world has the ball at the default rest spot, the other at (1, 0):
  // only the ball moved, so the images must differ where it is.
  const kimia::Image rest = renderWorld(groundWorld(), settings, camera);
  const kimia::Image withBall = renderWorld(ballWorld(), settings, camera);
  i32 different = 0;
  for (i32 y = 0; y < settings.height; ++y) {
    for (i32 x = 0; x < settings.width; ++x) {
      const u8* a = rest.at(x, y);
      const u8* b = withBall.at(x, y);
      if (std::abs(static_cast<i32>(a[0]) - static_cast<i32>(b[0])) +
              std::abs(static_cast<i32>(a[1]) - static_cast<i32>(b[1])) +
              std::abs(static_cast<i32>(a[2]) - static_cast<i32>(b[2])) >
          5) {
        ++different;
      }
    }
  }
  KIMIA_REQUIRE(different > 10);  // the ball visibly occupies the frame
}

KIMIA_TEST(raytrace_bvh_matches_bruteforce_exactly) {
  const WorldData world = ballWorld();
  RaytraceSettings settings;
  settings.width = 32;
  settings.height = 24;
  settings.samplesPerPixel = 2;
  settings.maxBounces = 1;
  RaytraceScene scene;
  std::string error;
  KIMIA_REQUIRE(buildFromWorld(world, scene, error));
  kimia::Image bvhImage;
  kimia::Image bruteImage;
  KIMIA_REQUIRE(render(scene, settings, defaultCamera(), false, bvhImage));
  KIMIA_REQUIRE(render(scene, settings, defaultCamera(), true, bruteImage));
  KIMIA_REQUIRE(bvhImage.pixels == bruteImage.pixels);
}

KIMIA_TEST(raytrace_block_casts_a_soft_shadow) {
  const WorldData world = blockWorld();
  RaytraceSettings settings;
  settings.width = 128;
  settings.height = 96;
  settings.samplesPerPixel = 16;
  settings.maxBounces = 2;
  settings.sunDirection = Vec3{0.45, 0.5, 0.45};  // 38 deg elevation, long shadows
  settings.sunIntensity = 4.0;
  settings.skyIntensity = 0.15;
  // Straight-down camera: image right = world +X, image up = world -Z.
  // Light travels toward +X+Z, so the 2x2x2 block casts its shadow toward
  // -X and -Z (image left and top). Measured: lit patch ~155 vs shadow
  // patch ~110 luminance.
  RaytraceCamera camera;
  camera.eye = Vec3{0.0, 6.0, 0.001};
  camera.target = Vec3{0.0, 0.0, 0.0};
  camera.up = Vec3{0.0, 0.0, -1.0};
  const kimia::Image image = renderWorld(world, settings, camera);

  auto meanLum = [&image](i32 x0, i32 x1, i32 y0, i32 y1) {
    f64 sum = 0.0;
    i32 count = 0;
    for (i32 y = y0; y < y1; ++y) {
      for (i32 x = x0; x < x1; ++x) {
        sum += luminance(image, x, y);
        ++count;
      }
    }
    return sum / static_cast<f64>(count);
  };
  const f64 shadowSide = meanLum(8, 40, 8, 34);      // floor in the shadow
  const f64 litSide = meanLum(88, 120, 8, 34);       // lit floor, same band
  KIMIA_REQUIRE(litSide > shadowSide * 1.25);
}


// --- Stage 24: the world's hour and weather light the render ---

KIMIA_TEST(raytrace_sky_follows_the_hour_and_the_rain) {
  kimia::WorldData world;
  kimia::raytrace::RaytraceSettings settings;

  // Midday: the sun is overhead, so the light travels straight down.
  world.profile.hour = 12.0;
  world.profile.rain = 0.0;
  kimia::raytrace::applyWorldSky(world, settings);
  KIMIA_REQUIRE(settings.sunDirection.y < -0.8);  // pointing down = sun up
  const kimia::f64 noonSun = settings.sunIntensity;
  const kimia::f64 noonSky = settings.skyIntensity;
  KIMIA_REQUIRE(near(noonSun, 2.2, 1e-12));
  KIMIA_REQUIRE(near(noonSky, 0.6, 1e-12));

  // Midnight: no sun at all, and only a dim sky (the floodlights).
  world.profile.hour = 0.0;
  kimia::raytrace::applyWorldSky(world, settings);
  KIMIA_REQUIRE(settings.sunIntensity == 0.0);
  KIMIA_REQUIRE(near(settings.skyIntensity, 0.10, 1e-12));
  KIMIA_REQUIRE(settings.skyIntensity < noonSky);
  KIMIA_REQUIRE(settings.sunDirection.y > 0.8);  // "sun" below the horizon

  // Morning and evening put the sun on opposite sides of the sky.
  world.profile.hour = 8.0;
  kimia::raytrace::applyWorldSky(world, settings);
  const kimia::f64 morningX = settings.sunDirection.x;
  world.profile.hour = 16.0;
  kimia::raytrace::applyWorldSky(world, settings);
  KIMIA_REQUIRE(morningX * settings.sunDirection.x < 0.0);

  // Rain at midday: still daytime, but dimmer than a clear noon.
  world.profile.hour = 12.0;
  world.profile.rain = 1.0;
  kimia::raytrace::applyWorldSky(world, settings);
  KIMIA_REQUIRE(settings.sunIntensity < noonSun);
  KIMIA_REQUIRE(near(settings.sunIntensity, 2.2 * 0.35, 1e-12));
  KIMIA_REQUIRE(settings.skyIntensity < noonSky);

  // The sun direction is always a unit vector, whatever the hour.
  for (kimia::f64 hour = 0.0; hour <= 24.0; hour += 3.0) {
    world.profile.hour = hour;
    kimia::raytrace::applyWorldSky(world, settings);
    KIMIA_REQUIRE(near(settings.sunDirection.length(), 1.0, 1e-12));
  }
}

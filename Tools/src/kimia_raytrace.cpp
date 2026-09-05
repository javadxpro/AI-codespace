// kimia_raytrace — offline path-traced render preview for KIMIA worlds.
//
// Usage:
//   kimia_raytrace <world.kimia> <out.png> [--width N] [--height N]
//                  [--spp N] [--bounces N] [--eye x y z] [--target x y z]
//                  [--sun x y z] [--intensity F] [--sky F] [--exposure F]
//
// Renders the world with PBR direct light (Cook-Torrance GGX), soft sun
// shadows and path-traced global illumination. Pure CPU, deterministic:
// the same input produces a byte-identical PNG on any platform.

#include <kimia/AssetPipeline.h>
#include <kimia/Raytracer.h>
#include <kimia/World.h>
#include <kimia/WorldIO.h>

#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <string>

namespace {

bool parseF64(const char* text, kimia::f64& out) {
  if (text == nullptr || *text == '\0') return false;
  char* end = nullptr;
  out = std::strtod(text, &end);
  return end != text && *end == '\0';
}

void printUsage() {
  std::printf("usage: kimia_raytrace <world.kimia> <out.png> [options]\n");
  std::printf("  --width N      output width (default 480)\n");
  std::printf("  --height N     output height (default 360)\n");
  std::printf("  --spp N        samples per pixel (default 32)\n");
  std::printf("  --bounces N    indirect bounces (default 3)\n");
  std::printf("  --eye x y z    camera position\n");
  std::printf("  --target x y z camera look-at\n");
  std::printf("  --up x y z     camera up vector\n");
  std::printf("  --sun x y z    sun direction (normalized on use)\n");
  std::printf("  --intensity F  sun intensity (default 2.2)\n");
  std::printf("  --sky F        sky environment intensity (default 0.6)\n");
  std::printf("  --exposure F   exposure multiplier (default 1.0)\n");
  std::printf("  --brute        skip the BVH (test hook)\n");
}

}  // namespace

int main(int argc, char** argv) {
  std::string worldPath;
  std::string outPath;
  kimia::raytrace::RaytraceSettings settings;
  kimia::raytrace::RaytraceCamera camera;
  bool bruteForce = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto nextF64 = [&](kimia::f64& out) -> bool {
      if (i + 1 >= argc) return false;
      return parseF64(argv[++i], out);
    };
    auto nextVec3 = [&](kimia::Vec3& out) -> bool {
      kimia::f64 x = 0.0, y = 0.0, z = 0.0;
      if (i + 3 >= argc) return false;
      if (!parseF64(argv[++i], x) || !parseF64(argv[++i], y) || !parseF64(argv[++i], z)) return false;
      out = kimia::Vec3{x, y, z};
      return true;
    };
    kimia::f64 scalar = 0.0;
    if (arg == "--help" || arg == "-h") {
      printUsage();
      return 0;
    } else if (arg == "--width" && nextF64(scalar)) {
      settings.width = static_cast<kimia::i32>(scalar);
    } else if (arg == "--height" && nextF64(scalar)) {
      settings.height = static_cast<kimia::i32>(scalar);
    } else if (arg == "--spp" && nextF64(scalar)) {
      settings.samplesPerPixel = static_cast<kimia::i32>(scalar);
    } else if (arg == "--bounces" && nextF64(scalar)) {
      settings.maxBounces = static_cast<kimia::i32>(scalar);
    } else if (arg == "--eye" && nextVec3(camera.eye)) {
    } else if (arg == "--target" && nextVec3(camera.target)) {
    } else if (arg == "--up" && nextVec3(camera.up)) {
    } else if (arg == "--sun" && nextVec3(settings.sunDirection)) {
    } else if (arg == "--intensity" && nextF64(settings.sunIntensity)) {
    } else if (arg == "--sky" && nextF64(settings.skyIntensity)) {
    } else if (arg == "--exposure" && nextF64(settings.exposure)) {
    } else if (arg == "--brute") {
      bruteForce = true;
    } else if (!arg.empty() && arg[0] == '-') {
      std::fprintf(stderr, "unknown option: %s\n", arg.c_str());
      printUsage();
      return 1;
    } else if (worldPath.empty()) {
      worldPath = arg;
    } else if (outPath.empty()) {
      outPath = arg;
    } else {
      std::fprintf(stderr, "unexpected argument: %s\n", arg.c_str());
      printUsage();
      return 1;
    }
  }
  if (worldPath.empty() || outPath.empty()) {
    printUsage();
    return 1;
  }

  kimia::WorldData world;
  std::string error;
  if (!kimia::WorldIO::loadFromFile(worldPath, world, error)) {
    std::fprintf(stderr, "ERROR %s: %s\n", worldPath.c_str(), error.c_str());
    return 1;
  }

  kimia::raytrace::RaytraceScene scene;
  if (!kimia::raytrace::buildFromWorld(world, scene, error)) {
    std::fprintf(stderr, "ERROR building scene: %s\n", error.c_str());
    return 1;
  }
  if (!error.empty()) std::printf("note: %s\n", error.c_str());

  kimia::Image image;
  const auto started = std::chrono::steady_clock::now();
  if (!kimia::raytrace::render(scene, settings, camera, bruteForce, image)) {
    std::fprintf(stderr, "ERROR: render failed (empty scene or bad settings)\n");
    return 1;
  }
  const auto ended = std::chrono::steady_clock::now();
  const kimia::f64 seconds = static_cast<kimia::f64>(
      std::chrono::duration_cast<std::chrono::microseconds>(ended - started).count()) / 1000000.0;

  if (!image.writePNG(outPath)) {
    std::fprintf(stderr, "ERROR: cannot write %s\n", outPath.c_str());
    return 1;
  }

  std::printf("RENDER %s -> %s | %dx%d | %d spp | %d bounces | %.2f s | %zu triangles\n", worldPath.c_str(),
              outPath.c_str(), settings.width, settings.height, settings.samplesPerPixel, settings.maxBounces,
              seconds, scene.triangleCount());
  return 0;
}

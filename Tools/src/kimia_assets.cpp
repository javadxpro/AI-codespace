// kimia_assets — KIMIA asset pipeline CLI.
//
// Converts/verifies game assets into engine formats:
//   .obj (+ .mtl) / .fbx  -> .kimiamesh (KIMIA mesh text v1; materials reported)
//   .png / .jpg/.jpeg     -> .kimiimage (info) + .kimi.png / .kimi.jpg (re-encoded)
//   .wav / .mp3 / .ogg / .flac -> .kimiaaudio (info) + .kimi.wav (16-bit PCM)
//
// Usage: kimia_assets [--quiet] <file> [<file> ...]
// Exit code: 0 = everything converted, 1 = at least one failure.

#include <kimia/AssetPipeline.h>
#include <kimia/Audio.h>
#include <kimia/Image.h>
#include <kimia/Mesh.h>

#include <cstdio>
#include <string>
#include <vector>

using kimia::usize;

namespace {

using kimia::assets::AssetType;
using kimia::assets::detectType;

std::string replaceExtension(const std::string& path, const std::string& suffix) {
  const usize slash = path.find_last_of("/\\");
  const usize dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos || (slash != std::string::npos && dotPos < slash)) return path + suffix;
  return path.substr(0, dotPos) + suffix;
}

std::string extensionOf(const std::string& path) {
  const usize slash = path.find_last_of("/\\");
  const usize dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos || (slash != std::string::npos && dotPos < slash)) return "";
  std::string ext = path.substr(dotPos);
  for (char& c : ext) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return ext;
}

void printUsage() {
  std::printf(
      "kimia_assets - KIMIA asset pipeline\n"
      "\n"
      "Usage: kimia_assets [--quiet] <file> [<file> ...]\n"
      "\n"
      "Formats:\n"
      "  .obj (+.mtl) .fbx -> .kimiamesh  (KIMIA mesh text v1; materials reported)\n"
      "  .png .jpg .jpeg   -> .kimiimage  (info) + re-encoded .kimi.png / .kimi.jpg\n"
      "  .wav .mp3 .ogg .flac -> .kimiaaudio (info) + .kimi.wav (16-bit PCM)\n");
}

int convertMesh(const std::string& path, bool quiet) {
  std::string error;
  std::optional<kimia::assets::MeshAsset> asset;
  std::string format;
  if (path.size() >= 4U && path.compare(path.size() - 4U, 4U, ".obj") == 0) {
    asset = kimia::assets::loadOBJAsset(path, error);
    format = "obj";
  } else {
    asset = kimia::assets::loadFBXAsset(path, error);
    format = "fbx";
  }
  if (!asset.has_value()) {
    std::fprintf(stderr, "ERROR %s: %s\n", path.c_str(), error.c_str());
    return 1;
  }
  const kimia::MeshData& mesh = asset->mesh;
  if (!quiet) {
    std::printf("MESH %s : %s | %s | %llu verts | %llu triangles | %llu materials | %llu sub-meshes\n",
                path.c_str(), mesh.name.c_str(), format.c_str(), static_cast<unsigned long long>(mesh.vertexCount()),
                static_cast<unsigned long long>(mesh.triangleCount()),
                static_cast<unsigned long long>(asset->materials.size()),
                static_cast<unsigned long long>(asset->subMeshes.size()));
    for (const kimia::MaterialData& material : asset->materials) {
      std::printf("  material %s | color %.3f %.3f %.3f | texture %s\n", material.name.c_str(), material.color.x,
                  material.color.y, material.color.z,
                  material.texturePath.empty() ? "(none)" : material.texturePath.c_str());
    }
  }
  std::string text;
  if (!kimia::meshToText(mesh, text)) {
    std::fprintf(stderr, "ERROR %s: mesh failed validation\n", path.c_str());
    return 1;
  }
  const std::string outPath = replaceExtension(path, ".kimiamesh");
  std::FILE* file = std::fopen(outPath.c_str(), "wb");
  if (file == nullptr) {
    std::fprintf(stderr, "ERROR: cannot write %s\n", outPath.c_str());
    return 1;
  }
  std::fwrite(text.data(), 1, text.size(), file);
  std::fclose(file);
  if (!quiet) std::printf("  -> wrote %s\n", outPath.c_str());
  return 0;
}

int convertImage(const std::string& path, bool quiet) {
  std::string error;
  auto image = kimia::assets::loadImage(path, error);
  if (!image.has_value()) {
    std::fprintf(stderr, "ERROR %s: %s\n", path.c_str(), error.c_str());
    return 1;
  }
  if (!quiet) {
    std::printf("IMAGE %s : %dx%d | %d channels\n", path.c_str(), image->width, image->height, image->channels);
  }
  int failures = 0;
  const std::string pngPath = replaceExtension(path, ".kimi.png");
  if (!image->writePNG(pngPath)) {
    std::fprintf(stderr, "ERROR: cannot write %s\n", pngPath.c_str());
    ++failures;
  } else if (!quiet) {
    std::printf("  -> wrote %s\n", pngPath.c_str());
  }
  const std::string jpgPath = replaceExtension(path, ".kimi.jpg");
  if (!image->writeJPG(jpgPath, 92)) {
    std::fprintf(stderr, "ERROR: cannot write %s\n", jpgPath.c_str());
    ++failures;
  } else if (!quiet) {
    std::printf("  -> wrote %s\n", jpgPath.c_str());
  }
  const std::string infoPath = replaceExtension(path, ".kimiimage");
  std::FILE* file = std::fopen(infoPath.c_str(), "wb");
  if (file == nullptr) {
    std::fprintf(stderr, "ERROR: cannot write %s\n", infoPath.c_str());
    ++failures;
  } else {
    std::fprintf(file, "# KIMIA image v1\nwidth %d\nheight %d\nchannels %d\n", image->width, image->height,
                 image->channels);
    std::fclose(file);
    if (!quiet) std::printf("  -> wrote %s\n", infoPath.c_str());
  }
  return failures == 0 ? 0 : 1;
}

int convertAudio(const std::string& path, bool quiet) {
  std::string error;
  auto audio = kimia::assets::loadAudio(path, error);
  if (!audio.has_value()) {
    std::fprintf(stderr, "ERROR %s: %s\n", path.c_str(), error.c_str());
    return 1;
  }
  if (!quiet) {
    std::printf("AUDIO %s : %d ch | %d Hz | %.3f s\n", path.c_str(), audio->channels, audio->sampleRate,
                audio->durationSeconds());
  }
  int failures = 0;
  const std::string wavPath = replaceExtension(path, ".kimi.wav");
  if (!audio->writeWAV(wavPath)) {
    std::fprintf(stderr, "ERROR: cannot write %s\n", wavPath.c_str());
    ++failures;
  } else if (!quiet) {
    std::printf("  -> wrote %s\n", wavPath.c_str());
  }
  const std::string infoPath = replaceExtension(path, ".kimiaaudio");
  std::FILE* file = std::fopen(infoPath.c_str(), "wb");
  if (file == nullptr) {
    std::fprintf(stderr, "ERROR: cannot write %s\n", infoPath.c_str());
    ++failures;
  } else {
    std::fprintf(file, "# KIMIA audio v1\nchannels %d\nsampleRate %d\nframes %llu\n", audio->channels, audio->sampleRate,
                 static_cast<unsigned long long>(audio->frameCount));
    std::fclose(file);
    if (!quiet) std::printf("  -> wrote %s\n", infoPath.c_str());
  }
  return failures == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> paths;
  bool quiet = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUsage();
      return 0;
    }
    if (arg == "--quiet" || arg == "-q") {
      quiet = true;
      continue;
    }
    if (arg.size() >= 2U && arg[0] == '-') {
      std::fprintf(stderr, "unknown option: %s\n", arg.c_str());
      printUsage();
      return 1;
    }
    paths.push_back(arg);
  }
  if (paths.empty()) {
    printUsage();
    return 1;
  }
  int failures = 0;
  for (const std::string& path : paths) {
    const auto type = detectType(path);
    if (!type.has_value()) {
      if (extensionOf(path) == ".blend") {
        std::fprintf(stderr,
                     "ERROR %s: .blend files are not read directly. Export the model from Blender "
                     "instead (File > Export > Wavefront .obj with materials, or FBX with embedded textures), "
                     "then run kimia_assets on the exported file. See Documentation/Assets.md.\n",
                     path.c_str());
      } else {
        std::fprintf(stderr, "ERROR %s: unsupported extension (expected .obj .fbx .ogg .flac .png .jpg .jpeg .wav .mp3)\n",
                     path.c_str());
      }
      ++failures;
      continue;
    }
    switch (*type) {
      case AssetType::mesh:
        failures += convertMesh(path, quiet);
        break;
      case AssetType::image:
        failures += convertImage(path, quiet);
        break;
      case AssetType::audio:
        failures += convertAudio(path, quiet);
        break;
    }
  }
  return failures == 0 ? 0 : 1;
}

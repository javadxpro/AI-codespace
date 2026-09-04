#include <kimia/AssetPipeline.h>
#include <kimia/Audio.h>
#include <kimia/Image.h>
#include <kimia/Mesh.h>
#include <kimia/Vec.h>
#include <kimia_test.h>

#include <algorithm>
#include <cmath>
#include <string>

#ifndef KIMIA_ASSET_DIR
#error "KIMIA_ASSET_DIR must be defined by CMake"
#endif
#ifndef KIMIA_TEST_TMP
#error "KIMIA_TEST_TMP must be defined by CMake"
#endif

#include <sys/stat.h>

namespace {
using kimia::Vec2;
using kimia::Vec3;
using kimia::f32;
using kimia::f64;
using kimia::u8;
using kimia::usize;

const std::string kAssets = std::string(KIMIA_ASSET_DIR) + "/";

// Test outputs go into the (gitignored) build directory, never into the
// source tree, no matter where the binary is run from.
std::string tmpPath(const std::string& name) {
  static const bool created = ::mkdir(KIMIA_TEST_TMP, 0755) == 0 || errno == EEXIST;
  static_cast<void>(created);
  return std::string(KIMIA_TEST_TMP) + "/" + name;
}

constexpr f64 kEps = 1e-9;

bool near3(const Vec3& a, const Vec3& b, f64 eps = kEps) {
  return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps && std::abs(a.z - b.z) <= eps;
}

bool nearVec3List(const std::vector<Vec3>& a, const std::vector<Vec3>& b, f64 eps = 1e-6) {
  if (a.size() != b.size()) return false;
  for (usize i = 0; i < a.size(); ++i) {
    if (!near3(a[i], b[i], eps)) return false;
  }
  return true;
}
}  // namespace

// --- OBJ ---

KIMIA_TEST(obj_loads_generated_cube) {
  kimia::MeshData mesh;
  std::string error;
  KIMIA_REQUIRE(kimia::loadFromOBJFile(kAssets + "cube.obj", mesh, error));
  KIMIA_REQUIRE(mesh.name == "Cube");
  KIMIA_REQUIRE(mesh.positions.size() == 24U);
  KIMIA_REQUIRE(mesh.indices.size() == 36U);
  KIMIA_REQUIRE(mesh.isValid());
  // First corner is (-0.5, -0.5, -0.5) with normal (0, 0, -1).
  KIMIA_REQUIRE(near3(mesh.positions[0], Vec3{-0.5, -0.5, -0.5}));
  KIMIA_REQUIRE(near3(mesh.normals[0], Vec3{0.0, 0.0, -1.0}));
  for (const Vec3& n : mesh.normals) {
    KIMIA_REQUIRE(std::abs(n.length() - 1.0) <= 1e-9);
  }
  for (const Vec2& uv : mesh.uvs) {
    KIMIA_REQUIRE(uv.x >= -1e-9 && uv.x <= 1.0 + 1e-9);
    KIMIA_REQUIRE(uv.y >= -1e-9 && uv.y <= 1.0 + 1e-9);
  }
  // All faces CCW from outside (dot(cross(e1,e2), n) > 0).
  for (usize i = 0; i + 2U < mesh.indices.size(); i += 3U) {
    const Vec3 p0 = mesh.positions[mesh.indices[i]];
    const Vec3 p1 = mesh.positions[mesh.indices[i + 1U]];
    const Vec3 p2 = mesh.positions[mesh.indices[i + 2U]];
    KIMIA_REQUIRE(kimia::dot(kimia::cross(p1 - p0, p2 - p0), mesh.normals[mesh.indices[i]]) > 0.0);
  }
}

KIMIA_TEST(obj_dedupe_merges_shared_triplets) {
  kimia::MeshData mesh;
  std::string error;
  KIMIA_REQUIRE(kimia::loadFromOBJFile(kAssets + "quad.obj", mesh, error));
  KIMIA_REQUIRE(mesh.positions.size() == 6U);  // no dedupe: 2 triangles x 3 verts
  kimia::MeshData deduped;
  KIMIA_REQUIRE(kimia::loadFromOBJFile(kAssets + "quad.obj", deduped, error, true));
  KIMIA_REQUIRE(deduped.positions.size() == 4U);  // shared full tuple merged
  KIMIA_REQUIRE(deduped.indices.size() == 6U);
}

KIMIA_TEST(obj_missing_file_returns_error) {
  kimia::MeshData mesh;
  std::string error;
  KIMIA_REQUIRE(!kimia::loadFromOBJFile(kAssets + "does_not_exist.obj", mesh, error));
  KIMIA_REQUIRE(!error.empty());
}

// --- FBX ---

KIMIA_TEST(fbx_binary_cube_loads) {
  std::string error;
  auto loaded = kimia::assets::loadMesh(kAssets + "box.fbx", error);
  KIMIA_REQUIRE(loaded.has_value());
  const kimia::MeshData& mesh = loaded->mesh;
  KIMIA_REQUIRE(loaded->sourceFormat == "fbx");
  KIMIA_REQUIRE(mesh.isValid());
  KIMIA_REQUIRE(mesh.vertexCount() >= 8U);
  KIMIA_REQUIRE(mesh.triangleCount() >= 12U);
  KIMIA_REQUIRE(!mesh.name.empty());
  // Normals must be unit-length (ufbx generates them when missing).
  for (const Vec3& n : mesh.normals) {
    KIMIA_REQUIRE(std::abs(n.length() - 1.0) <= 1e-3);
  }
}

KIMIA_TEST(fbx_blender_cube_24v_36i) {
  std::string error;
  auto loaded = kimia::assets::loadMesh(kAssets + "blender_cube.fbx", error);
  KIMIA_REQUIRE(loaded.has_value());
  KIMIA_REQUIRE(loaded->mesh.positions.size() == 24U);
  KIMIA_REQUIRE(loaded->mesh.indices.size() == 36U);
}

KIMIA_TEST(fbx_missing_file_returns_error) {
  std::string error;
  KIMIA_REQUIRE(!kimia::assets::loadMesh(kAssets + "missing.fbx", error).has_value());
  KIMIA_REQUIRE(!error.empty());
}

// --- Mesh text format ---

KIMIA_TEST(mesh_text_roundtrip_identical) {
  std::string error;
  kimia::MeshData source;
  KIMIA_REQUIRE(kimia::loadFromOBJFile(kAssets + "cube.obj", source, error));
  std::string text;
  KIMIA_REQUIRE(kimia::meshToText(source, text));
  kimia::MeshData loaded;
  KIMIA_REQUIRE(kimia::meshFromText(text, loaded, error));
  KIMIA_REQUIRE(loaded.name == source.name);
  KIMIA_REQUIRE(loaded.indices == source.indices);
  KIMIA_REQUIRE(nearVec3List(loaded.positions, source.positions));
  KIMIA_REQUIRE(nearVec3List(loaded.normals, source.normals));
  KIMIA_REQUIRE(loaded.uvs.size() == source.uvs.size());
  for (usize i = 0; i < loaded.uvs.size(); ++i) {
    KIMIA_REQUIRE(std::abs(loaded.uvs[i].x - source.uvs[i].x) <= 1e-6);
    KIMIA_REQUIRE(std::abs(loaded.uvs[i].y - source.uvs[i].y) <= 1e-6);
  }
}

KIMIA_TEST(mesh_text_tolerant_load) {
  std::string error;
  kimia::MeshData source;
  KIMIA_REQUIRE(kimia::loadFromOBJFile(kAssets + "cube.obj", source, error));
  std::string text;
  KIMIA_REQUIRE(kimia::meshToText(source, text));
  // Garbage lines and unknown keywords must be skipped.
  text = "# extra comment\nunknown_token 1 2 3\n" + text + "\n# trailing comment\n";
  kimia::MeshData loaded;
  KIMIA_REQUIRE(kimia::meshFromText(text, loaded, error));
  KIMIA_REQUIRE(loaded.positions.size() == source.positions.size());
  KIMIA_REQUIRE(loaded.indices == source.indices);
}

// --- Images (PNG / JPG) ---

KIMIA_TEST(image_png_roundtrip) {
  std::string error;
  auto image = kimia::Image::load(kAssets + "2x3.png", error);
  KIMIA_REQUIRE(image.has_value());
  KIMIA_REQUIRE(image->width == 6 && image->height == 3 && image->channels == 3);
  const u8* topLeft = image->at(0, 0);
  KIMIA_REQUIRE(topLeft[0] == 255 && topLeft[1] == 0 && topLeft[2] == 0);
  const u8* bottomRight = image->at(5, 2);
  KIMIA_REQUIRE(bottomRight[0] == 64 && bottomRight[1] == 0 && bottomRight[2] == 64);
  const u8* middle = image->at(2, 1);
  KIMIA_REQUIRE(middle[0] == 191 && middle[1] == 191 && middle[2] == 0);  // yellow * 0.75
  // Encode to PNG bytes and reload: lossless, must be identical.
  const std::vector<u8> pngBytes = image->encodePNG();
  KIMIA_REQUIRE(pngBytes.size() > 8U);
  KIMIA_REQUIRE(image->writePNG(tmpPath("png_roundtrip.png")));
  auto reloaded = kimia::Image::load(tmpPath("png_roundtrip.png"), error);
  KIMIA_REQUIRE(reloaded.has_value());
  KIMIA_REQUIRE(reloaded->width == 6 && reloaded->height == 3 && reloaded->channels == 3);
  KIMIA_REQUIRE(reloaded->pixels == image->pixels);
}

KIMIA_TEST(image_jpg_roundtrip) {
  std::string error;
  auto image = kimia::Image::load(kAssets + "2x2.jpg", error);
  KIMIA_REQUIRE(image.has_value());
  KIMIA_REQUIRE(image->width == 2 && image->height == 2 && image->channels == 3);
  // JPEG is lossy: allow a small tolerance.
  const u8* white = image->at(0, 0);
  KIMIA_REQUIRE(white[0] > 240 && white[1] > 240 && white[2] > 240);
  const u8* green = image->at(1, 1);
  KIMIA_REQUIRE(green[0] < 20 && green[1] > 235 && green[2] < 20);
  KIMIA_REQUIRE(image->writeJPG(tmpPath("jpg_roundtrip.jpg"), 95));
  auto reloaded = kimia::Image::load(tmpPath("jpg_roundtrip.jpg"), error);
  KIMIA_REQUIRE(reloaded.has_value());
  KIMIA_REQUIRE(reloaded->width == 2 && reloaded->height == 2);
  const u8* green2 = reloaded->at(1, 1);
  KIMIA_REQUIRE(green2[1] > 235 && green2[0] < 20 && green2[2] < 20);
}

KIMIA_TEST(image_missing_file_returns_error) {
  std::string error;
  KIMIA_REQUIRE(!kimia::Image::load(kAssets + "missing.png", error).has_value());
  KIMIA_REQUIRE(!error.empty());
}

// --- Audio (WAV / MP3) ---

KIMIA_TEST(audio_wav_tone_roundtrip) {
  std::string error;
  auto audio = kimia::AudioBuffer::load(kAssets + "tone.wav", error);
  KIMIA_REQUIRE(audio.has_value());
  KIMIA_REQUIRE(audio->channels == 2);
  KIMIA_REQUIRE(audio->sampleRate == 44100);
  KIMIA_REQUIRE(audio->frameCount == 22050U);
  KIMIA_REQUIRE(std::abs(audio->durationSeconds() - 0.5) <= 1e-6);
  // Sine at t=0 starts at zero.
  KIMIA_REQUIRE(std::abs(audio->samples[0]) <= 0.01f);
  // Peak amplitude 0.4.
  f32 peak = 0.0f;
  for (f32 sample : audio->samples) peak = std::max(peak, std::abs(sample));
  KIMIA_REQUIRE(peak > 0.35f && peak < 0.45f);
  // 16-bit WAV round-trip: same shape within quantization noise.
  KIMIA_REQUIRE(audio->writeWAV(tmpPath("tone.kimi.wav")));
  auto reloaded = kimia::AudioBuffer::load(tmpPath("tone.kimi.wav"), error);
  KIMIA_REQUIRE(reloaded.has_value());
  KIMIA_REQUIRE(reloaded->channels == 2 && reloaded->sampleRate == 44100);
  KIMIA_REQUIRE(reloaded->frameCount == 22050U);
  for (usize i = 0; i < reloaded->samples.size(); ++i) {
    KIMIA_REQUIRE(std::abs(reloaded->samples[i] - audio->samples[i]) <= 0.001f);
  }
}

KIMIA_TEST(audio_mp3_440hz_loads_with_signal) {
  std::string error;
  auto audio = kimia::AudioBuffer::load(kAssets + "440hz.mp3", error);
  KIMIA_REQUIRE(audio.has_value());
  KIMIA_REQUIRE(audio->sampleRate == 44100);
  KIMIA_REQUIRE(audio->channels > 0);
  KIMIA_REQUIRE(audio->frameCount > 0U);
  KIMIA_REQUIRE(audio->durationSeconds() > 0.5);
  // Real tone: the decoded signal must carry energy (a silent file would not).
  f64 energy = 0.0;
  const usize channelCount = static_cast<usize>(audio->channels);
  for (f32 sample : audio->samples) energy += static_cast<f64>(sample) * static_cast<f64>(sample);
  const f64 rms = std::sqrt(energy / static_cast<f64>(audio->samples.size() / channelCount));
  KIMIA_REQUIRE(rms > 0.05);
  f32 peak = 0.0f;
  for (f32 sample : audio->samples) peak = std::max(peak, std::abs(sample));
  KIMIA_REQUIRE(peak > 0.2f);
  // Downmix halves the sample count and keeps the same duration.
  const std::vector<f32> mono = audio->downmixMono();
  KIMIA_REQUIRE(mono.size() == static_cast<usize>(audio->frameCount));
}

KIMIA_TEST(audio_missing_file_returns_error) {
  std::string error;
  KIMIA_REQUIRE(!kimia::AudioBuffer::load(kAssets + "missing.wav", error).has_value());
  KIMIA_REQUIRE(!error.empty());
  KIMIA_REQUIRE(!kimia::AudioBuffer::load(kAssets + "missing.mp3", error).has_value());
}

// --- Pipeline dispatch ---

KIMIA_TEST(pipeline_detects_types_case_insensitive) {
  KIMIA_REQUIRE(kimia::assets::detectType("a.obj") == kimia::assets::AssetType::mesh);
  KIMIA_REQUIRE(kimia::assets::detectType("A.FBX") == kimia::assets::AssetType::mesh);
  KIMIA_REQUIRE(kimia::assets::detectType("x.PNG") == kimia::assets::AssetType::image);
  KIMIA_REQUIRE(kimia::assets::detectType("x.jpg") == kimia::assets::AssetType::image);
  KIMIA_REQUIRE(kimia::assets::detectType("x.jpeg") == kimia::assets::AssetType::image);
  KIMIA_REQUIRE(kimia::assets::detectType("x.wav") == kimia::assets::AssetType::audio);
  KIMIA_REQUIRE(kimia::assets::detectType("x.Mp3") == kimia::assets::AssetType::audio);
  KIMIA_REQUIRE(!kimia::assets::detectType("x.txt").has_value());
  KIMIA_REQUIRE(!kimia::assets::detectType("no_extension").has_value());
}

KIMIA_TEST(pipeline_loads_each_format) {
  std::string error;
  auto mesh = kimia::assets::loadMesh(kAssets + "cube.obj", error);
  KIMIA_REQUIRE(mesh.has_value() && mesh->sourceFormat == "obj");
  mesh = kimia::assets::loadMesh(kAssets + "blender_cube.fbx", error);
  KIMIA_REQUIRE(mesh.has_value() && mesh->sourceFormat == "fbx");
  KIMIA_REQUIRE(kimia::assets::loadImage(kAssets + "2x3.png", error).has_value());
  KIMIA_REQUIRE(kimia::assets::loadImage(kAssets + "2x2.jpg", error).has_value());
  KIMIA_REQUIRE(kimia::assets::loadAudio(kAssets + "tone.wav", error).has_value());
  KIMIA_REQUIRE(kimia::assets::loadAudio(kAssets + "440hz.mp3", error).has_value());
}

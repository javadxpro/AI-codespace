#pragma once

#include <kimia/Audio.h>
#include <kimia/GraphicsTypes.h>
#include <kimia/Image.h>
#include <kimia/Mesh.h>

#include <optional>
#include <string>
#include <vector>

namespace kimia {
namespace assets {

enum class AssetType { mesh, image, audio };

// Case-insensitive extension detection. Returns nullopt for unknown types.
std::optional<AssetType> detectType(const std::string& path);

// --- Meshes: .obj / .fbx ---
struct MeshLoadResult {
  MeshData mesh;
  std::string sourceFormat;  // "obj" | "fbx"
};

std::optional<MeshLoadResult> loadMesh(const std::string& path, std::string& error);

// Loads every mesh in the FBX scene (FBX files can contain several).
std::optional<std::vector<MeshData>> loadFBXAll(const std::string& path, std::string& error);

// --- Images: .png / .jpg / .jpeg ---
std::optional<Image> loadImage(const std::string& path, std::string& error);

// --- Audio: .wav / .mp3 ---
std::optional<AudioBuffer> loadAudio(const std::string& path, std::string& error);

}  // namespace assets
}  // namespace kimia

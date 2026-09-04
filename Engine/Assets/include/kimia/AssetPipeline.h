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
// `.mtl` is auxiliary (read through its OBJ); `.blend` is documented as an
// export path, not a loadable format.
std::optional<AssetType> detectType(const std::string& path);

// --- Meshes: .obj (+ .mtl) / .fbx ---
struct MeshLoadResult {
  MeshData mesh;
  std::string sourceFormat;  // "obj" | "fbx"
};

std::optional<MeshLoadResult> loadMesh(const std::string& path, std::string& error);

// Loads every mesh in the FBX scene (FBX files can contain several).
std::optional<std::vector<MeshData>> loadFBXAll(const std::string& path, std::string& error);

// A mesh together with its materials: the combined mesh, the material table
// (OBJ MTL / FBX materials) and one sub-mesh per material group. When an
// image is placed on an object, its MaterialData entry is what gets created
// or updated. Sub-meshes are empty when the file defines no materials.
struct MeshAsset {
  MeshData mesh;
  std::vector<MaterialData> materials;
  std::vector<MeshData> subMeshes;
};

// OBJ + MTL: parses `newmtl` / `Kd` / `map_Kd` (texture paths are resolved
// against the MTL file's directory) and splits `usemtl` runs into sub-meshes.
std::optional<MeshAsset> loadOBJAsset(const std::string& path, std::string& error);

// FBX: extracts material names, diffuse colors and diffuse textures. Embedded
// texture data is written next to the FBX file as `<base>_<material>.png`.
std::optional<MeshAsset> loadFBXAsset(const std::string& path, std::string& error);

// --- Images: .png / .jpg / .jpeg ---
std::optional<Image> loadImage(const std::string& path, std::string& error);

// --- Audio: .wav / .mp3 / .ogg (Vorbis) / .flac ---
std::optional<AudioBuffer> loadAudio(const std::string& path, std::string& error);

}  // namespace assets
}  // namespace kimia

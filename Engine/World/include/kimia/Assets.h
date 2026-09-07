#pragma once

#include <kimia/Types.h>

#include <string>
#include <vector>

namespace kimia {
namespace assetscan {

// --- Scanning a folder of files the user dropped in ---
//
// A person copies models, textures and sounds into a folder with their
// phone's file manager. The editor's job is to LOOK INSIDE them and say
// what is there — "this FBX has a skeleton and three clips: Idle, Run,
// Kick" — so they can be picked from a list instead of typed from memory.
//
// Reading an FBX is slow enough to matter on a phone, so a scan is
// something the user asks for, not something that happens every frame.

enum class AssetKind { Model, Texture, Sound, Unknown };

struct ScannedAsset {
  std::string file;   // name inside the folder, e.g. "hero.fbx"
  std::string path;   // the whole path, ready to hand to a loader
  AssetKind kind = AssetKind::Unknown;
  u64 bytes = 0U;

  // Models only, filled in by a deep scan.
  bool hasSkeleton = false;
  u32 boneCount = 0U;
  std::vector<std::string> clips;  // animation names inside the file
  std::string note;                // why a file could not be read
};

const char* assetKindName(AssetKind kind);
AssetKind kindOfFile(const std::string& file);

// Lists what is in `folder`. `deep` opens each model to find its
// skeleton and clips; without it the scan is a directory listing, which
// is instant even with a hundred files in there.
std::vector<ScannedAsset> scan(const std::string& folder, bool deep);

}  // namespace assetscan
}  // namespace kimia

#include <kimia/AssetPipeline.h>
#include <kimia/Assets.h>

#include <algorithm>
#include <filesystem>

namespace kimia {
namespace assetscan {

namespace {

std::string lowered(const std::string& text) {
  std::string out = text;
  for (char& c : out) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return out;
}

bool endsWith(const std::string& text, const std::string& tail) {
  return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
}

}  // namespace

const char* assetKindName(AssetKind kind) {
  switch (kind) {
    case AssetKind::Model: return "model";
    case AssetKind::Texture: return "texture";
    case AssetKind::Sound: return "sound";
    case AssetKind::Unknown: break;
  }
  return "unknown";
}

AssetKind kindOfFile(const std::string& file) {
  const std::string name = lowered(file);
  if (endsWith(name, ".fbx") || endsWith(name, ".obj")) return AssetKind::Model;
  if (endsWith(name, ".png") || endsWith(name, ".jpg") || endsWith(name, ".jpeg")) return AssetKind::Texture;
  if (endsWith(name, ".wav") || endsWith(name, ".mp3") || endsWith(name, ".ogg") ||
      endsWith(name, ".flac")) {
    return AssetKind::Sound;
  }
  return AssetKind::Unknown;
}

// Lists one folder into `found`, descending into subfolders: an animation
// pack arrives as actions/, dances/, goalkeeper/... and the editor must see
// all of it, not just the top level. `file` stays relative to the scanned
// root ("actions/Dribble.fbx"), `path` is the whole thing.
void scanInto(const std::string& root, const std::string& folder, std::vector<ScannedAsset>& found) {
  namespace fs = std::filesystem;
  std::error_code error;
  const fs::path rootPath = fs::weakly_canonical(fs::path(root), error);
  error.clear();
  const fs::path folderPath = fs::weakly_canonical(fs::path(folder), error);
  if (error) return;
  fs::directory_iterator iterator(folderPath, error);
  if (error) return;  // a missing or inaccessible folder is empty, not a crash

  const fs::directory_iterator end;
  for (; iterator != end; iterator.increment(error)) {
    if (error) {
      error.clear();
      continue;
    }
    const fs::directory_entry& entry = *iterator;
    std::error_code entryError;
    if (entry.is_directory(entryError)) {
      scanInto(root, entry.path().string(), found);
      continue;
    }
    if (entryError || !entry.is_regular_file(entryError)) continue;

    const std::string name = entry.path().filename().string();
    const AssetKind kind = kindOfFile(name);
    // Anything the engine cannot use is left out: showing a list full of
    // .txt and .zip would make the useful entries harder to find.
    if (kind == AssetKind::Unknown) continue;

    ScannedAsset asset;
    const fs::path wholePath = entry.path();
    std::error_code relativeError;
    fs::path relativePath = fs::relative(wholePath, rootPath, relativeError);
    if (relativeError || relativePath.empty()) relativePath = wholePath.filename();
    asset.file = relativePath.generic_string();
    asset.path = wholePath.generic_string();
    asset.kind = kind;
    std::error_code sizeError;
    asset.bytes = static_cast<u64>(entry.file_size(sizeError));
    if (sizeError) asset.bytes = 0U;
    found.push_back(asset);
  }
}

std::vector<ScannedAsset> scan(const std::string& folder, bool deep) {
  std::vector<ScannedAsset> found;
  scanInto(folder, folder, found);

  // Sorted so the list does not shuffle between scans: a person picking
  // the third item should get the same file next time.
  std::sort(found.begin(), found.end(),
            [](const ScannedAsset& a, const ScannedAsset& b) { return a.file < b.file; });

  if (!deep) return found;

  // The slow part: open each model and see what is inside. This is why a
  // deep scan is asked for rather than done on every listing.
  for (ScannedAsset& asset : found) {
    if (asset.kind != AssetKind::Model) continue;
    std::string error;
    auto skinned = assets::loadFBXSkinned(asset.path, error);
    if (skinned.has_value()) {
      asset.hasSkeleton = skinned->hasSkeleton();
      asset.boneCount = static_cast<u32>(skinned->skinned.skeleton.boneCount());
      for (const AnimationClip& clip : skinned->clips) {
        // A clip with no name cannot be picked from a list, so it is
        // given its position instead of being dropped.
        asset.clips.push_back(clip.name.empty() ? ("clip " + std::to_string(asset.clips.size() + 1U))
                                                : clip.name);
      }
      continue;
    }
    // No skeleton is the normal case for a prop, not a failure. Only say
    // something when the file cannot be read at all.
    std::string meshError;
    if (!assets::loadMesh(asset.path, meshError).has_value()) {
      asset.note = meshError.empty() ? std::string("cannot read this file") : meshError;
    }
  }
  return found;
}

}  // namespace assetscan
}  // namespace kimia

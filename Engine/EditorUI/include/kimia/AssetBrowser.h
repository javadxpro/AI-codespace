// AssetBrowser — the file panel that lists the contents of the
// project's assets / import directory.
//
// Phase 4+ placeholder: lists files of the configured types
// (.obj, .fbx, .png, .jpg, .kimia) the same way the WorldEditor
// already does. The full panel with thumbnails, drag-into-scene,
// and double-click-to-open lives in Phase 5.
#pragma once

#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

enum class AssetKind {
  Model,    // .obj / .fbx
  Image,    // .png / .jpg
  Scene,    // .kimia
  Other,
};

struct AssetEntry {
  std::string name;       // file basename
  std::string fullPath;   // absolute (or asset-relative) path
  AssetKind kind = AssetKind::Other;
  u64 sizeBytes = 0;
};

// Guess the AssetKind from a file extension.
AssetKind classifyFile(const std::string& name);

// List every importable asset in the given directory. Recurses one
// level deep (Phase 5+: full recursion).
std::vector<AssetEntry> scanAssets(const std::string& directory);

// Render the asset browser panel inside the given rect. The panel
// is scrollable; mouse-wheel / two-finger scroll handled by the
// Panel layer. Selection emits UiCommandKind::BeginDragEntity /
// DragEntityTo with the chosen file so the WorldEditor can place
// the model in the scene.
//
// Phase 4+: renders a list of file names, one per line.
void drawAssetBrowser(const Rect& rect,
                      const std::vector<AssetEntry>& assets,
                      i32 scrollY);

}  // namespace kimia::ui

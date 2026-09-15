#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct AssetMetadata {
  std::string name;
  std::string type;
  std::string path;
  u64 sizeBytes = 0;
  i64 modifiedSec = 0;
  i64 createdSec = 0;
  std::string author;
  std::string license;
  i32 refCount = 0;
};

void drawAssetMetadataPanel(const Rect& rect, const AssetMetadata& md);

}

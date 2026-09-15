#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct AssetFilterEntry {
  std::string label;
  std::string kind;        // "Model" / "Image" / "Scene"
  i32 count = 0;
  bool active = true;
};

void drawAssetFilterPanel(const Rect& rect,
                          const std::vector<AssetFilterEntry>& filters);

}

#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct ProjectAsset {
  std::string name;
  std::string path;
  std::string kind; // Model, Image, Scene, Material, Audio, Font
  u64 sizeBytes = 0;
};

void drawProjectBrowserPanel(const Rect& rect,
                             const std::string& folder,
                             const std::vector<ProjectAsset>& assets,
                             i32 selectedIndex,
                             i32 scrollY);

}

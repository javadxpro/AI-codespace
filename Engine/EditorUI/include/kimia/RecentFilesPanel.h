#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct RecentFile {
  std::string path;
  std::string thumbnail;   // small glyph hint
  f64 lastOpenedSec = 0.0f;
};

void drawRecentFilesPanel(const Rect& rect,
                          const std::vector<RecentFile>& files);

}

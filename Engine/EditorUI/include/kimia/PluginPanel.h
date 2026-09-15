#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct PluginEntry {
  std::string name;
  std::string version;
  std::string author;
  bool enabled = true;
  bool hasUpdate = false;
};

void drawPluginPanel(const Rect& rect,
                     const std::vector<PluginEntry>& plugins,
                     i32 scrollY);

}

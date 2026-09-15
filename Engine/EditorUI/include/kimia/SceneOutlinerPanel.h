#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct OutlinerEntry {
  std::string name;
  std::string type;
  bool isStatic = false;
  bool isHidden = false;
};

void drawSceneOutlinerPanel(const Rect& rect,
                            const std::vector<OutlinerEntry>& entries,
                            const std::string& filter,
                            i32 selectedIndex,
                            i32 scrollY);

}

#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct Tab {
  std::string title;
  std::string glyph;
  bool closable = false;
  bool dirty = false;
};

void drawTabBarPanel(const Rect& rect,
                     const std::vector<Tab>& tabs,
                     i32 activeTab);

}

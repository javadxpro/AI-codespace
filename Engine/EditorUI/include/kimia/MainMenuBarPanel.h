#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct MenuItem {
  std::string label;
  std::string shortcut;
  bool separator = false;
  bool disabled = false;
};

struct Menu {
  std::string name;
  std::vector<MenuItem> items;
  i32 selectedIndex = -1; // for highlighting currently-open menu
};

void drawMainMenuBarPanel(const Rect& rect, const std::vector<Menu>& menus);

}

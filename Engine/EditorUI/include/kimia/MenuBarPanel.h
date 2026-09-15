#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct MenuItem {
  std::string label;
  std::string shortcut;
  int id = 0;
  bool separator = false;
  bool disabled = false;
};

struct Menu {
  std::string title;
  std::vector<MenuItem> items;
};

void drawMenuBarPanel(const Rect& rect,
                      const std::vector<Menu>& menus,
                      int openMenuIndex);

}

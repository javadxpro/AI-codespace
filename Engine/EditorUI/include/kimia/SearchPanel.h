#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct SearchResult {
  std::string name;
  std::string path;
  std::string kind;
  i32 score = 0;
};

void drawSearchPanel(const Rect& rect,
                     const std::string& query,
                     const std::vector<SearchResult>& results,
                     i32 selectedIndex,
                     i32 scrollY);

}

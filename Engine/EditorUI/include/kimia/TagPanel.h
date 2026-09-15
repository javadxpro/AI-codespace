#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct Tag {
  std::string name;
  Color color{0.5f, 0.5f, 0.5f, 1.0f};
};

void drawTagPanel(const Rect& rect,
                  const std::vector<Tag>& tags,
                  const std::string& newTagInput);

}

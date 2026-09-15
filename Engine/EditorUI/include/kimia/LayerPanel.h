#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct LayerEntry {
  std::string name;
  bool visible = true;
  bool locked = false;
};

void drawLayerPanel(const Rect& rect,
                    const std::vector<LayerEntry>& layers);

}

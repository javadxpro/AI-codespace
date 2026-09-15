#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

enum class DockSide { Left, Right, Top, Bottom, Center };

struct DockTab {
  std::string title;
  std::string glyph;
};

struct DockArea {
  DockSide side = DockSide::Left;
  f32 size = 0.0f;          // width (Left/Right) or height (Top/Bottom)
  std::vector<DockTab> tabs;
  i32 activeTab = 0;
};

// Render the dock layout. The center rect is returned via outCenter
// so the caller can place the main Scene View there.
void drawDockLayoutPanel(const Rect& rect,
                         const std::vector<DockArea>& docks,
                         Rect& outCenter);

}

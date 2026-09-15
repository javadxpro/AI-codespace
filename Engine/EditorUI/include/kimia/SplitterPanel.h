#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class SplitterAxis { Horizontal, Vertical };

// Draw a draggable splitter bar. The bar is a 4-px-wide strip
// along the given axis. Phase 4+ renders the strip; Phase 5+
// wires the drag-to-resize into the dock layout.
void drawSplitterPanel(const Rect& rect, SplitterAxis axis);

}

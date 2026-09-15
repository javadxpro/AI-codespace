// TooltipPanel — a single-line tooltip that follows the pointer
// when it hovers over a widget. Phase 4+ exposes the rendering
// function; Phase 5+ will wire the actual hover detection in
// Widget.cpp to call this with a tooltip string.
#pragma once

#include "EditorUI.h"
#include <string>

namespace kimia::ui {

// Render a single-line tooltip at the given screen position.
// If `text` is empty, drawTooltipPanel is a no-op.
void drawTooltipPanel(f32 x, f32 y, const std::string& text);

}  // namespace kimia::ui

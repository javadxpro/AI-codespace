#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class ViewportMode { Perspective, Top, Front, Side };

void drawViewportPanel(const Rect& rect,
                       ViewportMode mode,
                       const std::string& title);

}

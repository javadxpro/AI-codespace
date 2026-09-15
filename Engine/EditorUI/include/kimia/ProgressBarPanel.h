#pragma once
#include "EditorUI.h"

namespace kimia::ui {

void drawProgressBarPanel(const Rect& rect,
                          const std::string& label,
                          float progress,
                          bool indeterminate);

}

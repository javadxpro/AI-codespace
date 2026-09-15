#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct ConsoleLine {
  std::string text;
  i32 severity = 0;  // 0=plain, 1=info, 2=warning, 3=error
};

void drawConsolePanel(const Rect& rect,
                      const std::vector<ConsoleLine>& lines,
                      const std::string& inputBuffer,
                      i32 scrollY,
                      bool autoScroll);

}

#pragma once
#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

struct ScriptLine {
  std::string text;
  bool highlight = false;  // syntax-highlighted line (Phase 5+ uses real parser)
};

void drawScriptEditorPanel(const Rect& rect,
                           const std::vector<ScriptLine>& lines,
                           i32 scrollY,
                           i32 cursorLine,
                           const std::string& statusMessage);

}

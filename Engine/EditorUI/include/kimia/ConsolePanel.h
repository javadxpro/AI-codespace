#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

enum class LogLevel { Info, Warn, Error };

struct LogLine {
  LogLevel level = LogLevel::Info;
  std::string text;
};

void drawConsolePanel(const Rect& rect,
                      const std::vector<LogLine>& lines,
                      const std::string& input,
                      i32 scrollY,
                      bool autoScroll);

}

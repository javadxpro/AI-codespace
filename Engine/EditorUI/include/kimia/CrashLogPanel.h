#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct CrashEntry {
  std::string timestamp;
  std::string thread;
  std::string message;
  std::string stackTrace;
};

void drawCrashLogPanel(const Rect& rect,
                       const std::vector<CrashEntry>& crashes,
                       i32 scrollY);

}

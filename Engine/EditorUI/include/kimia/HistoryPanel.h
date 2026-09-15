#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct HistoryEntry {
  std::string description;
  i64 timestamp = 0;
  bool undone = false;
};

void drawHistoryPanel(const Rect& rect,
                      const std::vector<HistoryEntry>& entries,
                      i32 scrollY);

}

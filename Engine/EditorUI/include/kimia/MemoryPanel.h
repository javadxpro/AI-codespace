#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct MemoryBucket {
  std::string name;
  u64 bytes = 0;
  u64 peakBytes = 0;
  i32 allocCount = 0;
};

void drawMemoryPanel(const Rect& rect,
                     const std::vector<MemoryBucket>& buckets);

}

#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct LocaleEntry {
  std::string code;        // "fa-IR"
  std::string displayName; // "فارسی (ایران)"
  i32 stringCount = 0;
  i32 translatedCount = 0;
  bool isCurrent = false;
};

void drawLocalizationPanel(const Rect& rect,
                           const std::vector<LocaleEntry>& locales,
                           i32 scrollY);

}

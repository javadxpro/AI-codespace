#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct AccountInfo {
  std::string username;
  std::string email;
  std::string avatarGlyph;
  i32 projectCount = 0;
  i32 publishedGameCount = 0;
  bool isOnline = true;
};

void drawAccountPanel(const Rect& rect, const AccountInfo& info);

}

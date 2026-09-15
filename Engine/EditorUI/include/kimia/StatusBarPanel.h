#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct StatusInfo {
  std::string sceneName;
  std::string version;
  f32 fps = 60.0f;
  u64 triCount = 0;
  std::string branch;
  std::string buildConfig;
};

void drawStatusBarPanel(const Rect& rect, const StatusInfo& info);

}

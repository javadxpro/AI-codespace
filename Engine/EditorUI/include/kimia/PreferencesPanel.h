#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct PrefsValues {
  bool autosave = true;
  bool autoReloadOnChange = true;
  bool showLineNumbers = true;
  bool enableVibration = true;
  bool enableSounds = true;
  f32 uiScale = 1.0f;
  i32 language = 0;       // 0=English,1=Persian,2=Japanese
  i32 renderBackend = 0;  // 0=GLES3,1=Vulkan,2=Software
};

void drawPreferencesPanel(const Rect& rect, const PrefsValues& p);

}

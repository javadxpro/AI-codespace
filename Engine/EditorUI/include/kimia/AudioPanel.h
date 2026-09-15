#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct AudioClipEntry {
  std::string name;
  f32 durationSec = 0.0f;
  bool muted = false;
  bool looped = false;
};

void drawAudioPanel(const Rect& rect,
                    const std::vector<AudioClipEntry>& clips);

}

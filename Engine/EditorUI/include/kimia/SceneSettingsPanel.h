#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct SceneSettingsProps {
  std::string sceneName = "untitled";
  f32 gravity = -9.81f;
  Vec3 ambientColor{0.2, 0.2, 0.2};
  f32 ambientIntensity = 0.3f;
  Vec3 fogColor{0.5, 0.5, 0.5};
  f32 fogStart = 50.0f;
  f32 fogEnd = 200.0f;
  bool fogEnabled = false;
  bool physicsEnabled = true;
  bool autoSave = true;
  i32 autoSaveIntervalSec = 60;
};

void drawSceneSettingsPanel(const Rect& rect, const SceneSettingsProps& props);

}

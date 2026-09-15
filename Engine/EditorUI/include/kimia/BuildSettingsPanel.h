#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class BuildPlatform { Android, Windows, Linux, Web };
enum class BuildConfig { Debug, Release, Profile };

struct BuildSettingsProps {
  BuildPlatform platform = BuildPlatform::Android;
  BuildConfig config = BuildConfig::Debug;
  bool embedAssets = true;
  bool arm64Only = true;
  bool compressAssets = true;
  bool stripDebugSymbols = false;
  std::string outputName = "kimia-app";
  std::string version = "0.30.0";
};

void drawBuildSettingsPanel(const Rect& rect, const BuildSettingsProps& props);

}

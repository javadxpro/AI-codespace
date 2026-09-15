#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct ProjectProps {
  std::string projectName = "My Game";
  std::string companyName = "Studio";
  std::string version = "0.30.0";
  std::string scenesDir = "scenes";
  std::string assetsDir = "assets";
  std::string profilesDir = "profiles";
  std::string buildDir = "build";
  bool useEditorUi = true;
  bool useNativeEditor = false;
};

void drawProjectPanel(const Rect& rect, const ProjectProps& props);

}

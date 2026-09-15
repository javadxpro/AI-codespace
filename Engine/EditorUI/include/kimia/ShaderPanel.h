#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class ShaderType { Vertex, Fragment, Compute };

struct ShaderEntry {
  std::string name;
  ShaderType type = ShaderType::Fragment;
  i32 lineCount = 0;
  bool compiled = false;
  f32 compileMs = 0.0f;
  std::string error;
};

void drawShaderPanel(const Rect& rect,
                     const std::vector<ShaderEntry>& shaders,
                     i32 scrollY);

}

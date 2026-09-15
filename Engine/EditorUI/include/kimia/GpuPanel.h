#pragma once
#include "EditorUI.h"

namespace kimia::ui {

struct GpuInfo {
  std::string vendor;
  std::string renderer;
  std::string version;
  i32 maxTextureSize = 4096;
  i32 maxVertexAttribs = 16;
  i32 maxUniformVectors = 1024;
  bool supportsCompute = true;
  bool supportsGeometry = true;
};

void drawGpuPanel(const Rect& rect, const GpuInfo& info);

}

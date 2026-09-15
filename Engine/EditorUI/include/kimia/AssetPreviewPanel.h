#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class AssetPreviewKind {
  None,
  Model3D,
  Image2D,
  Text,
  Unknown,
};

void drawAssetPreviewPanel(const Rect& rect,
                           AssetPreviewKind kind,
                           const std::string& name);

}

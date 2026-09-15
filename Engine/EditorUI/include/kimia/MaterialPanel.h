#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class MaterialBlend { Opaque, Alpha, Additive };

struct MaterialProps {
  std::string name;
  Vec3 albedo{0.7f, 0.7f, 0.7f};
  f32 roughness = 0.5f;
  f32 metalness = 0.0f;
  Vec3 emissive{0.0f, 0.0f, 0.0f};
  f32 opacity = 1.0f;
  std::string albedoTexture;
  std::string normalTexture;
  MaterialBlend blend = MaterialBlend::Opaque;
  bool doubleSided = false;
};

void drawMaterialPanel(const Rect& rect, const MaterialProps& props);

}

#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class AntialiasMode { None, FXAA, MSAA2, MSAA4, MSAA8 };
enum class ShadowQuality { Off, Low, Medium, High };
enum class TextureQuality { Low, Medium, High };

struct RenderSettingsProps {
  AntialiasMode antialias = AntialiasMode::FXAA;
  ShadowQuality shadows = ShadowQuality::Medium;
  TextureQuality textures = TextureQuality::High;
  bool vsync = true;
  f32 resolutionScale = 1.0f;
  i32 maxFps = 60;
  bool hdr = true;
  bool softParticles = false;
};

void drawRenderSettingsPanel(const Rect& rect, const RenderSettingsProps& props);

}

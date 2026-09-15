#include <kimia/RenderSettingsPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
const char* aaName(AntialiasMode m) {
  switch (m) {
    case AntialiasMode::None:  return "None";
    case AntialiasMode::FXAA:  return "FXAA";
    case AntialiasMode::MSAA2: return "MSAA 2x";
    case AntialiasMode::MSAA4: return "MSAA 4x";
    case AntialiasMode::MSAA8: return "MSAA 8x";
  }
  return "?";
}
const char* shadowName(ShadowQuality s) {
  switch (s) {
    case ShadowQuality::Off:     return "Off";
    case ShadowQuality::Low:     return "Low";
    case ShadowQuality::Medium:  return "Medium";
    case ShadowQuality::High:    return "High";
  }
  return "?";
}
const char* texName(TextureQuality t) {
  switch (t) {
    case TextureQuality::Low:    return "Low";
    case TextureQuality::Medium: return "Medium";
    case TextureQuality::High:   return "High";
  }
  return "?";
}
void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 80.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
}
}

void drawRenderSettingsPanel(const Rect& rect, const RenderSettingsProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Render", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[32];

  drawRow({rect.x, y, rect.w, rowH}, "Antialias", aaName(props.antialias));
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Shadows",   shadowName(props.shadows));
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Textures",  texName(props.textures));
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "VSync",
          props.vsync ? "on" : "off");
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.0f%%",
                static_cast<double>(props.resolutionScale * 100.0f));
  drawRow({rect.x, y, rect.w, rowH}, "Res scale", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d", props.maxFps);
  drawRow({rect.x, y, rect.w, rowH}, "Max FPS", buf);
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "HDR",
          props.hdr ? "on" : "off");
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Soft part.",
          props.softParticles ? "on" : "off");
}

}

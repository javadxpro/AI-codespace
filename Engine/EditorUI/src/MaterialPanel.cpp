#include <kimia/MaterialPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 70.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
}
const char* blendName(MaterialBlend b) {
  switch (b) {
    case MaterialBlend::Opaque:   return "Opaque";
    case MaterialBlend::Alpha:    return "Alpha";
    case MaterialBlend::Additive: return "Additive";
  }
  return "?";
}
}

void drawMaterialPanel(const Rect& rect, const MaterialProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText(props.name.empty() ? "Material" : props.name.c_str(),
           rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 26.0f;
  char buf[64];

  // Albedo swatch + hex.
  drawRect({rect.x + 4.0f, y + 4.0f, 12.0f, 12.0f},
           {static_cast<float>(props.albedo.x),
            static_cast<float>(props.albedo.y),
            static_cast<float>(props.albedo.z), 1.0f}, 2.0f);
  std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
                (int)(props.albedo.x*255.0f),
                (int)(props.albedo.y*255.0f),
                (int)(props.albedo.z*255.0f));
  drawText(buf, rect.x + 22.0f, y + 4.0f, 1, kText);
  y += rowH;

  std::snprintf(buf, sizeof(buf), "%.2f", props.roughness);
  drawRow({rect.x, y, rect.w, rowH}, "Roughness", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.metalness);
  drawRow({rect.x, y, rect.w, rowH}, "Metalness", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.1f, %.1f, %.1f",
                (double)props.emissive.x,
                (double)props.emissive.y,
                (double)props.emissive.z);
  drawRow({rect.x, y, rect.w, rowH}, "Emissive", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.opacity);
  drawRow({rect.x, y, rect.w, rowH}, "Opacity", buf);
  y += rowH;

  drawRow({rect.x, y, rect.w, rowH}, "Albedo tex",
          props.albedoTexture.empty() ? "(none)" : props.albedoTexture.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Normal tex",
          props.normalTexture.empty() ? "(none)" : props.normalTexture.c_str());
  y += rowH;

  std::snprintf(buf, sizeof(buf), "%s%s",
                blendName(props.blend),
                props.doubleSided ? " · 2-sided" : "");
  drawRow({rect.x, y, rect.w, rowH}, "Blend", buf);
}

}

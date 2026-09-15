#include <kimia/GpuPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
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

void drawGpuPanel(const Rect& rect, const GpuInfo& info) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("GPU", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[32];

  drawRow({rect.x, y, rect.w, rowH}, "Vendor",  info.vendor.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Renderer", info.renderer.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Version", info.version.c_str());
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d", info.maxTextureSize);
  drawRow({rect.x, y, rect.w, rowH}, "Max tex", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d", info.maxVertexAttribs);
  drawRow({rect.x, y, rect.w, rowH}, "Max attribs", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d", info.maxUniformVectors);
  drawRow({rect.x, y, rect.w, rowH}, "Max uniforms", buf);
  y += rowH + 2.0f;

  bool c = info.supportsCompute;
  if (checkbox("Compute shaders", c,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  bool g = info.supportsGeometry;
  if (checkbox("Geometry shaders", g,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
}

}

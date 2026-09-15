#include <kimia/TerrainPanel.h>
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

void drawTerrainPanel(const Rect& rect, const TerrainProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Terrain", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[64];

  std::snprintf(buf, sizeof(buf), "%d × %d",
                props.width, props.depth);
  drawRow({rect.x, y, rect.w, rowH}, "Resolution", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.spacing);
  drawRow({rect.x, y, rect.w, rowH}, "Spacing", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.heightScale);
  drawRow({rect.x, y, rect.w, rowH}, "Height scale", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.3f", props.noiseFrequency);
  drawRow({rect.x, y, rect.w, rowH}, "Noise freq", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d", props.seed);
  drawRow({rect.x, y, rect.w, rowH}, "Seed", buf);
  y += rowH + 2.0f;

  // Color swatch.
  drawRect({rect.x + 4.0f, y + 4.0f, 12.0f, 12.0f},
           {static_cast<float>(props.baseColor.x),
            static_cast<float>(props.baseColor.y),
            static_cast<float>(props.baseColor.z), 1.0f}, 2.0f);
  std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
                (int)(props.baseColor.x*255.0f),
                (int)(props.baseColor.y*255.0f),
                (int)(props.baseColor.z*255.0f));
  drawText(buf, rect.x + 22.0f, y + 4.0f, 1, kText);
  y += rowH;

  bool wire = props.wireframe;
  if (checkbox("Wireframe", wire, {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
}

}

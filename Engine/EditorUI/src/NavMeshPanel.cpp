#include <kimia/NavMeshPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 90.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
}
}

void drawNavMeshPanel(const Rect& rect, const NavMeshProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Nav Mesh", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[64];

  std::snprintf(buf, sizeof(buf), "%.2f", props.cellSize);
  drawRow({rect.x, y, rect.w, rowH}, "Cell size", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.cellHeight);
  drawRow({rect.x, y, rect.w, rowH}, "Cell height", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.agentHeight);
  drawRow({rect.x, y, rect.w, rowH}, "Agent height", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.agentRadius);
  drawRow({rect.x, y, rect.w, rowH}, "Agent radius", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.climbHeight);
  drawRow({rect.x, y, rect.w, rowH}, "Climb height", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.1f", props.slopeDeg);
  drawRow({rect.x, y, rect.w, rowH}, "Max slope", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d", props.tileSize);
  drawRow({rect.x, y, rect.w, rowH}, "Tile size", buf);
  y += rowH + 4.0f;

  if (props.built) {
    drawText("Built", rect.x + 4.0f, y, 1, kSuccess);
    std::snprintf(buf, sizeof(buf), "%d polys, %d verts",
                  props.polyCount, props.vertCount);
    drawText(buf, rect.x + 50.0f, y, 1, kTextMuted);
  } else {
    drawText("Not built", rect.x + 4.0f, y, 1, kWarning);
  }
}

}

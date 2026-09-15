#include <kimia/SnapPanel.h>
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

void drawSnapPanel(const Rect& rect, const SnapProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Snap", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[32];

  bool e = props.snapEnabled;
  if (checkbox("Snap enabled", e,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.positionStep);
  drawRow({rect.x, y, rect.w, rowH}, "Pos step", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.1f deg", props.rotationStepDeg);
  drawRow({rect.x, y, rect.w, rowH}, "Rot step", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.scaleStep);
  drawRow({rect.x, y, rect.w, rowH}, "Scale step", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.gridSize);
  drawRow({rect.x, y, rect.w, rowH}, "Grid size", buf);
  y += rowH + 2.0f;

  bool g = props.snapToGrid;
  if (checkbox("Snap to grid", g,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  bool s = props.snapToSurface;
  if (checkbox("Snap to surface", s,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  bool a = props.snapToAngle;
  if (checkbox("Snap to angle", a,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
}

}

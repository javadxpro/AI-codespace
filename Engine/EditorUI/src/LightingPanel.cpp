#include <kimia/LightingPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
const char* kindName(LightKind k) {
  switch (k) {
    case LightKind::Directional: return "Directional";
    case LightKind::Point:       return "Point";
    case LightKind::Spot:        return "Spot";
  }
  return "?";
}
void formatVec3(char* b, usize n, const Vec3& v, int d=2) {
  std::snprintf(b, n, "%.*f, %.*f, %.*f",
                d, (double)v.x, d, (double)v.y, d, (double)v.z);
}
void formatFloat(char* b, usize n, f32 v, int d=2) {
  std::snprintf(b, n, "%.*f", d, (double)v);
}
void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 70.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
}
}

void drawLightingPanel(const Rect& rect, const LightProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText(kindName(props.kind),
           rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  constexpr f32 btnW = 90.0f;
  constexpr f32 btnH = 18.0f;
  f32 y = rect.y + 26.0f;

  // Kind picker.
  if (button("Directional", {rect.x + 4.0f,                     y, btnW, btnH})) {}
  if (button("Point",       {rect.x + 4.0f + (btnW + 4.0f),     y, btnW, btnH})) {}
  if (button("Spot",        {rect.x + 4.0f + 2.0f * (btnW + 4.0f), y, btnW, btnH})) {}
  y += rowH + 2.0f;

  char buf[64];

  if (props.kind != LightKind::Point) {
    formatVec3(buf, sizeof(buf), props.direction);
    drawRow({rect.x, y, rect.w, rowH}, "Direction", buf);
    y += rowH;
  }
  if (props.kind != LightKind::Directional) {
    formatVec3(buf, sizeof(buf), props.position);
    drawRow({rect.x, y, rect.w, rowH}, "Position", buf);
    y += rowH;
  }

  formatVec3(buf, sizeof(buf), props.color);
  drawRow({rect.x, y, rect.w, rowH}, "Color", buf);
  y += rowH;
  formatFloat(buf, sizeof(buf), props.intensity);
  drawRow({rect.x, y, rect.w, rowH}, "Intensity", buf);
  y += rowH;

  if (props.kind != LightKind::Directional) {
    formatFloat(buf, sizeof(buf), props.range);
    drawRow({rect.x, y, rect.w, rowH}, "Range", buf);
    y += rowH;
  }
  if (props.kind == LightKind::Spot) {
    formatFloat(buf, sizeof(buf), props.spotAngleDeg);
    drawRow({rect.x, y, rect.w, rowH}, "Cone (deg)", buf);
    y += rowH;
  }

  bool shadows = props.castsShadows;
  if (checkbox("Shadows", shadows,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {
  }
}

}

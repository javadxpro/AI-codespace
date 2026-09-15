#include <kimia/ViewportPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
const char* shadingName(ViewportShading s) {
  switch (s) {
    case ViewportShading::Lit:       return "Lit";
    case ViewportShading::Unlit:     return "Unlit";
    case ViewportShading::Wireframe: return "Wire";
    case ViewportShading::Albedo:    return "Albedo";
    case ViewportShading::Normals:   return "Normal";
  }
  return "?";
}
const char* gizmoName(GizmoMode g) {
  switch (g) {
    case GizmoMode::None:   return "Select";
    case GizmoMode::Move:   return "Move";
    case GizmoMode::Rotate: return "Rotate";
    case GizmoMode::Scale:  return "Scale";
  }
  return "?";
}
void drawGrid(const Rect& v, f32 gridSize, f32 zoom) {
  using namespace theme;
  if (gridSize <= 0.0f) return;
  // We just draw relative to the viewport center.
  const f32 step = gridSize * zoom;
  if (step < 4.0f) return;
  const f32 cx = v.x + v.w * 0.5f;
  const f32 cy = v.y + v.h * 0.5f;

  // Vertical lines.
  f32 x = cx;
  while (x < v.x + v.w) {
    drawRect({x, v.y, 1.0f, v.h}, kPanelAlt, 0.0f);
    x += step;
  }
  x = cx - step;
  while (x > v.x) {
    drawRect({x, v.y, 1.0f, v.h}, kPanelAlt, 0.0f);
    x -= step;
  }
  // Horizontal lines.
  f32 y = cy;
  while (y < v.y + v.h) {
    drawRect({v.x, y, v.w, 1.0f}, kPanelAlt, 0.0f);
    y += step;
  }
  y = cy - step;
  while (y > v.y) {
    drawRect({v.x, y, v.w, 1.0f}, kPanelAlt, 0.0f);
    y -= step;
  }
  // Center axes.
  drawRect({cx, v.y, 1.0f, v.h}, kAccent, 0.0f);
  drawRect({v.x, cy, v.w, 1.0f}, kAccent, 0.0f);
}
void drawGizmo(const Rect& v, GizmoMode g) {
  using namespace theme;
  if (g == GizmoMode::None) return;
  const f32 cx = v.x + v.w * 0.5f;
  const f32 cy = v.y + v.h * 0.5f;
  constexpr f32 len = 30.0f;
  // X axis (red-ish accent hot)
  drawRect({cx, cy, len, 2.0f}, kAccentHot, 0.0f);
  // Y axis
  drawRect({cx - 2.0f, cy - len, 2.0f, len}, kSuccess, 0.0f);
  // Z axis (depth approximation)
  drawRect({cx - len * 0.5f, cy + 4.0f, len, 2.0f},
           Color{0.3f, 0.6f, 1.0f, 1.0f}, 0.0f);

  if (g == GizmoMode::Move) {
    // Markers.
    drawRect({cx + len - 4.0f, cy - 4.0f, 8.0f, 8.0f},
             kAccentHot, 0.0f);
    drawRect({cx - 4.0f, cy - len - 4.0f, 8.0f, 8.0f},
             kSuccess, 0.0f);
  } else if (g == GizmoMode::Rotate) {
    // Arc approximation via 3 short rects.
    drawRect({cx + len - 4.0f, cy - 4.0f, 8.0f, 8.0f},
             kAccentHot, 2.0f);
  } else if (g == GizmoMode::Scale) {
    drawRect({cx + len - 6.0f, cy - 2.0f, 4.0f, 4.0f},
             kAccentHot, 0.0f);
    drawRect({cx + len + 2.0f, cy - 2.0f, 4.0f, 4.0f},
             kAccentHot, 0.0f);
  }
}
}

void drawViewportPanel(const Rect& rect,
                       GizmoMode gizmo,
                       ViewportShading shading,
                       bool playing,
                       const std::string& cameraName,
                       f32 gridSize,
                       bool showGrid,
                       bool showStats,
                       f32 zoom) {
  using namespace theme;
  drawRect(rect, Color{0.08f, 0.08f, 0.10f, 1.0f}, 0.0f);
  drawRect({rect.x, rect.y, rect.w, 1.0f}, kPanelAlt, 0.0f);
  drawRect({rect.x, rect.y + rect.h - 1.0f, rect.w, 1.0f},
           kPanelAlt, 0.0f);
  drawRect({rect.x, rect.y, 1.0f, rect.h}, kPanelAlt, 0.0f);
  drawRect({rect.x + rect.w - 1.0f, rect.y, 1.0f, rect.h},
           kPanelAlt, 0.0f);

  if (showGrid) drawGrid(rect, gridSize, zoom);
  drawGizmo(rect, gizmo);

  // Top toolbar.
  constexpr f32 tbH = 22.0f;
  drawRect({rect.x + 1.0f, rect.y + 1.0f,
            rect.w - 2.0f, tbH},
           kPanelAlt, 0.0f);
  drawText(gizmoName(gizmo),
           rect.x + 6.0f, rect.y + 6.0f, 1, kAccentHot);
  drawText(shadingName(shading),
           rect.x + 80.0f, rect.y + 6.0f, 1, kText);
  if (!cameraName.empty()) {
    drawText(cameraName.c_str(),
             rect.x + rect.w - 110.0f, rect.y + 6.0f, 1, kTextMuted);
  }
  if (playing) {
    drawRect({rect.x + rect.w - 28.0f, rect.y + 6.0f, 6.0f, 6.0f},
             kError, 0.0f);
    drawText("PLAY", rect.x + rect.w - 20.0f,
             rect.y + 6.0f, 1, kError);
  }

  if (showStats) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.0fx", static_cast<double>(zoom));
    drawText(buf, rect.x + 6.0f, rect.y + rect.h - 14.0f, 1, kTextMuted);
  }
}

}

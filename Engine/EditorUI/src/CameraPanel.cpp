// CameraPanel implementation — see CameraPanel.h.
#include <kimia/CameraPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <cstdio>

namespace kimia::ui {

namespace {

void formatVec3(char* buf, usize n, const Vec3& v, i32 decimals = 1) {
  std::snprintf(buf, n, "%.*f, %.*f, %.*f",
                decimals, static_cast<double>(v.x),
                decimals, static_cast<double>(v.y),
                decimals, static_cast<double>(v.z));
}

void formatFloat(char* buf, usize n, f32 v, i32 decimals = 2) {
  std::snprintf(buf, n, "%.*f", decimals, static_cast<double>(v));
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

}  // namespace

void drawCameraPanel(const Rect& rect, const CameraProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText("Camera", rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  constexpr f32 btnW = 90.0f;
  constexpr f32 btnH = 18.0f;
  const f32 padX = 4.0f;
  f32 y = rect.y + 26.0f;

  // Projection: Persp / Ortho buttons.
  if (button("Perspective",
             {rect.x + padX, y, btnW, btnH})) {
    // Phase 5+ will toggle via UiCommand.
  }
  if (button("Orthographic",
             {rect.x + padX + btnW + 4.0f, y, btnW, btnH})) {
  }
  y += rowH + 2.0f;

  char buf[64];

  // FOV / ortho size.
  if (props.projection == CameraProjection::Perspective) {
    formatFloat(buf, sizeof(buf), props.fovYDeg, 1);
    drawRow({rect.x, y, rect.w, rowH}, "FOV (deg)", buf);
  } else {
    formatFloat(buf, sizeof(buf), props.orthoSize, 2);
    drawRow({rect.x, y, rect.w, rowH}, "Ortho size", buf);
  }
  y += rowH;

  // Near / Far.
  formatFloat(buf, sizeof(buf), props.nearPlane, 3);
  drawRow({rect.x, y, rect.w, rowH}, "Near", buf);
  y += rowH;
  formatFloat(buf, sizeof(buf), props.farPlane, 1);
  drawRow({rect.x, y, rect.w, rowH}, "Far", buf);
  y += rowH + 2.0f;

  // Eye / Target / Up.
  formatVec3(buf, sizeof(buf), props.eye);
  drawRow({rect.x, y, rect.w, rowH}, "Eye", buf);
  y += rowH;
  formatVec3(buf, sizeof(buf), props.target);
  drawRow({rect.x, y, rect.w, rowH}, "Target", buf);
  y += rowH;
  formatVec3(buf, sizeof(buf), props.up);
  drawRow({rect.x, y, rect.w, rowH}, "Up", buf);
}

}  // namespace kimia::ui

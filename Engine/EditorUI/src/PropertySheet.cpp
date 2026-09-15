// PropertySheet implementation — see PropertySheet.h.
#include <kimia/PropertySheet.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <cstdio>

namespace kimia::ui {

namespace {

void formatVec3(char* buf, usize n, const Vec3& v, i32 decimals = 2) {
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

void colorToChar(char* buf, usize n, const Vec3& v) {
  std::snprintf(buf, n, "#%02X%02X%02X",
                static_cast<int>(v.x * 255.0f),
                static_cast<int>(v.y * 255.0f),
                static_cast<int>(v.z * 255.0f));
}

}  // namespace

void drawPropertySheet(const Rect& rect, const EntityProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Title row: entity name (truncated by drawText).
  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText(props.name.c_str(), rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 26.0f;
  char buf[64];

  // Position
  formatVec3(buf, sizeof(buf), props.position, 2);
  drawRow({rect.x, y, rect.w, rowH}, "Position", buf);
  y += rowH;
  // Rotation
  formatVec3(buf, sizeof(buf), props.rotationEuler, 1);
  drawRow({rect.x, y, rect.w, rowH}, "Rotation", buf);
  y += rowH;
  // Scale
  formatVec3(buf, sizeof(buf), props.scale, 2);
  drawRow({rect.x, y, rect.w, rowH}, "Scale", buf);
  y += rowH;
  // Colour as a swatch + hex string.
  drawRect({rect.x + 4.0f, y + 4.0f, 12.0f, 12.0f},
           {static_cast<float>(props.color.x),
            static_cast<float>(props.color.y),
            static_cast<float>(props.color.z),
            1.0f}, 2.0f);
  colorToChar(buf, sizeof(buf), props.color);
  drawText(buf, rect.x + 22.0f, y + 4.0f, 1, kText);
  y += rowH;
  // Roughness
  formatFloat(buf, sizeof(buf), props.roughness);
  drawRow({rect.x, y, rect.w, rowH}, "Roughness", buf);
  y += rowH;
  // Metalness
  formatFloat(buf, sizeof(buf), props.metalness);
  drawRow({rect.x, y, rect.w, rowH}, "Metalness", buf);
  y += rowH + 4.0f;

  // Locked checkbox — copy the bool so we can pass it as non-const
  // reference (Phase 4+ is read-only so we ignore the toggle).
  bool locked = props.locked;
  if (checkbox("Locked", locked,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {
    // checkbox flips in place — Phase 5+ will route this through
    // a UiCommand so the WorldEditor + UndoStack see the change.
  }
  y += rowH;

  // Role hint at the bottom: "Player" / "Ball" / empty.
  const char* role = "";
  if (props.isPlayer) role = "Player";
  else if (props.isBall) role = "Ball";
  if (role[0] != '\0') {
    drawText(role,
             rect.x + 4.0f, rect.y + rect.h - 14.0f,
             1, kAccent);
  }
}

}  // namespace kimia::ui

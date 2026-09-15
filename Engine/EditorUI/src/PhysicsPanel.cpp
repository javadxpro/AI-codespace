// PhysicsPanel implementation — see PhysicsPanel.h.
#include <kimia/PhysicsPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <algorithm>
#include <cstdio>

namespace kimia::ui {

namespace {

const char* shapeName(ColliderShape s) {
  switch (s) {
    case ColliderShape::Box:     return "Box";
    case ColliderShape::Sphere:  return "Sphere";
    case ColliderShape::Plane:   return "Plane";
    case ColliderShape::Capsule: return "Capsule";
  }
  return "?";
}

void drawRow(const Rect& row, const char* label, char* valueBuf,
             usize valueBufSize) {
  using namespace theme;
  // Two-column row: label on the left, editable value on the right.
  constexpr f32 labelW = 70.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(valueBuf, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
  // We don't actually wire the click-to-edit here — Phase 5 will
  // open an inline input. For now the field is just a label so the
  // user can see the current value.
  (void)valueBuf;
  (void)valueBufSize;
}

void formatFloat(char* buf, usize n, f32 v) {
  std::snprintf(buf, n, "%.2f", static_cast<double>(v));
}

void formatVec3(char* buf, usize n, const Vec3& v) {
  std::snprintf(buf, n, "%.1f, %.1f, %.1f",
                static_cast<double>(v.x), static_cast<double>(v.y),
                static_cast<double>(v.z));
}

}  // namespace

bool drawPhysicsPanel(const Rect& rect, const PhysicsProps& current,
                      PhysicsProps& edited) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Physics", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;

  // Shape selector — three small buttons in a row.
  const f32 btnW = 56.0f;
  const f32 btnH = 18.0f;
  const f32 padX = 4.0f;
  if (button("Box",     {rect.x + padX + 0.0f * (btnW + 4.0f), y, btnW, btnH})) {
    edited.shape = ColliderShape::Box;
  }
  if (button("Sphere",  {rect.x + padX + 1.0f * (btnW + 4.0f), y, btnW, btnH})) {
    edited.shape = ColliderShape::Sphere;
  }
  if (button("Plane",   {rect.x + padX + 2.0f * (btnW + 4.0f), y, btnW, btnH})) {
    edited.shape = ColliderShape::Plane;
  }
  y += rowH + 2.0f;

  char buf[64];

  // Mass row (0 = static)
  std::snprintf(buf, sizeof(buf), "%s",
                current.mass <= 0.0f ? "static" : "%.2f kg");
  if (current.mass > 0.0f) formatFloat(buf, sizeof(buf), current.mass);
  drawRow({rect.x, y, rect.w, rowH}, "Mass", buf, sizeof(buf));
  y += rowH;

  // Friction
  formatFloat(buf, sizeof(buf), current.friction);
  drawRow({rect.x, y, rect.w, rowH}, "Friction", buf, sizeof(buf));
  y += rowH;

  // Restitution
  formatFloat(buf, sizeof(buf), current.restitution);
  drawRow({rect.x, y, rect.w, rowH}, "Bounce", buf, sizeof(buf));
  y += rowH;

  // Extents
  formatVec3(buf, sizeof(buf), current.extents);
  drawRow({rect.x, y, rect.w, rowH}, "Extents", buf, sizeof(buf));
  y += rowH;

  // Static / Dynamic / Kinematic — three radio-like buttons.
  if (button("Static",   {rect.x + padX + 0.0f * (btnW + 4.0f), y, btnW, btnH})) {
    edited.mass = 0.0f;
    edited.isKinematic = false;
  }
  if (button("Dynamic",  {rect.x + padX + 1.0f * (btnW + 4.0f), y, btnW, btnH})) {
    if (edited.mass <= 0.0f) edited.mass = 1.0f;
    edited.isKinematic = false;
  }
  if (button("Kinematic",{rect.x + padX + 2.0f * (btnW + 4.0f), y, btnW, btnH})) {
    edited.isKinematic = true;
  }
  y += rowH + 2.0f;

  // Trigger checkbox.
  if (checkbox("Trigger volume", edited.isTrigger,
               {rect.x + padX, y, rect.w - 2.0f * padX, rowH})) {
    // The checkbox helper flips the value in-place; we don't need to
    // re-toggle here. Just a placeholder so the syntax is happy.
  }
  y += rowH;

  // The shape name as a summary at the bottom.
  drawText(shapeName(current.shape),
           rect.x + 4.0f, rect.y + rect.h - 14.0f, 1, kTextDim);

  // For Phase 4+ the panel is read-only (it draws current props, not
  // an editing buffer). Return false so the caller doesn't push an
  // empty UiCommand onto the undo stack. Phase 5 will wire the
  // numeric sliders / inline inputs and return true when something
  // actually changed.
  (void)edited;
  return false;
}

}  // namespace kimia::ui

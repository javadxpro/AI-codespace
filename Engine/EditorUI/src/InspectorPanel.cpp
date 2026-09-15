#include <kimia/InspectorPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void drawField(const Rect& rect, const char* label,
               const std::string& value, const Color& tint) {
  using namespace theme;
  constexpr f32 labelW = 70.0f;
  drawText(label, rect.x + 4.0f, rect.y + 4.0f, 1,
           tint.r > 0.0f ? tint : kText);
  drawRect({rect.x + labelW, rect.y + 1.0f,
            rect.w - labelW - 4.0f, rect.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value.c_str(),
           rect.x + labelW + 4.0f, rect.y + 4.0f, 1, kText);
}
std::string formatValue(const InspectorProp& p) {
  char buf[64];
  switch (p.kind) {
    case InspectorKind::Float:  std::snprintf(buf, sizeof(buf), "%.3f",
                                             static_cast<double>(p.v0));
                                return buf;
    case InspectorKind::Int:    std::snprintf(buf, sizeof(buf), "%d",
                                             static_cast<int>(p.v0));
                                return buf;
    case InspectorKind::Bool:   return p.v0 != 0.0f ? "true" : "false";
    case InspectorKind::String: return p.s.empty() ? "(empty)" : p.s;
    case InspectorKind::Vec2:   std::snprintf(buf, sizeof(buf),
                                             "(%.2f, %.2f)",
                                             static_cast<double>(p.v0),
                                             static_cast<double>(p.v1));
                                return buf;
    case InspectorKind::Vec3:   std::snprintf(buf, sizeof(buf),
                                             "(%.2f, %.2f, %.2f)",
                                             static_cast<double>(p.v0),
                                             static_cast<double>(p.v1),
                                             static_cast<double>(p.v2));
                                return buf;
    case InspectorKind::Vec4:   std::snprintf(buf, sizeof(buf),
                                             "(%.2f, %.2f, %.2f, %.2f)",
                                             static_cast<double>(p.v0),
                                             static_cast<double>(p.v1),
                                             static_cast<double>(p.v2),
                                             static_cast<double>(p.v3));
                                return buf;
    case InspectorKind::Color:  std::snprintf(buf, sizeof(buf),
                                             "#%02X%02X%02X %02X",
                                             static_cast<int>(p.v0 * 255),
                                             static_cast<int>(p.v1 * 255),
                                             static_cast<int>(p.v2 * 255),
                                             static_cast<int>(p.v3 * 255));
                                return buf;
  }
  return "?";
}
}

void drawInspectorPanel(const Rect& rect,
                        const std::string& objectName,
                        const std::string& objectType,
                        const std::vector<InspectorProp>& props,
                        i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Header.
  drawRect({rect.x, rect.y, rect.w, 26.0f},
           kPanelAlt, 0.0f);
  drawRect({rect.x, rect.y + 25.0f, rect.w, 1.0f},
           kAccent, 0.0f);
  drawText(objectName.c_str(),
           rect.x + 6.0f, rect.y + 6.0f, 1, kAccentHot);
  drawText(objectType.c_str(),
           rect.x + rect.w - 80.0f, rect.y + 6.0f, 1, kTextMuted);

  pushClip(rect);
  constexpr f32 rowH = 20.0f;
  f32 y = rect.y + 30.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < props.size(); ++i) {
    if (y + rowH < rect.y + 30.0f) { y += rowH; continue; }
    if (y > rect.y + rect.h) break;

    if (!props[i].enabled) {
      drawRect({rect.x, y, rect.w, rowH - 2.0f},
               kPanelAlt, 0.0f);
    }
    const Color tint = props[i].enabled
        ? Color{0.0f, 0.0f, 0.0f, 0.0f}
        : Color{0.5f, 0.5f, 0.5f, 1.0f};
    drawField({rect.x, y, rect.w, rowH},
              props[i].name.c_str(),
              formatValue(props[i]),
              tint);

    // Color swatch for Color kind.
    if (props[i].kind == InspectorKind::Color) {
      const Rect swatch = {rect.x + rect.w - 18.0f,
                           y + 4.0f, 12.0f, 12.0f};
      drawRect(swatch,
               {props[i].v0, props[i].v1, props[i].v2, 1.0f}, 1.0f);
    }
    y += rowH;
  }
  popClip();
}

}

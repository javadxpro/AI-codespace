#include <kimia/InspectorModePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
const char* tabName(InspectorTab t) {
  switch (t) {
    case InspectorTab::Properties: return "Properties";
    case InspectorTab::Physics:    return "Physics";
    case InspectorTab::Material:   return "Material";
    case InspectorTab::Particle:   return "Particle";
    case InspectorTab::Script:     return "Script";
  }
  return "?";
}
}

void drawInspectorModePanel(const Rect& rect, InspectorTab activeTab) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Inspector", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 btnW = 90.0f;
  constexpr f32 btnH = 18.0f;
  f32 y = rect.y + 22.0f;
  const f32 padX = 4.0f;
  const InspectorTab tabs[] = {
    InspectorTab::Properties, InspectorTab::Physics,
    InspectorTab::Material,   InspectorTab::Particle,
    InspectorTab::Script,
  };
  for (usize i = 0; i < 5; ++i) {
    if (y + btnH > rect.y + rect.h) break;
    const bool active = tabs[i] == activeTab;
    drawRect({rect.x + padX, y, rect.w - 2 * padX, btnH},
             active ? kAccent : kPanelAlt, 2.0f);
    drawText(tabName(tabs[i]),
             rect.x + padX + 8.0f, y + 4.0f, 1,
             active ? kAccentHot : kText);
    y += btnH + 4.0f;
  }
}

}

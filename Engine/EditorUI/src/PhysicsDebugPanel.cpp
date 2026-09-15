#include <kimia/PhysicsDebugPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
const char* name(DebugDrawMode m) {
  switch (m) {
    case DebugDrawMode::Off:       return "Off";
    case DebugDrawMode::Wireframe: return "Wireframe";
    case DebugDrawMode::Normals:   return "Normals";
    case DebugDrawMode::Contacts:  return "Contacts";
    case DebugDrawMode::All:       return "All";
  }
  return "?";
}
}

void drawPhysicsDebugPanel(const Rect& rect, DebugDrawMode mode) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Physics Debug", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 btnW = 80.0f;
  constexpr f32 btnH = 18.0f;
  const f32 padX = 4.0f;
  f32 y = rect.y + 22.0f;
  const char* labels[] = {"Off", "Wire", "Norm", "Contacts", "All"};
  const DebugDrawMode modes[] = {
    DebugDrawMode::Off, DebugDrawMode::Wireframe,
    DebugDrawMode::Normals, DebugDrawMode::Contacts,
    DebugDrawMode::All,
  };
  for (usize i = 0; i < 5; ++i) {
    const bool active = modes[i] == mode;
    drawRect({rect.x + padX + static_cast<float>(i) * (btnW + 2.0f),
              y, btnW, btnH},
             active ? kAccent : kPanelAlt, 2.0f);
    drawText(labels[i],
             rect.x + padX + static_cast<float>(i) * (btnW + 2.0f) +
                 btnW * 0.5f - 8.0f,
             y + 4.0f, 1,
             active ? kAccentHot : kText);
  }
  y += btnH + 4.0f;

  // Current mode description.
  drawText(name(mode),
           rect.x + 4.0f, y, 1, kTextMuted);
}

}

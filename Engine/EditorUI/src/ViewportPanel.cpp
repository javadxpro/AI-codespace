#include <kimia/ViewportPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
const char* modeName(ViewportMode m) {
  switch (m) {
    case ViewportMode::Perspective: return "Perspective";
    case ViewportMode::Top:         return "Top";
    case ViewportMode::Front:       return "Front";
    case ViewportMode::Side:        return "Side";
  }
  return "?";
}
}

void drawViewportPanel(const Rect& rect,
                       ViewportMode mode,
                       const std::string& title) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Tab bar at the top — 4 small tabs.
  constexpr f32 tabW = 64.0f;
  constexpr f32 tabH = 18.0f;
  const char* tabs[] = {"Persp", "Top", "Front", "Side"};
  const ViewportMode modes[] = {
    ViewportMode::Perspective,
    ViewportMode::Top,
    ViewportMode::Front,
    ViewportMode::Side,
  };
  for (usize i = 0; i < 4; ++i) {
    const Rect tab{rect.x + static_cast<float>(i) * tabW,
                   rect.y, tabW, tabH};
    const bool active = modes[i] == mode;
    drawRect(tab, active ? kAccentDim : kPanelAlt, 0.0f);
    drawText(tabs[i],
             tab.x + 6.0f, tab.y + 4.0f, 1,
             active ? kAccentHot : kText);
  }

  // Scene area (placeholder — the actual scene is drawn under
  // the editor by the render path, this is just the chrome).
  const Rect scene{
      rect.x, rect.y + tabH,
      rect.w, rect.h - tabH};
  drawRect(scene, kPanelAlt, 0.0f);

  // Title overlay.
  drawText(title.empty() ? modeName(mode) : title.c_str(),
           scene.x + 6.0f, scene.y + 4.0f, 1, kTextMuted);

  // Axis gizmo in the bottom-right corner.
  const f32 gizmo = 30.0f;
  const f32 gx = scene.x + scene.w - gizmo - 4.0f;
  const f32 gy = scene.y + scene.h - gizmo - 4.0f;
  drawRect({gx, gy, gizmo, gizmo}, {0.0f, 0.0f, 0.0f, 0.4f}, 2.0f);
  // X axis (red).
  drawRect({gx + gizmo * 0.5f - 1.0f, gy + gizmo * 0.5f - 1.0f,
            gizmo * 0.4f, 2.0f}, {1.0f, 0.3f, 0.3f, 1.0f}, 0.0f);
  // Y axis (green).
  drawRect({gx + gizmo * 0.5f - 1.0f, gy + gizmo * 0.5f - 1.0f,
            2.0f, gizmo * 0.4f}, {0.3f, 1.0f, 0.3f, 1.0f}, 0.0f);
  // Z axis (blue).
  drawRect({gx + gizmo * 0.5f - 1.0f, gy + gizmo * 0.5f - 1.0f,
            2.0f, 2.0f}, {0.3f, 0.3f, 1.0f, 1.0f}, 0.0f);
}

}

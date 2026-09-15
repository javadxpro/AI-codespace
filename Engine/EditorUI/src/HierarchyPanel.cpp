#include <kimia/HierarchyPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawHierarchyPanel(const Rect& rect,
                        const std::vector<HierarchyNode>& nodes,
                        i32 selectedIndex,
                        i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Hierarchy", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  constexpr f32 indentStep = 12.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    const bool sel = (static_cast<i32>(i) == selectedIndex);
    if (sel) {
      drawRect({rect.x, y, rect.w, rowH - 2.0f},
               kAccent, 0.0f);
    }

    // Visibility toggle.
    const Color vc = nodes[i].visible ? kAccent : kTextMuted;
    drawRect({rect.x + 4.0f, y + 4.0f, 10.0f, 10.0f},
             nodes[i].visible ? kAccent : kPanelAlt, 1.0f);
    if (!nodes[i].visible) {
      drawRect({rect.x + 4.0f, y + 8.0f, 10.0f, 2.0f},
               kError, 0.0f);
    }

    // Expand marker.
    const f32 mx = rect.x + 4.0f + indentStep + nodes[i].depth * indentStep;
    if (nodes[i].depth > 0) {
      drawRect({mx - 4.0f, y + rowH * 0.5f, 6.0f, 1.0f},
               kTextMuted, 0.0f);
    }

    const f32 nx = mx + 4.0f;
    drawText(nodes[i].name.c_str(),
             nx, y + 3.0f, 1,
             sel ? kAccentHot : (nodes[i].visible ? kText : kTextMuted));
    (void)vc;
  }
  popClip();
}

}

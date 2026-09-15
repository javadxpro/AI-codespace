// HierarchyTreePanel implementation — see HierarchyTreePanel.h.
#include <kimia/HierarchyTreePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawHierarchyTreePanel(const Rect& rect,
                            const std::vector<HierarchyNode>& nodes,
                            i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Hierarchy", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 16.0f;
  constexpr f32 padX = 6.0f;
  constexpr f32 indent = 12.0f;
  const f32 startY = rect.y + 16.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const f32 y = startY + static_cast<f32>(i) * rowH;
    if (y + rowH < rect.y + 16.0f) continue;
    if (y > rect.y + rect.h) break;

    const float x = rect.x + padX +
                     static_cast<float>(nodes[i].depth) * indent;

    if (nodes[i].selected) {
      drawRect({rect.x, y, rect.w, rowH}, kAccentDim, 0.0f);
    }

    // Expand/collapse triangle.
    if (nodes[i].hasChildren) {
      drawText(nodes[i].expanded ? "v" : ">",
               x, y + 4.0f, 1, kTextMuted);
    }

    const Color textColor = nodes[i].selected ? kAccentHot : kText;
    drawText(nodes[i].name.c_str(),
             x + (nodes[i].hasChildren ? 8.0f : 0.0f),
             y + 4.0f, 1, textColor);
  }
  popClip();
}

}  // namespace kimia::ui

#include <kimia/NodeEditorPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

namespace {
constexpr f32 kNodeW = 100.0f;
constexpr f32 kHeaderH = 18.0f;
constexpr f32 kPortH = 14.0f;
constexpr f32 kPortSize = 8.0f;
}

void drawNodeEditorPanel(const Rect& rect,
                         const std::vector<NodeGraphNode>& nodes,
                         const std::vector<NodeGraphLink>& links,
                         Vec2 pan) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  // Background grid (5 px squares) for spatial orientation.
  for (f32 x = std::fmod(pan.x, 16.0f); x < rect.w; x += 16.0f) {
    if (x < 0.0f) continue;
    drawRect({rect.x + x, rect.y, 1.0f, rect.h}, kBorder, 0.0f);
  }
  for (f32 y = std::fmod(pan.y, 16.0f); y < rect.h; y += 16.0f) {
    if (y < 0.0f) continue;
    drawRect({rect.x, rect.y + y, rect.w, 1.0f}, kBorder, 0.0f);
  }

  pushClip(rect);

  // Draw links first (so they sit under the nodes).
  for (const auto& link : links) {
    if (link.fromNode < 0 || link.toNode < 0) continue;
    if (static_cast<usize>(link.fromNode) >= nodes.size()) continue;
    if (static_cast<usize>(link.toNode) >= nodes.size()) continue;
    const auto& a = nodes[link.fromNode];
    const auto& b = nodes[link.toNode];
    const f32 ax = rect.x + a.position.x + kNodeW + pan.x;
    const f32 ay = rect.y + a.position.y + kHeaderH +
                   kPortH * static_cast<f32>(link.fromOutput) + pan.y;
    const f32 bx = rect.x + b.position.x + pan.x;
    const f32 by = rect.y + b.position.y + kHeaderH +
                   kPortH * static_cast<f32>(link.toInput) + pan.y;
    drawRect({(ax + bx) * 0.5f - 0.5f,
              std::min(ay, by),
              1.0f, std::abs(by - ay)}, kAccent, 0.0f);
    drawRect({std::min(ax, bx), ay - 0.5f,
              std::abs(bx - ax), 1.0f}, kAccent, 0.0f);
  }

  // Draw each node.
  for (const auto& node : nodes) {
    const f32 x = rect.x + node.position.x + pan.x;
    const f32 y = rect.y + node.position.y + pan.y;
    const f32 h = kHeaderH + kPortH *
        static_cast<f32>(std::max(node.inputs.size(),
                                   node.outputs.size()));
    // Body.
    drawRect({x, y, kNodeW, h}, kPanelAlt, 2.0f);
    // Header.
    drawRect({x, y, kNodeW, kHeaderH}, kAccentDim, 0.0f);
    drawText(node.name.c_str(),
             x + 4.0f, y + 4.0f, 1, kText);
    // Input ports.
    for (usize i = 0; i < node.inputs.size(); ++i) {
      const f32 py = y + kHeaderH + kPortH * static_cast<float>(i);
      drawRect({x - kPortSize * 0.5f, py + (kPortH - kPortSize) * 0.5f,
                kPortSize, kPortSize}, kAccent, 4.0f);
      drawText(node.inputs[i].c_str(),
               x + 6.0f, py + 2.0f, 1, kTextMuted);
    }
    // Output ports.
    for (usize i = 0; i < node.outputs.size(); ++i) {
      const f32 py = y + kHeaderH + kPortH * static_cast<float>(i);
      drawRect({x + kNodeW - kPortSize * 0.5f,
                py + (kPortH - kPortSize) * 0.5f,
                kPortSize, kPortSize}, kWarning, 4.0f);
      drawText(node.outputs[i].c_str(),
               x + kNodeW - 30.0f, py + 2.0f, 1, kTextMuted);
    }
  }
  popClip();
}

}

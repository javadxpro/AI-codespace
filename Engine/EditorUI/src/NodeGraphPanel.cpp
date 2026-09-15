#include <kimia/NodeGraphPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

namespace {
Rect nodeToScreen(const Rect& rect,
                  const NodeItem& n,
                  f32 panX, f32 panY, f32 zoom) {
  return {rect.x + (n.x + panX) * zoom,
          rect.y + (n.y + panY) * zoom,
          n.w * zoom,
          n.h * zoom};
}
void pinPos(const Rect& nodeRect,
            const NodeItem& n, std::size_t idx,
            bool output, f32& px, f32& py) {
  const f32 y = nodeRect.y + 18.0f +
                static_cast<f32>(idx) * 14.0f;
  px = output ? nodeRect.x + nodeRect.w : nodeRect.x;
  py = y;
}
}

void drawNodeGraphPanel(const Rect& rect,
                        const std::vector<NodeItem>& nodes,
                        const std::vector<NodeConnection>& conns,
                        i32 selectedNode,
                        f32 panX, f32 panY,
                        f32 zoom) {
  using namespace theme;
  drawRect(rect, kPanelAlt, 0.0f);

  if (zoom <= 0.0f) return;

  // Connections first (under nodes).
  for (std::size_t c = 0; c < conns.size(); ++c) {
    if (conns[c].fromNode < 0 ||
        conns[c].fromNode >= static_cast<i32>(nodes.size()) ||
        conns[c].toNode < 0 ||
        conns[c].toNode >= static_cast<i32>(nodes.size())) continue;
    const Rect aR = nodeToScreen(rect, nodes[conns[c].fromNode],
                                 panX, panY, zoom);
    const Rect bR = nodeToScreen(rect, nodes[conns[c].toNode],
                                 panX, panY, zoom);
    f32 ax, ay, bx, by;
    pinPos(aR, nodes[conns[c].fromNode],
           static_cast<std::size_t>(conns[c].fromPin), true, ax, ay);
    pinPos(bR, nodes[conns[c].toNode],
           static_cast<std::size_t>(conns[c].toPin),   false, bx, by);
    // Draw two thin rects as bezier approximation.
    const f32 lx = std::min(ax, bx);
    const f32 lw = std::abs(bx - ax);
    const f32 ly = std::min(ay, by);
    const f32 lh = std::max(2.0f, std::abs(by - ay));
    drawRect({lx, ly, lw, lh}, kAccent, 0.0f);
  }

  pushClip(rect);
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const Rect r = nodeToScreen(rect, nodes[i], panX, panY, zoom);
    if (r.x + r.w < rect.x || r.y + r.h < rect.y ||
        r.x > rect.x + rect.w || r.y > rect.y + rect.h) continue;

    drawRect(r, {nodes[i].r / 255.0f,
                 nodes[i].g / 255.0f,
                 nodes[i].b / 255.0f,
                 1.0f},
             static_cast<i32>(i) == selectedNode ? 3.0f : 1.0f);

    drawText(nodes[i].title.c_str(),
             r.x + 4.0f, r.y + 4.0f, 1, kAccentHot);

    // Pins.
    f32 py = r.y + 18.0f;
    for (std::size_t p = 0; p < nodes[i].pins.size(); ++p) {
      const bool out = nodes[i].pins[p].output;
      drawRect({out ? r.x + r.w - 4.0f : r.x,
                py, 4.0f, 4.0f},
               out ? kAccent : kSuccess, 0.0f);
      drawText(nodes[i].pins[p].name.c_str(),
               r.x + (out ? r.w - 60.0f : 8.0f), py - 2.0f, 1, kText);
      py += 14.0f;
    }
  }
  popClip();
}

}

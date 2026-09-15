#include <kimia/MinimapPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawMinimapPanel(const Rect& rect,
                      const Vec2& viewCenter,
                      f32 viewSize,
                      const std::vector<MinimapDot>& dots) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Map", rect.x + 4.0f, rect.y + 2.0f, 1, kTextMuted);

  const f32 mapX = rect.x;
  const f32 mapY = rect.y + 12.0f;
  const f32 mapW = rect.w;
  const f32 mapH = rect.h - 14.0f;

  // World → minimap mapping. We treat Z as the up-axis in the
  // top-down minimap view (X horizontal, Y vertical), with
  // viewSize metres fitting in the rect.
  if (viewSize <= 0.0f) return;
  const f32 scale = std::min(mapW, mapH) / viewSize;

  // Axis-aligned view rectangle.
  const f32 vx = mapX + (mapW - viewSize * scale) * 0.5f;
  const f32 vy = mapY + (mapH - viewSize * scale) * 0.5f;
  drawRect({vx, vy, viewSize * scale, viewSize * scale},
           kPanelAlt, 1.0f);

  // World center cross-hair.
  drawRect({mapX + mapW * 0.5f - 4.0f, mapY + mapH * 0.5f,
            8.0f, 1.0f}, kAccent, 0.0f);
  drawRect({mapX + mapW * 0.5f, mapY + mapH * 0.5f - 4.0f,
            1.0f, 8.0f}, kAccent, 0.0f);

  // Dots.
  for (const auto& d : dots) {
    const f32 dx = mapX + mapW * 0.5f +
                   static_cast<float>(d.worldPos.x - viewCenter.x) * scale;
    const f32 dy = mapY + mapH * 0.5f -
                   static_cast<float>(d.worldPos.y - viewCenter.y) * scale;
    drawRect({dx - 1.5f, dy - 1.5f, 3.0f, 3.0f}, d.color, 1.0f);
  }
  (void)viewCenter;
}

}

#include <kimia/MiniMapPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
Rect worldToRect(const Rect& panel,
                 f32 wx, f32 wy, f32 ww, f32 wh,
                 f32 minX, f32 minY, f32 maxX, f32 maxY) {
  const f32 worldW = maxX - minX;
  const f32 worldH = maxY - minY;
  if (worldW <= 0.0f || worldH <= 0.0f) {
    return {panel.x, panel.y, 0.0f, 0.0f};
  }
  const f32 nx = (wx - minX) / worldW;
  const f32 ny = (wy - minY) / worldH;
  const f32 nw = ww / worldW;
  const f32 nh = wh / worldH;
  return {panel.x + nx * panel.w,
          panel.y + ny * panel.h,
          nw * panel.w,
          nh * panel.h};
}
}

void drawMiniMapPanel(const Rect& rect,
                      const std::vector<MiniMapItem>& items,
                      f32 worldMinX, f32 worldMinY,
                      f32 worldMaxX, f32 worldMaxY,
                      f32 viewportX, f32 viewportY,
                      f32 viewportW, f32 viewportH) {
  using namespace theme;
  drawRect(rect, kPanelAlt, 0.0f);

  // Border.
  drawRect({rect.x, rect.y, rect.w, 1.0f}, kTextMuted, 0.0f);
  drawRect({rect.x, rect.y + rect.h - 1.0f, rect.w, 1.0f}, kTextMuted, 0.0f);
  drawRect({rect.x, rect.y, 1.0f, rect.h}, kTextMuted, 0.0f);
  drawRect({rect.x + rect.w - 1.0f, rect.y, 1.0f, rect.h}, kTextMuted, 0.0f);

  // Items.
  for (std::size_t i = 0; i < items.size(); ++i) {
    const Rect r = worldToRect(rect,
                               items[i].worldX, items[i].worldY,
                               items[i].worldW, items[i].worldH,
                               worldMinX, worldMinY,
                               worldMaxX, worldMaxY);
    if (r.w <= 0.0f || r.h <= 0.0f) continue;
    drawRect(r, {items[i].r / 255.0f,
                 items[i].g / 255.0f,
                 items[i].b / 255.0f,
                 1.0f}, 0.0f);
  }

  // Viewport rect (highlighted).
  const Rect vp = worldToRect(rect,
                              viewportX, viewportY,
                              viewportW, viewportH,
                              worldMinX, worldMinY,
                              worldMaxX, worldMaxY);
  drawRect(vp, kAccent, 1.0f);
}

}

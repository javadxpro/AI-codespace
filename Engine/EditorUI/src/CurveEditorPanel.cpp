#include <kimia/CurveEditorPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

void drawCurveEditorPanel(const Rect& rect,
                          const std::vector<CurveKey>& keys) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Curve", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  if (keys.size() < 2) return;

  // The grid area is below the title row.
  const f32 gx = rect.x + 4.0f;
  const f32 gy = rect.y + 18.0f;
  const f32 gw = rect.w - 8.0f;
  const f32 gh = rect.h - 22.0f;

  // Background grid (2 horizontal lines at 25% and 75%).
  drawRect({gx, gy + gh * 0.25f, gw, 1.0f}, kBorder, 0.0f);
  drawRect({gx, gy + gh * 0.75f, gw, 1.0f}, kBorder, 0.0f);
  drawRect({gx + gw * 0.5f, gy, 1.0f, gh}, kBorder, 0.0f);

  // Find value range to scale the curve vertically.
  f32 vMin = keys[0].v, vMax = keys[0].v;
  for (const auto& k : keys) {
    if (k.v < vMin) vMin = k.v;
    if (k.v > vMax) vMax = k.v;
  }
  const f32 vRange = vMax - vMin;
  const auto toScreen = [&](float t, float v) -> std::pair<f32, f32> {
    const f32 sx = gx + t * gw;
    const f32 ratio = vRange > 0.0f ? (v - vMin) / vRange : 0.5f;
    const f32 sy = gy + (1.0f - ratio) * gh;
    return {sx, sy};
  };

  // Draw the polyline as adjacent 1-pixel rects (no GL line primitive).
  for (std::size_t i = 0; i + 1 < keys.size(); ++i) {
    const auto p0 = toScreen(keys[i].t, keys[i].v);
    const auto p1 = toScreen(keys[i+1].t, keys[i+1].v);
    const f32 x0 = std::min(p0.first, p1.first);
    const f32 x1 = std::max(p0.first, p1.first);
    const f32 yMid = (p0.second + p1.second) * 0.5f;
    drawRect({x0, yMid - 0.5f, x1 - x0, 1.0f}, kAccent, 0.0f);
  }

  // Draw the keys as small dots.
  for (const auto& k : keys) {
    const auto p = toScreen(k.t, k.v);
    drawRect({p.first - 2.0f, p.second - 2.0f, 4.0f, 4.0f},
             kAccentHot, 2.0f);
  }
}

}

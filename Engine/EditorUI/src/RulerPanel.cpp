#include <kimia/RulerPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>
#include <cmath>

namespace kimia::ui {

namespace {
f32 stepForZoom(f32 ppu) {
  // Choose a nice round step in pixels that produces ~50-100 px spacing.
  const f32 targetPx = 80.0f;
  f32 step = targetPx / (ppu > 0.0f ? ppu : 1.0f);
  f32 mag = std::pow(10.0f, std::floor(std::log10(step)));
  f32 norm = step / mag;
  f32 nice;
  if (norm < 1.5f)      nice = 1.0f;
  else if (norm < 3.5f) nice = 2.0f;
  else if (norm < 7.5f) nice = 5.0f;
  else                  nice = 10.0f;
  return nice * mag;
}
}

void drawRulerHorizontal(const Rect& rect, f32 origin, f32 ppu) {
  using namespace theme;
  drawRect(rect, kPanelAlt, 0.0f);

  const f32 step = stepForZoom(ppu);
  const i32 first = static_cast<i32>(std::floor((origin - rect.x) / (step * ppu)));
  const i32 last  = static_cast<i32>(std::ceil(((origin - rect.x) + rect.w) / (step * ppu)));

  for (i32 i = first; i <= last; ++i) {
    const f32 x = origin + static_cast<f32>(i) * step * ppu;
    if (x < rect.x || x > rect.x + rect.w) continue;
    drawRect({x, rect.y + rect.h - 6.0f, 1.0f, 5.0f},
             kTextMuted, 0.0f);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", i);
    drawText(buf, x + 2.0f, rect.y + 2.0f, 1, kTextMuted);
  }
  // Top accent.
  drawRect({rect.x, rect.y, rect.w, 1.0f}, kAccent, 0.0f);
}

void drawRulerVertical(const Rect& rect, f32 origin, f32 ppu) {
  using namespace theme;
  drawRect(rect, kPanelAlt, 0.0f);

  const f32 step = stepForZoom(ppu);
  const i32 first = static_cast<i32>(std::floor((origin - rect.y) / (step * ppu)));
  const i32 last  = static_cast<i32>(std::ceil(((origin - rect.y) + rect.h) / (step * ppu)));

  for (i32 i = first; i <= last; ++i) {
    const f32 y = origin + static_cast<f32>(i) * step * ppu;
    if (y < rect.y || y > rect.y + rect.h) continue;
    drawRect({rect.x + rect.w - 6.0f, y, 5.0f, 1.0f},
             kTextMuted, 0.0f);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", i);
    drawText(buf, rect.x + 2.0f, y + 2.0f, 1, kTextMuted);
  }
  // Left accent.
  drawRect({rect.x, rect.y, 1.0f, rect.h}, kAccent, 0.0f);
}

}

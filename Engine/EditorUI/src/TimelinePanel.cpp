#include <kimia/TimelinePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawTimelinePanel(const Rect& rect,
                       const std::vector<TimelineBlock>& blocks,
                       f32 viewStart, f32 viewEnd,
                       f32 playheadTime) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Timeline", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rulerH = 18.0f;
  const Rect ruler = {rect.x + 4.0f, rect.y + 18.0f,
                      rect.w - 8.0f, rulerH};
  drawRect(ruler, kPanelAlt, 0.0f);

  const f32 viewSpan = viewEnd - viewStart;
  if (viewSpan <= 0.0f) return;

  // Ruler ticks.
  const i32 ticks = 10;
  for (i32 i = 0; i <= ticks; ++i) {
    const f32 t = viewStart + viewSpan * static_cast<f32>(i) / ticks;
    const f32 x = ruler.x + static_cast<f32>(i) / ticks * ruler.w;
    drawRect({x, ruler.y + rulerH - 5.0f, 1.0f, 5.0f},
             kTextMuted, 0.0f);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.1fs", static_cast<double>(t));
    drawText(buf, x + 2.0f, ruler.y + 2.0f, 1, kTextMuted);
  }

  // Blocks.
  const Rect laneArea = {rect.x + 4.0f,
                         ruler.y + rulerH + 4.0f,
                         rect.w - 8.0f,
                         rect.h - (rulerH + 22.0f)};
  constexpr f32 blockH = 20.0f;
  for (std::size_t i = 0; i < blocks.size(); ++i) {
    const f32 y = laneArea.y + static_cast<float>(i) * (blockH + 4.0f);
    if (y + blockH > laneArea.y + laneArea.h) break;
    const f32 x0 = laneArea.x + (blocks[i].startTime - viewStart) /
                                 viewSpan * laneArea.w;
    const f32 x1 = laneArea.x + (blocks[i].startTime + blocks[i].duration -
                                 viewStart) / viewSpan * laneArea.w;
    if (x1 < laneArea.x || x0 > laneArea.x + laneArea.w) continue;
    const f32 lx = std::max(x0, laneArea.x);
    const f32 lw = std::min(x1, laneArea.x + laneArea.w) - lx;
    if (lw <= 0.0f) continue;
    const Color c = {blocks[i].r / 255.0f,
                     blocks[i].g / 255.0f,
                     blocks[i].b / 255.0f,
                     blocks[i].selected ? 1.0f : 0.6f};
    drawRect({lx, y, lw, blockH}, c, 2.0f);
    drawText(blocks[i].label.c_str(),
             lx + 4.0f, y + 4.0f, 1, kAccentHot);
  }

  // Playhead.
  const f32 px = laneArea.x + (playheadTime - viewStart) /
                              viewSpan * laneArea.w;
  if (px > laneArea.x && px < laneArea.x + laneArea.w) {
    drawRect({px - 1.0f, ruler.y, 2.0f, laneArea.h + 4.0f},
             kAccentHot, 0.0f);
  }
}

}

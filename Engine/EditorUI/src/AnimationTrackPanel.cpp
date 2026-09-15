#include <kimia/AnimationTrackPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawAnimationTrackPanel(const Rect& rect,
                             const std::vector<AnimTrack>& tracks,
                             f32 viewStart, f32 viewEnd,
                             f32 playheadTime,
                             i32 selectedTrack) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  constexpr f32 leftW = 80.0f;
  const Rect trackArea = {rect.x, rect.y + 16.0f,
                          rect.w, rect.h - 16.0f};
  const Rect timeArea = {rect.x + leftW, rect.y,
                         rect.w - leftW, 16.0f};
  drawRect(timeArea, kPanelAlt, 0.0f);

  // Time ticks.
  const f32 viewSpan = viewEnd - viewStart;
  if (viewSpan > 0.0f) {
    const i32 ticks = 8;
    for (i32 i = 0; i <= ticks; ++i) {
      const f32 t = viewStart + viewSpan * static_cast<f32>(i) / ticks;
      const f32 x = timeArea.x + static_cast<f32>(i) / ticks * timeArea.w;
      drawRect({x, timeArea.y + 10.0f, 1.0f, 6.0f},
               kTextMuted, 0.0f);
      char buf[16];
      std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(t));
      drawText(buf, x + 2.0f, timeArea.y + 2.0f, 1, kTextMuted);
    }
  }

  constexpr f32 trackH = 24.0f;
  for (std::size_t i = 0; i < tracks.size(); ++i) {
    const f32 y = trackArea.y + static_cast<float>(i) * trackH;
    if (y + trackH > rect.y + rect.h) break;

    const bool sel = (static_cast<i32>(i) == selectedTrack);
    drawRect({rect.x, y, leftW, trackH},
             sel ? kAccentDim : kPanelAlt, 0.0f);
    drawText(tracks[i].name.c_str(),
             rect.x + 4.0f, y + 6.0f, 1,
             sel ? kAccentHot : kText);

    const Rect lane = {rect.x + leftW, y,
                       rect.w - leftW, trackH};
    drawRect(lane, kPanel, 0.0f);
    drawRect({lane.x, lane.y + lane.h * 0.5f, lane.w, 1.0f},
             kTextMuted, 0.0f);

    if (viewSpan <= 0.0f) continue;
    for (std::size_t k = 0; k < tracks[i].keys.size(); ++k) {
      const f32 kx = lane.x + (tracks[i].keys[k].time - viewStart) /
                                  viewSpan * lane.w;
      if (kx < lane.x || kx > lane.x + lane.w) continue;
      const f32 norm = (tracks[i].keys[k].value + 1.0f) * 0.5f;
      const f32 ky = lane.y + lane.h * (1.0f - std::min(1.0f, std::max(0.0f, norm)));
      drawRect({kx - 2.0f, ky - 2.0f, 4.0f, 4.0f},
               tracks[i].keys[k].selected ? kAccentHot : kAccent, 0.0f);
    }
  }

  // Playhead.
  if (viewSpan > 0.0f) {
    const f32 px = rect.x + leftW + (playheadTime - viewStart) /
                                  viewSpan * (rect.w - leftW);
    if (px > rect.x + leftW && px < rect.x + rect.w) {
      drawRect({px - 1.0f, rect.y + 16.0f, 2.0f, rect.h - 16.0f},
               kAccentHot, 0.0f);
    }
  }
}

}

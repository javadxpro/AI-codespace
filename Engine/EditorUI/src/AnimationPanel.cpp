#include <kimia/AnimationPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>
#include <algorithm>

namespace kimia::ui {

namespace {
const char* loopName(AnimationLoop l) {
  switch (l) {
    case AnimationLoop::Once: return "Once";
    case AnimationLoop::Loop: return "Loop";
    case AnimationLoop::PingPong: return "PingPong";
  }
  return "?";
}
}

void drawAnimationPanel(const Rect& rect, const AnimationProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText(props.clipName.c_str(),
           rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 26.0f;
  char buf[64];

  // Transport: Play / Stop / Pause.
  constexpr f32 btnW = 50.0f;
  constexpr f32 btnH = 18.0f;
  if (button("Play",  {rect.x + 4.0f,                  y, btnW, btnH})) {}
  if (button("Pause", {rect.x + 4.0f + (btnW + 4.0f),  y, btnW, btnH})) {}
  if (button("Stop",  {rect.x + 4.0f + 2.0f * (btnW + 4.0f), y, btnW, btnH})) {}
  y += rowH + 2.0f;

  // Loop mode toggle.
  if (button("Once",     {rect.x + 4.0f,                     y, btnW, btnH})) {}
  if (button("Loop",     {rect.x + 4.0f + (btnW + 4.0f),     y, btnW, btnH})) {}
  if (button("PingPong", {rect.x + 4.0f + 2.0f * (btnW + 4.0f), y, btnW + 8.0f, btnH})) {}
  y += rowH + 2.0f;

  // Playhead bar.
  const f32 barX = rect.x + 4.0f;
  const f32 barW = rect.w - 8.0f;
  drawRect({barX, y + 6.0f, barW, 4.0f}, kPanelAlt, 2.0f);
  const f32 ratio = props.durationSec > 0.0f
      ? std::max(0.0f, std::min(1.0f, props.currentTime / props.durationSec))
      : 0.0f;
  drawRect({barX, y + 6.0f, barW * ratio, 4.0f}, kAccent, 2.0f);
  y += rowH;

  // Time readout.
  std::snprintf(buf, sizeof(buf), "%.2f / %.2f s",
                static_cast<double>(props.currentTime),
                static_cast<double>(props.durationSec));
  drawText(buf, rect.x + 4.0f, y + 4.0f, 1, kText);
  y += rowH;

  // Playback rate.
  std::snprintf(buf, sizeof(buf), "%.2fx",
                static_cast<double>(props.playbackRate));
  drawText(buf, rect.x + 4.0f, y + 4.0f, 1, kTextMuted);

  (void)loopName;
}

}

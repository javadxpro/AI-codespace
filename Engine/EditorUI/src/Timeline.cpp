// Timeline implementation — see Timeline.h.
#include <kimia/Timeline.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <cstdio>
#include <string>

namespace kimia::ui {

namespace {

// Convert seconds to a "MM:SS" string for the label.
std::string formatTime(f32 seconds) {
  const i32 s = static_cast<i32>(seconds);
  const i32 mm = s / 60;
  const i32 ss = s % 60;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
  return buf;
}

}  // namespace

void drawTimeline(const Rect& rect, const TimelineState& state) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Four 24x24 transport buttons in a row.
  const f32 btn = 24.0f;
  const f32 gap = 4.0f;
  const f32 startX = rect.x + 8.0f;
  const f32 y = rect.y + (rect.h - btn) * 0.5f;

  // Play / Pause toggle: same square, different glyph.
  if (button("Play", {startX, y, btn, btn})) {
    // Phase 5: emit UiCommandKind::PlayPressed / PausePressed.
  }
  // Stop / Step / Record. The full set will land in Phase 5; for now
  // we just emit them so the user can see the transport strip is live.
  if (button("Stop", {startX + (btn + gap), y, btn, btn})) {
  }
  if (button("Step", {startX + 2.0f * (btn + gap), y, btn, btn})) {
  }
  if (button("Reset", {startX + 3.0f * (btn + gap), y, btn, btn})) {
  }

  // The horizontal playhead strip occupies the rest of the row.
  const f32 stripX = startX + 4.0f * (btn + gap) + 8.0f;
  const f32 stripW = rect.x + rect.w - stripX - 8.0f;
  const f32 stripY = rect.y + rect.h * 0.5f - 2.0f;
  drawRect({stripX, stripY, stripW, 4.0f}, kTitlebar, 2.0f);

  // Playhead dot at the normalized position.
  const f32 dotX = stripX + stripW * std::max(0.0f,
                                              std::min(1.0f, state.playhead));
  drawRect({dotX - 3.0f, rect.y + 2.0f, 6.0f, rect.h - 4.0f},
           state.playing ? kAccent : kTextDim, 2.0f);

  // Time label on the right edge of the strip.
  const std::string label = formatTime(state.totalTime);
  drawText(label.c_str(), stripX + 4.0f, stripY - 10.0f, 1, kText);
}

}  // namespace kimia::ui

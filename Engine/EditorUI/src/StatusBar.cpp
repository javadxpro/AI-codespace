// StatusBar implementation — see StatusBar.h.
#include <kimia/StatusBar.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <cstdio>
#include <cstring>

namespace kimia::ui {

void drawStatusBar(const Rect& rect, const Status& s) {
  using namespace theme;
  drawRect(rect, kTitlebar, 0.0f);

  char buf[32];

  // Left segment: tool name.
  drawText(s.tool, rect.x + 8.0f, rect.y + 4.0f, 1, kAccent);

  // Next: entity count.
  std::snprintf(buf, sizeof(buf), "%zu ent.", s.entityCount);
  drawText(buf, rect.x + 80.0f, rect.y + 4.0f, 1, kText);

  // Next: FPS, coloured the same way as StatsPanel.
  Color fpsColor = kSuccess;
  if (s.fps < 30.0f) fpsColor = kWarning;
  if (s.fps < 15.0f) fpsColor = kError;
  std::snprintf(buf, sizeof(buf), "%.0f fps", static_cast<double>(s.fps));
  drawText(buf, rect.x + 140.0f, rect.y + 4.0f, 1, fpsColor);

  // Right segment: play / pause indicator.
  const char* playState = "stopped";
  if (s.playing && s.paused) playState = "paused";
  else if (s.playing)        playState = "playing";
  drawText(playState,
           rect.x + rect.w - 60.0f, rect.y + 4.0f, 1,
           s.playing ? kAccent : kTextDim);

  // Transient hint in the center (if any).
  if (s.hint != nullptr && s.hint[0] != '\0') {
    // Centre the hint inside the bar, leaving room for the segments.
    const f32 hintW = static_cast<f32>(std::strlen(s.hint)) * 6.0f;
    const f32 cx = rect.x + rect.w * 0.5f;
    drawText(s.hint, cx - hintW * 0.5f, rect.y + 4.0f, 1, kWarning);
  }
}

}  // namespace kimia::ui

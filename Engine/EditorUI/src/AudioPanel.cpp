#include <kimia/AudioPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawAudioPanel(const Rect& rect,
                    const std::vector<AudioClipEntry>& clips) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Audio", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  const f32 startY = rect.y + 18.0f;
  char buf[32];
  for (std::size_t i = 0; i < clips.size(); ++i) {
    const f32 y = startY + static_cast<f32>(i) * rowH;
    if (y + rowH > rect.y + rect.h) break;

    // Mute / loop indicator on the left.
    drawText(clips[i].muted ? "M" : (clips[i].looped ? "L" : " "),
             rect.x + 4.0f, y + 4.0f, 1,
             clips[i].muted ? kError : (clips[i].looped ? kAccent : kTextDim));
    // Clip name.
    drawText(clips[i].name.c_str(),
             rect.x + 18.0f, y + 4.0f, 1, kText);
    // Duration on the right.
    std::snprintf(buf, sizeof(buf), "%.1fs",
                  static_cast<double>(clips[i].durationSec));
    drawText(buf,
             rect.x + rect.w - 36.0f, y + 4.0f, 1, kTextMuted);
  }
  popClip();
}

}

#include <kimia/WaveformPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>
#include <cmath>

namespace kimia::ui {

void drawWaveformPanel(const Rect& rect,
                       const std::vector<f32>& samples,
                       f32 sampleRate,
                       f32 playheadSeconds,
                       bool playing) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Waveform", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  const Rect plot = {rect.x + 4.0f, rect.y + 18.0f,
                     rect.w - 8.0f, rect.h - 30.0f};
  drawRect(plot, kPanelAlt, 0.0f);

  // Center baseline.
  drawRect({plot.x, plot.y + plot.h * 0.5f,
            plot.w, 1.0f},
           kTextMuted, 0.0f);

  if (samples.empty()) return;

  const f32 totalSecs = static_cast<f32>(samples.size()) /
                        (sampleRate > 0.0f ? sampleRate : 1.0f);
  if (totalSecs <= 0.0f) return;
  const f32 midY = plot.y + plot.h * 0.5f;
  const f32 halfH = plot.h * 0.45f;

  // Bucket samples per pixel.
  const std::size_t pixelCount = static_cast<std::size_t>(plot.w);
  if (pixelCount == 0) return;
  for (std::size_t px = 0; px < pixelCount; ++px) {
    const f32 t0 = static_cast<f32>(px) / pixelCount * totalSecs;
    const f32 t1 = static_cast<f32>(px + 1) / pixelCount * totalSecs;
    const std::size_t i0 = static_cast<std::size_t>(t0 * sampleRate);
    const std::size_t i1 = std::min(samples.size(),
                                    static_cast<std::size_t>(t1 * sampleRate));
    if (i1 <= i0) continue;
    f32 mn = 0.0f, mx = 0.0f;
    for (std::size_t i = i0; i < i1; ++i) {
      if (samples[i] < mn) mn = samples[i];
      if (samples[i] > mx) mx = samples[i];
    }
    const f32 y0 = midY - mx * halfH;
    const f32 y1 = midY - mn * halfH;
    drawRect({plot.x + static_cast<f32>(px), y0, 1.0f, std::max(1.0f, y1 - y0)},
             kAccent, 0.0f);
  }

  // Playhead.
  if (totalSecs > 0.0f) {
    const f32 px = plot.x + (playheadSeconds / totalSecs) * plot.w;
    if (px >= plot.x && px <= plot.x + plot.w) {
      drawRect({px - 1.0f, plot.y, 2.0f, plot.h},
               playing ? kAccentHot : kTextMuted, 0.0f);
    }
  }

  // Time label.
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.2f s", static_cast<double>(playheadSeconds));
  drawText(buf, rect.x + 6.0f, rect.y + rect.h - 14.0f, 1, kTextMuted);
  std::snprintf(buf, sizeof(buf), "/ %.2f s", static_cast<double>(totalSecs));
  drawText(buf, rect.x + rect.w - 60.0f, rect.y + rect.h - 14.0f,
           1, kTextMuted);
}

}

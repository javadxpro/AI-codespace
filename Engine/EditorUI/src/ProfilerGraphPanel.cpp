#include <kimia/ProfilerGraphPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

void drawProfilerGraph(const Rect& rect,
                       const std::vector<float>& samples,
                       float maxValue) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  if (samples.empty() || maxValue <= 0.0f) return;

  // Render the polyline as a series of 1-px vertical bars — this
  // is what drawRect supports without needing GL line primitives.
  const float w = rect.w;
  const float h = rect.h;
  const float stepX = w / static_cast<float>(samples.size());
  const float y0 = rect.y;
  for (std::size_t i = 0; i < samples.size(); ++i) {
    const float ratio = std::max(0.0f, std::min(1.0f, samples[i] / maxValue));
    const float barH = ratio * h;
    if (barH < 1.0f) continue;
    const float x = rect.x + static_cast<float>(i) * stepX;
    drawRect({x, y0 + (h - barH), std::max(1.0f, stepX), barH},
             kAccent, 0.0f);
  }
}

}

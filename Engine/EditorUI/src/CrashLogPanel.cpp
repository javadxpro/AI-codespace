#include <kimia/CrashLogPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawCrashLogPanel(const Rect& rect,
                       const std::vector<CrashEntry>& crashes,
                       i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  drawText("Crash Log", rect.x + 6.0f, rect.y + 4.0f, 1, kError);

  if (crashes.empty()) {
    drawText("No crashes recorded.",
             rect.x + 6.0f, rect.y + 22.0f, 1, kSuccess);
    return;
  }

  pushClip(rect);
  constexpr f32 rowH = 60.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < crashes.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    drawRect({rect.x + 4.0f, y + 2.0f,
              rect.w - 8.0f, rowH - 4.0f},
             kPanelAlt, 2.0f);
    drawText(crashes[i].timestamp.c_str(),
             rect.x + 8.0f, y + 6.0f, 1, kTextMuted);
    drawText(crashes[i].thread.c_str(),
             rect.x + rect.w - 80.0f, y + 6.0f, 1, kAccent);
    drawText(crashes[i].message.c_str(),
             rect.x + 8.0f, y + 22.0f, 1, kError);
    if (!crashes[i].stackTrace.empty()) {
      // Truncate to fit.
      const std::string& st = crashes[i].stackTrace;
      const usize maxLen = static_cast<usize>((rect.w - 16.0f) / 6.0f);
      drawText(st.substr(0, std::min(st.size(), maxLen)).c_str(),
               rect.x + 8.0f, y + 38.0f, 1, kTextMuted);
    }
  }
  popClip();
}

}

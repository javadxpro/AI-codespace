#include <kimia/ScriptEditorPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawScriptEditorPanel(const Rect& rect,
                          const std::vector<ScriptLine>& lines,
                          i32 scrollY,
                          i32 cursorLine,
                          const std::string& statusMessage) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Header bar.
  constexpr f32 headerH = 16.0f;
  drawRect({rect.x, rect.y, rect.w, headerH}, kTitlebar, 0.0f);
  drawText(statusMessage.empty() ? "script.kimia" : statusMessage.c_str(),
           rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Line numbers + code.
  pushClip(rect);
  constexpr f32 lineH = 12.0f;
  constexpr f32 lineNumW = 28.0f;
  const f32 startY = rect.y + headerH + 2.0f -
                     static_cast<f32>(scrollY);
  char buf[16];
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * lineH;
    if (y + lineH < rect.y + headerH) continue;
    if (y > rect.y + rect.h) break;

    const bool isCursor = static_cast<i32>(i) == cursorLine;
    if (isCursor) {
      drawRect({rect.x, y, rect.w, lineH}, kAccentDim, 0.0f);
    }
    // Line number.
    std::snprintf(buf, sizeof(buf), "%zu", i + 1);
    drawText(buf, rect.x + 4.0f, y, 1, kTextMuted);
    // Code.
    drawText(lines[i].text.c_str(),
             rect.x + lineNumW, y, 1,
             lines[i].highlight ? kAccent : kText);
  }
  popClip();
}

}

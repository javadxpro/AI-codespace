#include <kimia/ConsolePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
Color severityColor(i32 s) {
  using namespace theme;
  switch (s) {
    case 1: return kSuccess;
    case 2: return kWarning;
    case 3: return kError;
    default: return kText;
  }
}
}

void drawConsolePanel(const Rect& rect,
                      const std::vector<ConsoleLine>& lines,
                      const std::string& inputBuffer,
                      i32 scrollY,
                      bool autoScroll) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  drawText("Console", rect.x + 6.0f, rect.y + 4.0f, 1, kText);
  if (autoScroll) {
    drawText("auto", rect.x + rect.w - 36.0f, rect.y + 4.0f, 1, kAccent);
  }

  // Lines area: between the header and the input bar.
  const f32 headerH = 14.0f;
  const f32 inputH = 18.0f;
  const f32 lineH = 12.0f;
  const Rect linesArea{
      rect.x, rect.y + headerH,
      rect.w, rect.h - headerH - inputH - 4.0f};

  pushClip(linesArea);
  const f32 startY = linesArea.y - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * lineH;
    if (y + lineH < linesArea.y) continue;
    if (y > linesArea.y + linesArea.h) break;
    drawText(lines[i].text.c_str(),
             linesArea.x + 4.0f, y, 1, severityColor(lines[i].severity));
  }
  popClip();

  // Input bar at the bottom.
  const Rect inputBar{
      rect.x, rect.y + rect.h - inputH,
      rect.w, inputH};
  drawRect(inputBar, kPanelAlt, 2.0f);
  drawText(">", inputBar.x + 4.0f, inputBar.y + 4.0f, 1, kAccent);
  drawText(inputBuffer.c_str(),
           inputBar.x + 14.0f, inputBar.y + 4.0f, 1, kText);
  // Caret.
  const f32 caretX = inputBar.x + 14.0f +
                     static_cast<f32>(inputBuffer.size()) * 6.0f;
  drawRect({caretX, inputBar.y + 3.0f, 1.0f, inputBar.h - 6.0f},
           kAccent, 0.0f);
}

}

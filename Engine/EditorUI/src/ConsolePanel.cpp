#include <kimia/ConsolePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
Color levelColor(LogLevel l) {
  using namespace theme;
  if (l == LogLevel::Info)  return kText;
  if (l == LogLevel::Warn)  return kWarning;
  return kError;
}
const char* levelTag(LogLevel l) {
  if (l == LogLevel::Info)  return "I";
  if (l == LogLevel::Warn)  return "W";
  return "E";
}
}

void drawConsolePanel(const Rect& rect,
                      const std::vector<LogLine>& lines,
                      const std::string& input,
                      i32 scrollY,
                      bool autoScroll) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Console", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 inputH = 18.0f;
  constexpr f32 lineH = 14.0f;
  const Rect inputRect = {rect.x + 4.0f,
                          rect.y + rect.h - inputH - 4.0f,
                          rect.w - 8.0f,
                          inputH};
  const Rect linesRect = {rect.x + 4.0f,
                          rect.y + 18.0f,
                          rect.w - 8.0f,
                          rect.h - 18.0f - inputH - 8.0f};
  drawRect(linesRect, kPanelAlt, 0.0f);

  pushClip(linesRect);
  const f32 startY = linesRect.y + linesRect.h - 14.0f -
                     static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const f32 y = startY - static_cast<float>(lines.size() - 1 - i) * lineH;
    if (y + lineH < linesRect.y) continue;
    if (y > linesRect.y + linesRect.h) break;
    drawText(levelTag(lines[i].level),
             linesRect.x + 2.0f, y, 1, levelColor(lines[i].level));
    drawText(lines[i].text.c_str(),
             linesRect.x + 14.0f, y, 1, levelColor(lines[i].level));
  }
  popClip();

  // Input line.
  drawRect(inputRect, kPanel, 2.0f);
  const std::string prompt = "> " + input;
  drawText(prompt.c_str(),
           inputRect.x + 4.0f, inputRect.y + 2.0f, 1,
           input.empty() ? kTextMuted : kText);

  if (autoScroll) {
    drawText("[auto]", rect.x + rect.w - 36.0f,
             rect.y + 6.0f, 1, kAccent);
  }
}

}

// LogPanel implementation — see LogPanel.h.
#include <kimia/LogPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <cctype>
#include <cstring>
#include <string>

namespace kimia::ui {

namespace {

bool startsWith(const std::string& s, const char* prefix) {
  const usize n = std::strlen(prefix);
  if (s.size() < n) return false;
  for (usize i = 0; i < n; ++i) {
    if (std::tolower(static_cast<unsigned char>(s[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i]))) {
      return false;
    }
  }
  return true;
}

Color colorFor(LogSeverity sev) {
  using namespace theme;
  switch (sev) {
    case LogSeverity::Info:    return kSuccess;
    case LogSeverity::Warning: return kWarning;
    case LogSeverity::Error:   return kError;
    case LogSeverity::Plain:   return kText;
  }
  return kText;
}

}  // namespace

LogSeverity classifyLogLine(const std::string& line) {
  if (startsWith(line, "[err]") || startsWith(line, "error")) {
    return LogSeverity::Error;
  }
  if (startsWith(line, "[warn]") || startsWith(line, "warning")) {
    return LogSeverity::Warning;
  }
  if (startsWith(line, "[info]")) {
    return LogSeverity::Info;
  }
  return LogSeverity::Plain;
}

void drawLogPanel(const Rect& rect,
                  const std::vector<std::string>& lines,
                  i32 scrollY,
                  bool autoScroll) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Header row with the panel name + a small "auto-scroll" indicator.
  drawText("Log", rect.x + 6.0f, rect.y + 4.0f, 1, kText);
  if (autoScroll) {
    drawText("auto", rect.x + rect.w - 30.0f, rect.y + 4.0f, 1, kAccent);
  }

  pushClip(rect);
  const f32 headerH = 14.0f;
  const f32 lineH = 12.0f;
  const f32 startY = rect.y + headerH - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const f32 y = startY + static_cast<f32>(i) * lineH;
    if (y + lineH < rect.y + headerH) continue;
    if (y > rect.y + rect.h) break;
    const Color c = colorFor(classifyLogLine(lines[i]));
    drawText(lines[i].c_str(), rect.x + 4.0f, y, 1, c);
  }
  popClip();
}

}  // namespace kimia::ui

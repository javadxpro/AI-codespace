// LogPanel — the bottom dock that renders the editor's recent log
// lines (the SceneSnapshot.logLines vector). Each line gets a
// severity colour: red for "error", yellow for "warning", green
// for "info", default for plain text. Lines with no severity tag
// fall back to the default theme colour.
//
// Phase 4+: pure render. The log ring buffer lives in EditorUI
// (the global logLines() vector) and the WorldEditor pushes into
// it through SceneSnapshot.
#pragma once

#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

enum class LogSeverity {
  Plain,
  Info,
  Warning,
  Error,
};

// Classify a single log line by the leading "[INFO]"/"[WARN]"/"[ERR]"
// tag. The default is Plain (no colour).
LogSeverity classifyLogLine(const std::string& line);

// Render the log panel inside the given rect. `autoScroll` makes the
// panel scroll to the bottom of the log on every frame so the user
// always sees the newest line.
void drawLogPanel(const Rect& rect,
                  const std::vector<std::string>& lines,
                  i32 scrollY,
                  bool autoScroll);

}  // namespace kimia::ui

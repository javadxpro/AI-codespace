// VersionPanel — the small overlay that shows the engine version,
// build date, Git commit, and target device. Useful for bug
// reports so the user can copy a single block of info instead of
// having to remember each piece.
//
// Phase 4+: render only. The values come from Version.h and a
// few constexpr strings baked into the binary.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

struct VersionInfo {
  const char* version = "";       // e.g. "0.30.0"
  const char* buildDate = "";     // ISO-8601
  const char* gitCommit = "";     // short hash
  const char* targetDevice = "";  // e.g. "Poco X3 Pro (arm64-v8a)"
};

void drawVersionPanel(const Rect& rect, const VersionInfo& info);

}  // namespace kimia::ui

// SceneBrowser — the panel that lists every saved scene (.kimia
// file) and lets the user load one with a single tap. Phase 4+ is
// read-only: the WorldEditor populates SceneSnapshot.sceneFiles
// from its scenes directory and this panel just renders the list.
//
// Phase 5+ wires the tap to emit a UiCommandKind::LoadScene and
// show a confirmation dialog if the current scene has unsaved
// changes.
#pragma once

#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

struct SceneEntry {
  std::string name;        // file basename without .kimia
  std::string fullPath;    // absolute (or scene-dir-relative) path
  bool dirty = false;      // marked when the user has unsaved changes
  bool currentScene = false;
};

void drawSceneBrowser(const Rect& rect,
                      const std::vector<SceneEntry>& scenes,
                      i32 scrollY);

}  // namespace kimia::ui

// SceneFile — thin wrapper over WorldEditor::saveWorld / loadWorld
// that returns a UiCommand ready to be pushed onto the undo stack.
//
// The editor's toolbar has a "Save" and "Load" button. Each one
// produces a UiCommand of kind SaveScene / LoadScene with the
// chosen file name. This helper translates that command into
// the WorldEditor's loadWorld/saveWorld call and reports any
// I/O error back through a status string so the UI can show
// "saved as foo.kimia" / "failed to load: ...".
#pragma once

#include "EditorUI.h"
#include <string>

namespace kimia {
class WorldEditor;
}

namespace kimia::ui {

enum class SceneFileStatus {
  Ok,
  NoPath,
  LoadFailed,
  SaveFailed,
  AlreadyUpToDate,  // save: nothing changed since last save
};

struct SceneFileResult {
  SceneFileStatus status = SceneFileStatus::Ok;
  std::string message;   // human-readable
  i64 bytesWritten = 0;
};

// Save the current world to the given path (or, if path is empty,
// the current editor scene path). On success returns Ok and fills
// bytesWritten.
SceneFileResult saveScene(::kimia::WorldEditor& editor,
                          const std::string& path);

// Load the world from the given path into the editor. The editor
// is reset to the freshly-loaded world (UndoStack should be cleared
// by the caller since the previous undo history no longer applies).
SceneFileResult loadScene(::kimia::WorldEditor& editor,
                          const std::string& path);

}  // namespace kimia::ui

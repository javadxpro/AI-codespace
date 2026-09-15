// HostBridge: an adapter between WorldEditor (the engine) and the
// EditorUI's SceneSnapshot (what the UI needs to render). Lives in the
// EditorUI namespace because it is the only place that needs to know
// about both layers.
//
// The Android JNI (and later the D3D11 PC app) wires this up: it owns a
// WorldEditor instance, builds a SceneSnapshot from its current state,
// calls EditorUI::draw(), then applies the resulting UiCommands back to
// the engine. The editor never sees the host platform.
#pragma once

#include "EditorUI.h"

#include <string>
#include <vector>

namespace kimia {

class WorldEditor;

}

namespace kimia::ui {

// Read the engine's current state into a SceneSnapshot that the editor
// can render. The world may be null (empty scene) — the editor still
// renders the chrome around it.
SceneSnapshot snapshotFrom(WorldEditor* editor);

// Apply a single UiCommand to the engine. Returns true if the command
// was consumed (the editor may emit commands the engine can't apply
// yet, like Phase 2 timeline events; those return false).
bool applyCommand(WorldEditor* editor, const UiCommand& cmd);

// Apply a batch of commands in order. Useful at the end of EditorUI::draw.
void applyCommands(WorldEditor* editor, const std::vector<UiCommand>& cmds);

}  // namespace kimia::ui

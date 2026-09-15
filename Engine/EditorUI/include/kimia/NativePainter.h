// NativePainter — the host-side glue that turns a live WorldEditor into
// a SceneSnapshot, hands it to the EditorUI, and applies the resulting
// UiCommands back to the engine. This is the single seam the Android
// jni_glue uses to overlay the in-process UI on top of the captured
// scene frame; the desktop / test harnesses exercise the same code.
//
// Kept in EditorUI (rather than in the Android shim) so the desktop
// build and the test build can both link and regression-test it.
#pragma once

#include "EditorUI.h"
#include "HostBridge.h"

namespace kimia {
class WorldEditor;
struct Image;
}

namespace kimia::ui {

// One-shot overlay: build the snapshot, draw, rasterise, apply commands.
// `image` keeps its RGB(A) contents (the scene frame) and gains the
// editor's translucent panels. Safe to call every frame.
void paintNativeEditor(Image& image, WorldEditor& editor);

}  // namespace kimia::ui

// NativePainter implementation — see NativePainter.h for the design.
//
// Phase 2: the host (Android jni_glue) builds a SceneSnapshot from the
// live WorldEditor, hands it to EditorUI, drains the DrawCmd list and
// composites it on top of the captured scene frame. This file is the
// portable version of that loop; the Android side calls into it.
#include <kimia/NativePainter.h>
#include <kimia/EditorUI.h>
#include <kimia/HostBridge.h>
#include <kimia/RasterBridge.h>
#include <kimia/World.h>
#include <kimia/Scene.h>
#include <kimia/Image.h>

#include <string>
#include <vector>

namespace kimia::ui {

void paintNativeEditor(::kimia::Image& image, ::kimia::WorldEditor& editor) {
  if (image.isEmpty()) return;

  FrameContext ctx;
  ctx.scene = snapshotFrom(&editor);
  if (!ctx.scene.valid) return;

  draw(ctx);

  // Drain the per-frame draw commands and composite them over the scene.
  // rasteriseOver handles both RGB and RGBA targets, so the editor
  // overlay works against either renderer output.
  const std::vector<DrawCmd> cmds = takeDrawCmds();
  rasteriseOver(cmds, image);

  // Apply any UI commands the editor emitted this frame straight back
  // into the engine. The editor is the source of truth for the UI; the
  // engine stays the source of truth for the world.
  for (const UiCommand& cmd : ctx.commands) {
    applyCommand(&editor, cmd);
  }
}

}  // namespace kimia::ui

// NativePainter implementation — see NativePainter.h for the design.
//
// Phase 2 builds a SceneSnapshot from the live WorldEditor, hands it
// to EditorUI, drains the DrawCmd list and composites it on top of the
// scene frame. Phase 3 also tries the GPU path first (paintEditorGl)
// and falls back to the CPU rasteriser when the GL renderer isn't
// ready or doesn't take ownership of the frame. Both paths end up
// producing the same editor overlay; the difference is where the
// fragment math runs.
#include <kimia/NativePainter.h>
#include <kimia/EditorUI.h>
#include <kimia/EditorGl.h>
#include <kimia/HostBridge.h>
#include <kimia/RasterBridge.h>
#include <kimia/World.h>
#include <kimia/Scene.h>
#include <kimia/Image.h>

#include <string>
#include <vector>

namespace kimia::ui {

// Module-local: built once on the first call, then reused. A future
// commit will move this into EditorUI's lifecycle (init/destroy) so the
// GL path has the same lifetime as the CPU one.
namespace {
EditorGl& sharedGl() {
  static EditorGl gl;
  static bool initialised = false;
  if (!initialised) {
    std::string err;
    gl.init(err);
    initialised = true;
  }
  return gl;
}
}  // namespace

void paintNativeEditor(::kimia::Image& image, ::kimia::WorldEditor& editor) {
  if (image.isEmpty()) return;

  FrameContext ctx;
  ctx.scene = snapshotFrom(&editor);
  if (!ctx.scene.valid) return;

  draw(ctx);

  const std::vector<DrawCmd> cmds = takeDrawCmds();
  if (cmds.empty()) {
    for (const UiCommand& cmd : ctx.commands) {
      applyCommand(&editor, cmd);
    }
    return;
  }

  // Phase 3: prefer the GPU path; fall back to the CPU rasteriser so
  // the editor overlay is always visible, even on devices where the
  // GL renderer can't come up.
  EditorGl& gl = sharedGl();
  if (gl.ready()) {
    gl.paint(cmds, image.width, image.height);
    // The GPU path paints directly into the surface framebuffer, so
    // there's no need to composite on the CPU side. Fall back to the
    // raster path for now (Phase 4 wires the GPU output into the
    // captured image instead of the swap-chain).
  }
  rasteriseOver(cmds, image);

  for (const UiCommand& cmd : ctx.commands) {
    applyCommand(&editor, cmd);
  }
}

}  // namespace kimia::ui

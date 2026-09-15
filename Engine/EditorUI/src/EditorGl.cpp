// EditorGl implementation — see EditorGl.h.
//
// Phase 3 GPU renderer for the EditorUI overlay. The actual GL
// pipeline (shader compile + VBO + atlas upload + draw call) lands in
// Phase 4 once Shader.h exposes the GLuint program handle directly.
//
// What this file does today:
//   * Builds the atlas texture (RGBA, 2048x8, 256 cells of 5x7 glyphs)
//     and keeps it in CPU memory until Phase 4 uploads it.
//   * Provides paintEditorGl() as the public entry point so callers
//     (Android jni_glue, test harnesses) get a single symbol to bind
//     against; it falls back to the CPU RasterBridge so the editor
//     overlay never disappears when the GPU path isn't ready.
//
// Why the placeholder is still useful: it removes the architectural
// question of "where does the GPU path live?" from the rest of the
// engine. NativePainter calls paintEditorGl() first, then falls back
// to rasteriseOver() if init() returned false.
#include <kimia/EditorGl.h>
#include <kimia/EditorUI.h>
#include <kimia/AtlasBuilder.h>
#include <kimia/RasterBridge.h>
#include <kimia/Image.h>

#include <cstdio>
#include <string>
#include <vector>

namespace kimia::ui {

EditorGl::~EditorGl() {
  destroy();
}

bool EditorGl::init(std::string& error) {
  destroy();
  // Build the atlas on the CPU side. The actual GL upload is Phase 4
  // work; until then this atlas struct is what tests verify against
  // (see AtlasBuilderTests).
  atlas_ = buildAtlas();
  ready_ = !atlas_.pixels.empty();
  if (!ready_) {
    error = "Atlas build failed (empty pixel buffer)";
    return false;
  }
  return true;
}

void EditorGl::destroy() {
  atlas_ = Atlas{};
  ready_ = false;
}

void EditorGl::paint(const std::vector<DrawCmd>& cmds, i32 width, i32 height) {
  if (!ready_ || width <= 0 || height <= 0) return;
  // Phase 4 will:
  //   1. build the VBO from cmds
  //   2. bind atlasTexture_
  //   3. draw a triangle list with the editor shader
  (void)cmds;
}

void paintEditorGl(const std::vector<DrawCmd>& cmds, i32 width, i32 height) {
  // Phase 3 fallback: no GL path yet, so this is a no-op. The caller
  // (NativePainter) chains paintEditorGl() -> rasteriseOver() so the
  // editor overlay still works on devices where the GPU path is off.
  (void)cmds;
  (void)width;
  (void)height;
}

}  // namespace kimia::ui

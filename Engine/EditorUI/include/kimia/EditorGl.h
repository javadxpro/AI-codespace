// EditorGl — the GPU path for the EditorUI overlay.
//
// Phase 3 replaces the CPU rasteriser with a real GLES3 renderer. The
// host calls paintEditorGl() after taking the DrawCmd list out of the
// EditorUI buffer; the function batches all rectangles + glyphs into a
// single draw call against a fragment shader that does rounded-rect
// SDF + atlas sampling for glyphs.
//
// The renderer owns its own shader, vertex buffer, and atlas texture.
// Initialise once, call paintEditorGl() every frame, destroy() on
// surface loss. The class is safe to live across surface recreations:
// reinit() resets the GPU state.
#pragma once

#include "EditorUI.h"
#include "AtlasBuilder.h"

#include <vector>

namespace kimia::ui {

class EditorGl {
public:
  EditorGl() = default;
  ~EditorGl();
  EditorGl(const EditorGl&) = delete;
  EditorGl& operator=(const EditorGl&) = delete;

  // Compile shaders, allocate the atlas texture, set up the vertex
  // buffer. Returns false on GL error (logged). After init() succeeds
  // the renderer is ready to accept paint calls.
  bool init(std::string& error);

  // Tear down all GL state. Safe to call multiple times.
  void destroy();

  // Paint a batch of DrawCmds onto the current framebuffer. The caller
  // is responsible for binding the right framebuffer + viewport (the
  // Android jni_glue sets these up before paintEditorGl).
  void paint(const std::vector<DrawCmd>& cmds, i32 width, i32 height);

  bool ready() const { return ready_; }

private:
  bool ready_ = false;
  Atlas atlas_{};
  std::uint32_t program_ = 0;
  std::uint32_t vao_ = 0;
  std::uint32_t vbo_ = 0;
  std::uint32_t atlasTexture_ = 0;
  // Cached uniform locations.
  int uViewport_ = -1;
  int uRectRect_ = -1;
  int uRectCorner_ = -1;
  int uRectColor_ = -1;
  int uGlyphRect_ = -1;
  int uGlyphUV_ = -1;
  int uGlyphColor_ = -1;
  int uIsGlyph_ = -1;
};

// Free function for the host: paint the EditorUI's draw command list
// using the GPU path. Falls back to the CPU rasteriser if the renderer
// failed to initialise, so callers can always call this unconditionally.
void paintEditorGl(const std::vector<DrawCmd>& cmds, i32 width, i32 height);

}  // namespace kimia::ui

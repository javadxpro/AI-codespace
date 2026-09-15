// EditorRasterBridge — turns EditorUI draw commands into an RGBA image
// that the host can composite on top of the rendered scene (either via
// ANativeWindow blit for the software path or a GL texture upload for
// the GPU path).
//
// The rasteriser is intentionally simple:
//   * rounded rectangles: alpha-blended fill with optional corner radius
//   * glyphs: sampled from the same atlas the GL path will eventually
//     use, so the editor text looks identical either way
//
// Both paths consume the same DrawCmd list, so the CPU raster output
// matches what the GL shader will draw once it's wired up.
#pragma once

#include "EditorUI.h"
#include "Widget.h"

#include <cstdint>
#include <string>
#include <vector>

namespace kimia {
struct Image;  // forward decl from Graphics/Image.h
}

namespace kimia::ui {

// Internal hook exposed for the raster bridge: snapshot the current
// DrawCmd list and clear it. The list is a frame-local buffer owned by
// EditorUI; this just lets the raster bridge consume it without dragging
// in the implementation headers.
std::vector<DrawCmd> takeDrawCmds();

// Rasterise the supplied draw commands into an image. The image is the
// same size as the surface (see resize()). Existing pixels are preserved
// (we draw the UI on top of the rendered scene).
void rasteriseInto(const std::vector<DrawCmd>& cmds, ::kimia::Image& image);

// Helper: paint a single rounded rect into the image. Exposed for tests.
void paintRoundedRect(::kimia::Image& image, const Rect& r, const Color& color,
                      f32 corner);

// Helper: paint a single glyph from the atlas at (x, y) in surface coords.
void paintGlyph(::kimia::Image& image, char ascii, f32 x, f32 y, i32 scale,
                const Color& color);

// Rasterise commands into a temporary RGBA buffer of the same size as
// `target`, then alpha-composite the result back onto `target`.
// Used by the Android jni_glue when the underlying renderer only produced
// an RGB frame (the software path); without this we'd silently drop the
// editor overlay on devices that can't keep GLES3 up.
void rasteriseOver(const std::vector<DrawCmd>& cmds, ::kimia::Image& target);

}  // namespace kimia::ui

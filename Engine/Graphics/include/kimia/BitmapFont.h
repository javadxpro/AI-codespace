#pragma once

#include <kimia/Image.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <string>

namespace kimia {

// A tiny built-in 5x7 bitmap font (printable ASCII 32..126) drawn straight
// into an Image — the on-frame HUD path that works everywhere the software
// rasterizer works (no GL, no font files, no system fonts). Anything outside
// ASCII is drawn as a hollow box so a missing glyph is visible, not silent.
//
// Coordinates are pixels, origin top-left; `scale` repeats every glyph pixel
// scale x scale times (2 = 10x14 glyphs, readable on a phone at 640x480).
namespace font {

inline constexpr i32 kGlyphWidth = 5;
inline constexpr i32 kGlyphHeight = 7;
inline constexpr i32 kGlyphAdvance = 6;  // 5 columns + 1 blank column

// The 7 row bitmasks (bit 4 = leftmost column) of a glyph; ' ' for unknown.
const u8* glyphRows(char c);

// Width in pixels of `text` at `scale` (no trailing gap after the last glyph).
i32 textWidth(const std::string& text, i32 scale = 1);
inline i32 textHeight(i32 scale = 1) { return kGlyphHeight * scale; }

// Draws `text` with its top-left corner at (x, y). Pixels outside the image
// are skipped; `color` is written as-is (0..1 -> 0..255, no shading).
// Returns the number of glyphs drawn (clipped ones included).
i32 drawText(Image& image, i32 x, i32 y, const std::string& text, const Vec3& color, i32 scale = 1);

// A filled rectangle blended over the image (alpha 0..1) — the HUD backdrop.
void fillRect(Image& image, i32 x, i32 y, i32 width, i32 height, const Vec3& color, f64 alpha);

// A horizontal bar: `fraction` (0..1) of `width` filled with `fill`, the rest
// with `back` — the power / charge meter.
void drawBar(Image& image, i32 x, i32 y, i32 width, i32 height, f64 fraction, const Vec3& fill, const Vec3& back);

}  // namespace font
}  // namespace kimia

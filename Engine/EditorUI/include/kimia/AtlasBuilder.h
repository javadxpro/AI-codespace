// AtlasBuilder — packs every glyph the EditorUI can render (the 5x7
// ASCII range plus the extended icon set) into one contiguous RGBA
// image plus a small per-glyph lookup table. The GL path uploads this
// texture once and samples it for every draw call; the CPU path keeps
// the same layout so positions match across both backends.
//
// Layout: a single row of cells, each cell is 8 pixels wide (1 pixel
// padding on every side) and 9 pixels tall (1 row padding) so a 5x7
// glyph with 1px padding on all sides fits cleanly. Cells are laid
// out left-to-right; the cell index in the row is the glyph's atlas
// column. Glyphs are 1bpp alpha — we expand to RGBA at build time.
#pragma once

#include "Icons.h"

#include <kimia/Types.h>

#include <array>
#include <cstdint>
#include <vector>

namespace kimia::ui {

struct AtlasCell {
  f32 u0 = 0.0f, v0 = 0.0f;   // top-left in [0,1] texture space
  f32 u1 = 0.0f, v1 = 0.0f;   // bottom-right
  i32 width = 0;                // cell width in pixels
  i32 height = 0;               // cell height in pixels
};

struct Atlas {
  std::vector<std::uint8_t> pixels;   // RGBA, row-major, alpha-only in practice
  i32 width = 0;
  i32 height = 0;
  // Indexed by the glyph code (ASCII 0..255); cells beyond 0x9E are
  // empty (cell = 0..0).
  std::array<AtlasCell, 256> cells{};
  static constexpr i32 kCellSize = 8;  // definition (inline since C++17)    // 5 + 3 padding
  static constexpr i32 kGlyphW = 5;
  static constexpr i32 kGlyphH = 7;
  static constexpr i32 kPadX = 1;        // padding inside cell
  static constexpr i32 kPadY = 1;
};

// Build the atlas. The same code is used by the GL renderer (to upload
// the texture) and by the CPU rasteriser (to look up glyph positions),
// so the result is the canonical glyph layout for the whole editor.
Atlas buildAtlas();

}  // namespace kimia::ui

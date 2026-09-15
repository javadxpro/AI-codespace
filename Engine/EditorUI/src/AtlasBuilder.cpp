// AtlasBuilder — see AtlasBuilder.h for the layout.
//
// One row of cells, each cell kCellSize pixels wide and tall (5x7 glyph
// + 3-pixel padding). The cell's top-left in [0,1] UV space is what
// the GL renderer uploads into the vertex buffer per glyph. The CPU
// rasteriser doesn't sample the texture, but it does use the same cell
// positions so visual layout matches the GL path bit-for-bit.
#include <kimia/AtlasBuilder.h>
#include <kimia/EditorFont.h>

#include <cstring>

namespace kimia::ui {

namespace {

// Fill an AtlasCell from a 5x7 bit row pattern.
void fillCellBits(std::uint8_t* dest, i32 destStride, const std::uint8_t bits[7]) {
  for (i32 y = 0; y < Atlas::kGlyphH; ++y) {
    const std::uint8_t row = bits[y];
    u8* dstRow = dest + y * destStride;
    for (i32 x = 0; x < Atlas::kGlyphW; ++x) {
      // Bit 4 = leftmost, bit 0 = rightmost.
      const bool set = (row >> (Atlas::kGlyphW - 1 - x)) & 1u;
      dstRow[x] = set ? 255 : 0;
    }
  }
}

void clearCell(std::uint8_t* dest, i32 destStride) {
  for (i32 y = 0; y < Atlas::kCellSize; ++y) {
    std::memset(dest + y * destStride, 0,
                static_cast<usize>(Atlas::kCellSize));
  }
}

}  // namespace

Atlas buildAtlas() {
  Atlas atlas;
  // 256 cells (0..255) covers both ASCII and the 0x80..0x9E icon range.
  atlas.width = Atlas::kCellSize * 256;
  atlas.height = Atlas::kCellSize;
  atlas.pixels.assign(static_cast<usize>(atlas.width) *
                          static_cast<usize>(atlas.height) * 4u,
                      0u);

  for (i32 code = 0; code < 256; ++code) {
    const i32 cellX = code * Atlas::kCellSize;
    const f32 u0 = static_cast<f32>(cellX) / static_cast<f32>(atlas.width);
    const f32 u1 = static_cast<f32>(cellX + Atlas::kCellSize) /
                   static_cast<f32>(atlas.width);
    atlas.cells[static_cast<usize>(code)] = AtlasCell{
        u0, 0.0f, u1, 1.0f,
        Atlas::kCellSize, Atlas::kCellSize,
    };

    u8* cell = atlas.pixels.data() +
               static_cast<usize>(cellX) * 4u;
    clearCell(cell, Atlas::kCellSize * 4);

    // Padded inner area for the actual glyph.
    u8* inner = cell + Atlas::kPadY * (Atlas::kCellSize * 4) +
                Atlas::kPadX * 4;
    std::uint8_t bits[Atlas::kGlyphH] = {0};

    if (code >= 0x20 && code <= 0x7E) {
      // ASCII font: row index = code - 0x20.
      const i32 row = code - 0x20;
      for (i32 y = 0; y < Atlas::kGlyphH; ++y) {
        bits[y] = kFont5x7[row][y];
      }
      fillCellBits(inner, Atlas::kCellSize * 4, bits);
    } else if (code >= 0x80) {
      // Extended icon range. Bits go into the alpha channel via the
      // glyphBitmap API; the GL renderer samples alpha.
      if (glyphBitmap(static_cast<Glyph>(code), bits)) {
        fillCellBits(inner, Atlas::kCellSize * 4, bits);
      }
    }
  }

  return atlas;
}

}  // namespace kimia::ui

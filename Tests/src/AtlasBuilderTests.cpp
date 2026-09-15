// AtlasBuilder tests — Phase 3.
//
// Verifies that the glyph atlas has the right dimensions, that ASCII
// glyphs and extended icons are placed into the right cells, and that
// the UV coordinates line up with the texture.
#include <kimia_test.h>
#include <kimia/AtlasBuilder.h>
#include <kimia/Icons.h>
#include <kimia/EditorFont.h>
#include <kimia/Types.h>

KIMIA_TEST(Atlas_HasExpectedDimensions) {
  auto atlas = kimia::ui::buildAtlas();
  // 256 cells * 8 pixels = 2048 pixels wide; 8 pixels tall.
  KIMIA_REQUIRE(atlas.width == 2048);
  KIMIA_REQUIRE(atlas.height == 8);
  // RGBA so 4 bytes per pixel.
  KIMIA_REQUIRE(atlas.pixels.size() ==
                static_cast<size_t>(atlas.width) *
                static_cast<size_t>(atlas.height) * 4u);
}

KIMIA_TEST(Atlas_AsciiCellHasGlyph) {
  // '#' is code 0x23, cell index 0x23. Its 5x7 bitmap has at least one
  // pixel set. Sample a known cell position.
  auto atlas = kimia::ui::buildAtlas();
  const int code = '#';
  const int cellX = code * kimia::ui::Atlas::kCellSize;
  // Glyph pixels live inside the padded area: row kPadY, col kPadX..kPadX+5
  // (the alpha byte is what we painted).
  bool any = false;
  for (int y = 0; y < 7; ++y) {
    const size_t base = static_cast<size_t>(
        (y + kimia::ui::Atlas::kPadY) * atlas.width + cellX +
        kimia::ui::Atlas::kPadX) * 4u;
    for (int x = 0; x < 5; ++x) {
      // Alpha is the 4th byte.
      if (atlas.pixels[base + static_cast<size_t>(x) * 4u + 3u] == 255) {
        any = true;
        break;
      }
    }
    if (any) break;
  }
  KIMIA_REQUIRE(any);
}

KIMIA_TEST(Atlas_ExtendedGlyphCellHasPixels) {
  // Glyph::ToolSelect = 0x8F. Its bitmap has at least one pixel set.
  auto atlas = kimia::ui::buildAtlas();
  const int code = static_cast<int>(kimia::ui::Glyph::ToolSelect);
  const int cellX = code * kimia::ui::Atlas::kCellSize;
  bool any = false;
  for (int y = 0; y < 7; ++y) {
    const size_t base = static_cast<size_t>(
        (y + kimia::ui::Atlas::kPadY) * atlas.width + cellX +
        kimia::ui::Atlas::kPadX) * 4u;
    for (int x = 0; x < 5; ++x) {
      if (atlas.pixels[base + static_cast<size_t>(x) * 4u + 3u] == 255) {
        any = true;
        break;
      }
    }
    if (any) break;
  }
  KIMIA_REQUIRE(any);
}

KIMIA_TEST(Atlas_UnusedCellIsEmpty) {
  // Code 0x00 (NUL) is unused; its 5x7 row is all zeros, so the inner
  // padded area should be entirely alpha=0.
  auto atlas = kimia::ui::buildAtlas();
  const int cellX = 0;
  for (int y = 0; y < 7; ++y) {
    const size_t base = static_cast<size_t>(
        (y + kimia::ui::Atlas::kPadY) * atlas.width + cellX +
        kimia::ui::Atlas::kPadX) * 4u;
    for (int x = 0; x < 5; ++x) {
      KIMIA_REQUIRE(atlas.pixels[base + static_cast<size_t>(x) * 4u + 3u] == 0);
    }
  }
}

KIMIA_TEST(Atlas_CellUvsAreMonotonic) {
  // Cell UVs increase strictly from cell 0 to cell 255. The GL renderer
  // relies on this for its vertex buffer.
  auto atlas = kimia::ui::buildAtlas();
  for (int i = 1; i < 256; ++i) {
    KIMIA_REQUIRE(atlas.cells[i].u0 > atlas.cells[i - 1].u0);
    KIMIA_REQUIRE(atlas.cells[i].u0 < atlas.cells[i].u1);
    KIMIA_REQUIRE(atlas.cells[i].u1 <= 1.0f);
  }
}

// Glyphs that aren't in the standard ASCII range — tool icons, media buttons,
// tree expand/collapse markers. Stored as 5x7 bitmaps next to the font.
#pragma once

#include <cstdint>

namespace kimia::ui {

// Identifier for a UI glyph. Codes >= 0x80 live in this enum; the standard
// font covers 0x20..0x7E.
enum class Glyph : std::uint16_t {
  CaretRight   = 0x80,  // ► collapsed tree
  CaretDown    = 0x81,  // ▼ expanded tree
  Dot          = 0x82,  // •
  Cross        = 0x83,  // × close
  Plus         = 0x84,  // +
  Minus        = 0x85,  // −
  Check        = 0x86,  // ✓
  Eye          = 0x87,  // visible
  EyeOff       = 0x88,  // hidden
  Lock         = 0x89,  // reserved name
  Cube         = 0x8A,
  Sphere       = 0x8B,
  Plane        = 0x8C,
  Player       = 0x8D,
  Ball         = 0x8E,
  ToolSelect   = 0x8F,
  ToolMove     = 0x90,
  ToolRotate   = 0x91,
  ToolScale    = 0x92,
  Play         = 0x93,
  Pause        = 0x94,
  Step         = 0x95,
  Stop         = 0x96,
  Save         = 0x97,
  Open         = 0x98,
  Trash        = 0x99,
  Duplicate    = 0x9A,
  Settings     = 0x9B,
  Search       = 0x9C,
  Filter       = 0x9D,
  Menu         = 0x9E,
  Last,
};

// Get the 5x7 bitmap (row-major, top-to-bottom, bit 4 = leftmost) for a
// glyph code. Returns false for unsupported codes.
bool glyphBitmap(Glyph g, std::uint8_t out[7]);

// Convenience: returns the bitmap for an ASCII char OR an extended Glyph.
bool glyphBitmapForChar(char c, std::uint8_t out[7]);

}  // namespace kimia::ui

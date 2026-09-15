#include <kimia/Icons.h>
#include <kimia/EditorUI.h>

namespace kimia::ui {

// 5x7 bitmaps for icons. Bit 4 = leftmost pixel of each row.
// Most icons are simple geometric shapes so the engine's 5x7 font can
// carry them without needing a separate atlas.

namespace {

constexpr std::uint8_t kCaretRight[7] = {
  0b00100, 0b01100, 0b11100, 0b11110, 0b11100, 0b01100, 0b00100,
};
constexpr std::uint8_t kCaretDown[7] = {
  0b00000, 0b00000, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000,
};
constexpr std::uint8_t kDot[7] = {
  0b00000, 0b00000, 0b01100, 0b01100, 0b00000, 0b00000, 0b00000,
};
constexpr std::uint8_t kCross[7] = {
  0b00000, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b00000,
};
constexpr std::uint8_t kPlus[7] = {
  0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000,
};
constexpr std::uint8_t kMinus[7] = {
  0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000,
};
constexpr std::uint8_t kCheck[7] = {
  0b00000, 0b00001, 0b00011, 0b10110, 0b11100, 0b01000, 0b00000,
};
constexpr std::uint8_t kEye[7] = {
  0b00000, 0b01110, 0b11111, 0b11111, 0b11111, 0b01110, 0b00000,
};
constexpr std::uint8_t kEyeOff[7] = {
  0b00000, 0b11010, 0b11101, 0b11111, 0b10111, 0b01111, 0b00000,
};
constexpr std::uint8_t kLock[7] = {
  0b01110, 0b10001, 0b10001, 0b11111, 0b11011, 0b11011, 0b11111,
};
constexpr std::uint8_t kCube[7] = {
  0b00000, 0b01110, 0b11111, 0b11011, 0b11111, 0b01110, 0b00000,
};
constexpr std::uint8_t kSphere[7] = {
  0b00000, 0b01110, 0b11111, 0b11011, 0b11111, 0b01110, 0b00000,
};
constexpr std::uint8_t kPlane[7] = {
  0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000,
};
constexpr std::uint8_t kPlayer[7] = {
  0b01110, 0b01110, 0b00100, 0b11111, 0b00100, 0b01010, 0b10001,
};
constexpr std::uint8_t kBall[7] = {
  0b00000, 0b01110, 0b11111, 0b11011, 0b11111, 0b01110, 0b00000,
};
constexpr std::uint8_t kToolSelect[7] = {
  0b00010, 0b00110, 0b01110, 0b11110, 0b01110, 0b00110, 0b00010,
};
constexpr std::uint8_t kToolMove[7] = {
  0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000, 0b00000,
};
constexpr std::uint8_t kToolRotate[7] = {
  0b00000, 0b01110, 0b10001, 0b10101, 0b10001, 0b01110, 0b00000,
};
constexpr std::uint8_t kToolScale[7] = {
  0b00000, 0b00100, 0b01110, 0b11111, 0b01110, 0b00100, 0b00000,
};
constexpr std::uint8_t kPlay[7] = {
  0b00010, 0b00110, 0b01110, 0b11110, 0b01110, 0b00110, 0b00010,
};
constexpr std::uint8_t kPause[7] = {
  0b00000, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b00000,
};
constexpr std::uint8_t kStep[7] = {
  0b00000, 0b10010, 0b10110, 0b11110, 0b10110, 0b10010, 0b00000,
};
constexpr std::uint8_t kStop[7] = {
  0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000,
};
constexpr std::uint8_t kSave[7] = {
  0b11111, 0b10001, 0b10101, 0b10101, 0b10001, 0b10001, 0b11111,
};
constexpr std::uint8_t kOpen[7] = {
  0b00000, 0b00110, 0b01111, 0b11110, 0b11111, 0b01110, 0b00000,
};
constexpr std::uint8_t kTrash[7] = {
  0b11111, 0b10001, 0b11111, 0b11111, 0b11011, 0b11011, 0b11111,
};
constexpr std::uint8_t kDuplicate[7] = {
  0b01110, 0b11111, 0b11011, 0b11111, 0b11111, 0b01110, 0b00000,
};
constexpr std::uint8_t kSettings[7] = {
  0b00000, 0b01110, 0b11111, 0b10101, 0b11111, 0b01110, 0b00000,
};
constexpr std::uint8_t kSearch[7] = {
  0b01110, 0b10001, 0b10001, 0b01110, 0b00000, 0b00100, 0b00000,
};
constexpr std::uint8_t kFilter[7] = {
  0b11111, 0b00100, 0b00100, 0b01110, 0b01110, 0b11111, 0b00000,
};
constexpr std::uint8_t kMenu[7] = {
  0b00000, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00000,
};

struct Entry {
  Glyph code;
  const std::uint8_t* bits;
};

constexpr Entry kTable[] = {
    {Glyph::CaretRight, kCaretRight},
    {Glyph::CaretDown, kCaretDown},
    {Glyph::Dot, kDot},
    {Glyph::Cross, kCross},
    {Glyph::Plus, kPlus},
    {Glyph::Minus, kMinus},
    {Glyph::Check, kCheck},
    {Glyph::Eye, kEye},
    {Glyph::EyeOff, kEyeOff},
    {Glyph::Lock, kLock},
    {Glyph::Cube, kCube},
    {Glyph::Sphere, kSphere},
    {Glyph::Plane, kPlane},
    {Glyph::Player, kPlayer},
    {Glyph::Ball, kBall},
    {Glyph::ToolSelect, kToolSelect},
    {Glyph::ToolMove, kToolMove},
    {Glyph::ToolRotate, kToolRotate},
    {Glyph::ToolScale, kToolScale},
    {Glyph::Play, kPlay},
    {Glyph::Pause, kPause},
    {Glyph::Step, kStep},
    {Glyph::Stop, kStop},
    {Glyph::Save, kSave},
    {Glyph::Open, kOpen},
    {Glyph::Trash, kTrash},
    {Glyph::Duplicate, kDuplicate},
    {Glyph::Settings, kSettings},
    {Glyph::Search, kSearch},
    {Glyph::Filter, kFilter},
    {Glyph::Menu, kMenu},
};

}  // namespace

bool glyphBitmap(Glyph g, std::uint8_t out[7]) {
  for (const Entry& e : kTable) {
    if (e.code == g) {
      for (int i = 0; i < 7; ++i) out[i] = e.bits[i];
      return true;
    }
  }
  return false;
}

bool glyphBitmapForChar(char c, std::uint8_t out[7]) {
  // Reserved codes 0x80..0x9E are icon glyphs.
  if (static_cast<unsigned char>(c) >= 0x80) {
    return glyphBitmap(static_cast<Glyph>(static_cast<unsigned char>(c)), out);
  }
  return false;
}

}  // namespace kimia::ui

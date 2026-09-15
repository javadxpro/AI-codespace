// RasterBridge unit tests — Phase 2.
//
// Verifies that the CPU rasteriser correctly converts DrawCmd lists
// into pixels on an Image. These tests don't touch GL; they're the
// regression baseline the host can use on any build machine, and they
// stay green while we wire up the GL path on top.
#include <kimia_test.h>
#include <kimia/EditorUI.h>
#include <kimia/RasterBridge.h>
#include <kimia/Image.h>
#include <kimia/Types.h>

namespace {

// Build a small RGBA image filled with transparent black.
kimia::Image makeImage(kimia::i32 w, kimia::i32 h) {
  kimia::Image img;
  img.width = w;
  img.height = h;
  img.channels = 4;
  img.pixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h) * 4u, 0u);
  return img;
}

kimia::u8 pixelAt(const kimia::Image& img, kimia::i32 x, kimia::i32 y) {
  return img.pixels[(static_cast<size_t>(y) * img.width + static_cast<size_t>(x)) *
                    static_cast<size_t>(img.channels) + 3];  // alpha
}

}  // namespace

KIMIA_TEST(RasterBridge_PaintRoundedRectFillsInterior) {
  auto img = makeImage(20, 20);
  kimia::ui::Color red{1.0f, 0.0f, 0.0f, 1.0f};
  kimia::ui::Rect r{2, 2, 16, 16};
  kimia::ui::paintRoundedRect(img, r, red, 0.0f);
  // Centre pixel should be fully painted.
  KIMIA_REQUIRE(pixelAt(img, 10, 10) == 255);
  // Pixel outside the rect should still be the original transparent black.
  KIMIA_REQUIRE(pixelAt(img, 0, 0) == 0);
}

KIMIA_TEST(RasterBridge_RasteriseRectClearsBuffer) {
  // After rasteriseInto with a single Rect, the rect's interior pixels
  // must be the rect's colour and the rest must be unchanged.
  auto img = makeImage(8, 8);
  kimia::ui::Color green{0.0f, 1.0f, 0.0f, 1.0f};
  std::vector<kimia::ui::DrawCmd> cmds;
  kimia::ui::DrawCmd dc;
  dc.kind = kimia::ui::DrawKind::Rect;
  dc.rect = {2, 2, 4, 4};
  dc.color = green;
  dc.corner = 0.0f;
  cmds.push_back(dc);
  kimia::ui::rasteriseInto(cmds, img);
  KIMIA_REQUIRE(pixelAt(img, 3, 3) == 255);
  KIMIA_REQUIRE(pixelAt(img, 1, 1) == 0);
}

KIMIA_TEST(RasterBridge_PaintGlyphMarksPixels) {
  // '#' has bits set across its body; after painting it, several pixels
  // inside its 5x7 cell should have alpha > 0.
  auto img = makeImage(8, 10);
  kimia::ui::Color white{1.0f, 1.0f, 1.0f, 1.0f};
  kimia::ui::paintGlyph(img, '#', 1.0f, 1.0f, 1, white);
  kimia::i32 hits = 0;
  for (kimia::i32 y = 1; y < 8; ++y) {
    for (kimia::i32 x = 1; x < 6; ++x) {
      if (pixelAt(img, x, y) > 0) ++hits;
    }
  }
  KIMIA_REQUIRE(hits > 10);
}

KIMIA_TEST(RasterBridge_EmptyCommandListIsNoOp) {
  auto img = makeImage(4, 4);
  std::vector<kimia::ui::DrawCmd> cmds;
  kimia::ui::rasteriseInto(cmds, img);
  KIMIA_REQUIRE(pixelAt(img, 2, 2) == 0);
}

KIMIA_TEST(RasterBridge_ClipsOffscreenRect) {
  // A rect that extends past the right and bottom edges must be clipped.
  auto img = makeImage(8, 8);
  kimia::ui::Color blue{0.0f, 0.0f, 1.0f, 1.0f};
  kimia::ui::Rect r{4, 4, 100, 100};
  kimia::ui::paintRoundedRect(img, r, blue, 0.0f);
  KIMIA_REQUIRE(pixelAt(img, 5, 5) == 255);
  // Pixel at (7, 7) is inside the rect and inside the image: still blue.
  KIMIA_REQUIRE(pixelAt(img, 7, 7) == 255);
  // Pixel at (0, 0) was never touched.
  KIMIA_REQUIRE(pixelAt(img, 0, 0) == 0);
}

KIMIA_TEST(RasterBridge_RasteriseOverCompositesOnRgbFrame) {
  // The software renderer only emits an RGB frame; the editor overlay has
  // to composite on top of that. rasteriseOver is the entry point that
  // owns that path — it must paint onto the RGB buffer without expecting
  // alpha storage in the destination.
  kimia::Image rgb;
  rgb.width = 4;
  rgb.height = 4;
  rgb.channels = 3;
  rgb.pixels.assign(static_cast<size_t>(rgb.width) *
                        static_cast<size_t>(rgb.height) * 3u, 0u);

  std::vector<kimia::ui::DrawCmd> cmds;
  kimia::ui::DrawCmd dc;
  dc.kind = kimia::ui::DrawKind::Rect;
  dc.rect = {0.0f, 0.0f, 4.0f, 4.0f};
  dc.color = kimia::ui::Color{1.0f, 0.0f, 0.0f, 1.0f};
  dc.corner = 0.0f;
  cmds.push_back(dc);

  kimia::ui::rasteriseOver(cmds, rgb);

  // Red square on top of black: red pixel in the centre.
  KIMIA_REQUIRE(rgb.pixels[0] > 200);   // R
  KIMIA_REQUIRE(rgb.pixels[1] < 50);    // G
  KIMIA_REQUIRE(rgb.pixels[2] < 50);    // B
}

KIMIA_TEST(RasterBridge_RasteriseOverKeepsUntouchedPixels) {
  // Pixels outside any rect must keep their original RGB value.
  kimia::Image rgb;
  rgb.width = 4;
  rgb.height = 4;
  rgb.channels = 3;
  rgb.pixels.assign(static_cast<size_t>(rgb.width) *
                        static_cast<size_t>(rgb.height) * 3u, 0u);
  // Pre-fill with green at every pixel.
  for (size_t i = 0; i < rgb.pixels.size(); i += 3) {
    rgb.pixels[i + 0] = 0;
    rgb.pixels[i + 1] = 200;
    rgb.pixels[i + 2] = 0;
  }

  std::vector<kimia::ui::DrawCmd> cmds;
  kimia::ui::DrawCmd dc;
  dc.kind = kimia::ui::DrawKind::Rect;
  dc.rect = {0.0f, 0.0f, 2.0f, 2.0f};   // top-left quadrant only
  dc.color = kimia::ui::Color{1.0f, 0.0f, 0.0f, 1.0f};
  dc.corner = 0.0f;
  cmds.push_back(dc);
  kimia::ui::rasteriseOver(cmds, rgb);

  // Top-left pixel: red.
  KIMIA_REQUIRE(rgb.pixels[0] > 200);
  KIMIA_REQUIRE(rgb.pixels[1] < 50);
  // Bottom-right pixel: green, untouched.
  const size_t br = (static_cast<size_t>(3) * rgb.width + 3) * 3;
  KIMIA_REQUIRE(rgb.pixels[br + 0] == 0);
  KIMIA_REQUIRE(rgb.pixels[br + 1] == 200);
  KIMIA_REQUIRE(rgb.pixels[br + 2] == 0);
}

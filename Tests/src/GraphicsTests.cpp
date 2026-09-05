#include <kimia/BitmapFont.h>
#include <kimia/GraphicsTypes.h>
#include <kimia/Image.h>
#include <kimia/Mesh.h>
#include <kimia/Vec.h>
#include <kimia_test.h>

#include <cmath>

namespace {
using kimia::Vec2;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::usize;

constexpr f64 kEps = 1e-9;

bool near3(const Vec3& a, const Vec3& b, f64 eps = kEps) {
  return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps && std::abs(a.z - b.z) <= eps;
}

bool isUnit(const Vec3& v, f64 eps = 1e-6) { return std::abs(v.length() - 1.0) <= eps; }

// Every triangle must be counter-clockwise seen from outside: the face normal
// must point the same way as its first vertex normal. Zero-area triangles
// (pole rings of the sphere grid) are skipped.
void requireOutwardWinding(const kimia::MeshData& mesh) {
  for (usize i = 0; i + 2U < mesh.indices.size(); i += 3U) {
    const Vec3 p0 = mesh.positions[mesh.indices[i]];
    const Vec3 p1 = mesh.positions[mesh.indices[i + 1U]];
    const Vec3 p2 = mesh.positions[mesh.indices[i + 2U]];
    const Vec3 faceNormal = kimia::cross(p1 - p0, p2 - p0);
    if (faceNormal.lengthSquared() < 1e-24) continue;
    const Vec3 vertexNormal = mesh.normals[mesh.indices[i]];
    KIMIA_REQUIRE(kimia::dot(faceNormal, vertexNormal) > 0.0);
  }
}
}  // namespace

KIMIA_TEST(cube_24v_36i_with_per_face_normals) {
  const kimia::MeshData cube = kimia::makeCube(2.0);
  KIMIA_REQUIRE(cube.positions.size() == 24U);
  KIMIA_REQUIRE(cube.indices.size() == 36U);
  KIMIA_REQUIRE(cube.normals.size() == 24U);
  KIMIA_REQUIRE(cube.uvs.size() == 24U);
  KIMIA_REQUIRE(cube.isValid());
  // Corner positions lie exactly on the cube surface (half extent = 1).
  for (const Vec3& p : cube.positions) {
    KIMIA_REQUIRE(std::abs(std::abs(p.x) - 1.0) <= kEps || std::abs(p.x) <= kEps);
    KIMIA_REQUIRE(std::abs(std::abs(p.y) - 1.0) <= kEps || std::abs(p.y) <= kEps);
    KIMIA_REQUIRE(std::abs(std::abs(p.z) - 1.0) <= kEps || std::abs(p.z) <= kEps);
  }
  // Normals are unit axis vectors and equal the outward direction of the face.
  for (const Vec3& n : cube.normals) KIMIA_REQUIRE(isUnit(n));
  i32 frontFaceCount = 0;
  for (usize i = 0; i < cube.normals.size(); ++i) {
    const Vec3& n = cube.normals[i];
    KIMIA_REQUIRE(kimia::dot(n, cube.positions[i]) > 0.0);
    if (near3(n, Vec3{0.0, 0.0, -1.0})) ++frontFaceCount;
  }
  KIMIA_REQUIRE(frontFaceCount == 4);
  requireOutwardWinding(cube);
}

KIMIA_TEST(cube_uvs_cover_01) {
  const kimia::MeshData cube = kimia::makeCube(1.0);
  for (const Vec2& uv : cube.uvs) {
    KIMIA_REQUIRE(uv.x >= -kEps && uv.x <= 1.0 + kEps);
    KIMIA_REQUIRE(uv.y >= -kEps && uv.y <= 1.0 + kEps);
  }
}

KIMIA_TEST(sphere_reference_153v_768i_outward) {
  // The reference sphere (16 rings x 8 segments): 153 vertices / 768 indices.
  const kimia::MeshData sphere = kimia::makeSphere(16, 8);
  KIMIA_REQUIRE(sphere.positions.size() == 153U);
  KIMIA_REQUIRE(sphere.indices.size() == 768U);
  KIMIA_REQUIRE(sphere.normals.size() == 153U);
  KIMIA_REQUIRE(sphere.isValid());
  for (usize i = 0; i < sphere.positions.size(); ++i) {
    KIMIA_REQUIRE(isUnit(sphere.normals[i], 1e-5));
    // Outward: normal points away from the origin (dot > 0 everywhere).
    KIMIA_REQUIRE(kimia::dot(sphere.normals[i], sphere.positions[i]) > 0.0);
  }
  requireOutwardWinding(sphere);
}

KIMIA_TEST(sphere_vertex_count_formula) {
  // verts = (rings+1)*(segments+1), indices = rings*segments*6.
  const kimia::MeshData sphere = kimia::makeSphere(4, 6);
  KIMIA_REQUIRE(sphere.positions.size() == 35U);
  KIMIA_REQUIRE(sphere.indices.size() == 144U);
  requireOutwardWinding(sphere);
}

KIMIA_TEST(plane_4v_6i_faces_up) {
  const kimia::MeshData plane = kimia::makePlane(4.0, 2.0);
  KIMIA_REQUIRE(plane.positions.size() == 4U);
  KIMIA_REQUIRE(plane.indices.size() == 6U);
  KIMIA_REQUIRE(plane.isValid());
  for (const Vec3& p : plane.positions) KIMIA_REQUIRE(std::abs(p.y) <= kEps);
  for (const Vec3& n : plane.normals) KIMIA_REQUIRE(near3(n, Vec3{0.0, 1.0, 0.0}));
  requireOutwardWinding(plane);
}

KIMIA_TEST(mesh_data_validation_contract) {
  kimia::MeshData empty;
  KIMIA_REQUIRE(!empty.isValid());
  kimia::MeshData cube = kimia::makeCube(1.0);
  KIMIA_REQUIRE(cube.vertexCount() == 24U);
  KIMIA_REQUIRE(cube.triangleCount() == 12U);
  cube.indices.pop_back();
  KIMIA_REQUIRE(!cube.isValid());
}

// --- Bitmap font (the on-frame HUD) ---

namespace {
kimia::Image blankImage(i32 width, i32 height, kimia::u8 value) {
  kimia::Image image;
  image.width = width;
  image.height = height;
  image.channels = 3;
  image.pixels.assign(static_cast<usize>(width) * static_cast<usize>(height) * 3U, value);
  return image;
}

bool pixelIs(const kimia::Image& image, i32 x, i32 y, kimia::u8 r, kimia::u8 g, kimia::u8 b) {
  const kimia::u8* p = image.at(x, y);
  return p[0] == r && p[1] == g && p[2] == b;
}
}  // namespace

KIMIA_TEST(font_glyph_table_is_5x7_ascii_with_a_visible_box_for_the_rest) {
  namespace font = kimia::font;
  KIMIA_REQUIRE(font::kGlyphWidth == 5);
  KIMIA_REQUIRE(font::kGlyphHeight == 7);
  KIMIA_REQUIRE(font::kGlyphAdvance == 6);
  // 'A': a pointed top, a crossbar on row 3, open legs.
  const kimia::u8* a = font::glyphRows('A');
  KIMIA_REQUIRE(a[0] == 0x0E);
  KIMIA_REQUIRE(a[1] == 0x11);
  KIMIA_REQUIRE(a[3] == 0x1F);
  KIMIA_REQUIRE(a[6] == 0x11);
  // Space is blank; '!' is a dotted stem.
  for (i32 row = 0; row < 7; ++row) KIMIA_REQUIRE(font::glyphRows(' ')[row] == 0x00);
  KIMIA_REQUIRE(font::glyphRows('!')[5] == 0x00);
  KIMIA_REQUIRE(font::glyphRows('!')[6] == 0x04);
  // Every printable glyph fits in 5 columns (no bit above bit 4).
  for (i32 code = 32; code <= 126; ++code) {
    const kimia::u8* rows = font::glyphRows(static_cast<char>(code));
    for (i32 row = 0; row < 7; ++row) KIMIA_REQUIRE((rows[row] & 0xE0) == 0);
  }
  // Outside ASCII (a UTF-8 byte, a control char): the hollow box.
  const kimia::u8* box = font::glyphRows(static_cast<char>(0xD8));
  KIMIA_REQUIRE(box[0] == 0x1F);
  KIMIA_REQUIRE(box[3] == 0x11);
  KIMIA_REQUIRE(box[6] == 0x1F);
  KIMIA_REQUIRE(font::glyphRows('\n') == box);
  KIMIA_REQUIRE(font::glyphRows(static_cast<char>(127)) == box);
}

KIMIA_TEST(font_text_metrics_scale_without_a_trailing_gap) {
  namespace font = kimia::font;
  KIMIA_REQUIRE(font::textWidth("") == 0);
  KIMIA_REQUIRE(font::textWidth("A") == 5);
  KIMIA_REQUIRE(font::textWidth("AB") == 11);
  KIMIA_REQUIRE(font::textWidth("PAR 3") == 29);
  KIMIA_REQUIRE(font::textWidth("PAR 3", 2) == 58);
  KIMIA_REQUIRE(font::textWidth("PAR 3", 0) == 0);
  KIMIA_REQUIRE(font::textHeight() == 7);
  KIMIA_REQUIRE(font::textHeight(3) == 21);
}

KIMIA_TEST(font_draw_text_writes_exact_pixels_and_clips_at_the_edges) {
  namespace font = kimia::font;
  kimia::Image image = blankImage(16, 10, 0);
  // 'I' at (2, 1): row 0 = 0x0E -> columns 1..3 lit, column 0 and 4 dark.
  KIMIA_REQUIRE(font::drawText(image, 2, 1, "I", Vec3{1.0, 0.5, 0.0}) == 1);
  KIMIA_REQUIRE(pixelIs(image, 2, 1, 0, 0, 0));
  KIMIA_REQUIRE(pixelIs(image, 3, 1, 255, 128, 0));
  KIMIA_REQUIRE(pixelIs(image, 4, 1, 255, 128, 0));
  KIMIA_REQUIRE(pixelIs(image, 5, 1, 255, 128, 0));
  KIMIA_REQUIRE(pixelIs(image, 6, 1, 0, 0, 0));
  // Row 1 = 0x04 -> only the middle column.
  KIMIA_REQUIRE(pixelIs(image, 3, 2, 0, 0, 0));
  KIMIA_REQUIRE(pixelIs(image, 4, 2, 255, 128, 0));
  KIMIA_REQUIRE(pixelIs(image, 5, 2, 0, 0, 0));
  // Row 6 (y = 7) is the last drawn row; y = 8 stays dark.
  KIMIA_REQUIRE(pixelIs(image, 4, 7, 255, 128, 0));
  KIMIA_REQUIRE(pixelIs(image, 4, 8, 0, 0, 0));
  usize lit = 0;
  for (usize i = 0; i < image.pixels.size(); i += 3U) lit += image.pixels[i] == 255 ? 1U : 0U;
  KIMIA_REQUIRE(lit == 3U + 5U + 3U);  // 0x0E + 5 x 0x04 + 0x0E

  // Scale 2 doubles every glyph pixel; text partly off the right edge and
  // above the top is clipped, not written out of bounds, and still counted.
  kimia::Image scaled = blankImage(20, 20, 0);
  KIMIA_REQUIRE(font::drawText(scaled, 0, 0, "I", Vec3{1.0, 1.0, 1.0}, 2) == 1);
  KIMIA_REQUIRE(pixelIs(scaled, 2, 0, 255, 255, 255));
  KIMIA_REQUIRE(pixelIs(scaled, 3, 1, 255, 255, 255));
  KIMIA_REQUIRE(pixelIs(scaled, 0, 0, 0, 0, 0));
  KIMIA_REQUIRE(pixelIs(scaled, 4, 13, 255, 255, 255));
  KIMIA_REQUIRE(pixelIs(scaled, 4, 14, 0, 0, 0));
  kimia::Image tiny = blankImage(4, 4, 0);
  KIMIA_REQUIRE(font::drawText(tiny, -2, -3, "HUD", Vec3{1.0, 1.0, 1.0}, 3) == 3);
  KIMIA_REQUIRE(tiny.pixels.size() == 4U * 4U * 3U);
  kimia::Image empty;
  KIMIA_REQUIRE(font::drawText(empty, 0, 0, "x", Vec3{1.0, 1.0, 1.0}) == 0);
}

KIMIA_TEST(font_fill_rect_blends_and_bar_fills_a_fraction) {
  namespace font = kimia::font;
  kimia::Image image = blankImage(10, 6, 200);
  // Half-transparent black over grey 200 -> 100; outside the rect untouched.
  font::fillRect(image, 2, 1, 4, 3, Vec3{0.0, 0.0, 0.0}, 0.5);
  KIMIA_REQUIRE(pixelIs(image, 2, 1, 100, 100, 100));
  KIMIA_REQUIRE(pixelIs(image, 5, 3, 100, 100, 100));
  KIMIA_REQUIRE(pixelIs(image, 1, 1, 200, 200, 200));
  KIMIA_REQUIRE(pixelIs(image, 6, 1, 200, 200, 200));
  KIMIA_REQUIRE(pixelIs(image, 2, 4, 200, 200, 200));
  // A rect hanging off the image is clipped (no crash, edge pixels blended).
  font::fillRect(image, -5, -5, 7, 7, Vec3{1.0, 1.0, 1.0}, 1.0);
  KIMIA_REQUIRE(pixelIs(image, 0, 0, 255, 255, 255));
  KIMIA_REQUIRE(pixelIs(image, 1, 1, 255, 255, 255));
  KIMIA_REQUIRE(pixelIs(image, 2, 2, 100, 100, 100));
  // Bar: 25% of 8 px = 2 px fill, the rest back; fraction is clamped.
  kimia::Image bar = blankImage(10, 3, 0);
  font::drawBar(bar, 1, 1, 8, 1, 0.25, Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 0.0, 1.0});
  KIMIA_REQUIRE(pixelIs(bar, 1, 1, 255, 0, 0));
  KIMIA_REQUIRE(pixelIs(bar, 2, 1, 255, 0, 0));
  KIMIA_REQUIRE(pixelIs(bar, 3, 1, 0, 0, 255));
  KIMIA_REQUIRE(pixelIs(bar, 8, 1, 0, 0, 255));
  KIMIA_REQUIRE(pixelIs(bar, 9, 1, 0, 0, 0));
  KIMIA_REQUIRE(pixelIs(bar, 1, 0, 0, 0, 0));
  font::drawBar(bar, 1, 1, 8, 1, 7.0, Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 0.0, 1.0});
  KIMIA_REQUIRE(pixelIs(bar, 8, 1, 255, 0, 0));
  font::drawBar(bar, 1, 1, 8, 1, -1.0, Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 0.0, 1.0});
  KIMIA_REQUIRE(pixelIs(bar, 1, 1, 0, 0, 255));
}

// EditorRasterBridge implementation.
//
// Renders the DrawCmd list produced by EditorUI::draw() into an RGBA
// Image. This is the Phase 1 path: a pure CPU rasteriser that the host
// composites onto the surface (either by direct ANativeWindow blit, or
// by uploading as a GL texture once the GL pipeline lands).
//
// For Phase 1 the result looks identical to what the GL shader will
// eventually produce, since both paths consume the same DrawCmd list.
#include <kimia/RasterBridge.h>
#include <kimia/EditorFont.h>
#include <kimia/Image.h>
#include <kimia/Icons.h>
#include <kimia/EditorUI.h>
#include <kimia/Types.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace kimia::ui {

namespace {

// Same 5x7 font table as EditorUI.cpp's atlas builder, kept duplicated so
// the raster path stays self-contained. If this drifts from the engine
// font, the editor will look slightly different on CPU vs GL — we'll
// fix it once the GL path lands.

// Alpha-blend a single source pixel onto a destination pixel.
inline void blendPixel(u8* dst, f32 sr, f32 sg, f32 sb, f32 sa) {
  if (sa <= 0.0f) return;
  if (sa >= 1.0f) {
    dst[0] = static_cast<u8>(std::clamp(sr * 255.0f, 0.0f, 255.0f));
    dst[1] = static_cast<u8>(std::clamp(sg * 255.0f, 0.0f, 255.0f));
    dst[2] = static_cast<u8>(std::clamp(sb * 255.0f, 0.0f, 255.0f));
    dst[3] = 255;
    return;
  }
  const f32 da = dst[3] / 255.0f;
  const f32 outA = sa + da * (1.0f - sa);
  if (outA <= 0.0f) return;
  const f32 inv = 1.0f / outA;
  const f32 dr = dst[0] / 255.0f;
  const f32 dg = dst[1] / 255.0f;
  const f32 db = dst[2] / 255.0f;
  dst[0] = static_cast<u8>(std::clamp((sr * sa + dr * da * (1.0f - sa)) * inv * 255.0f,
                                      0.0f, 255.0f));
  dst[1] = static_cast<u8>(std::clamp((sg * sa + dg * da * (1.0f - sa)) * inv * 255.0f,
                                      0.0f, 255.0f));
  dst[2] = static_cast<u8>(std::clamp((sb * sa + db * da * (1.0f - sa)) * inv * 255.0f,
                                      0.0f, 255.0f));
  dst[3] = static_cast<u8>(std::clamp(outA * 255.0f, 0.0f, 255.0f));
}

// Inside-test for a rounded rect. Phase 1 doesn't anti-alias; that's
// acceptable because the editor overlay is mostly opaque panels.
inline bool insideRoundedRect(f32 px, f32 py, const Rect& r, f32 corner) {
  if (px < r.x || px >= r.right() || py < r.y || py >= r.bottom()) return false;
  if (corner <= 0.0f) return true;
  // Distance to the nearest corner centre.
  f32 dx = 0.0f, dy = 0.0f;
  if (px < r.x + corner) dx = (r.x + corner) - px;
  else if (px > r.right() - corner) dx = px - (r.right() - corner);
  if (py < r.y + corner) dy = (r.y + corner) - py;
  else if (py > r.bottom() - corner) dy = py - (r.bottom() - corner);
  if (dx == 0.0f && dy == 0.0f) return true;  // inside the safe area
  return (dx * dx + dy * dy) <= corner * corner;
}

}  // namespace

std::vector<DrawCmd> takeDrawCmds() {
  return ::kimia::ui::takeDrawCmds();
}

void rasteriseInto(const std::vector<DrawCmd>& cmds, ::kimia::Image& image) {
  if (cmds.empty()) return;
  if (image.isEmpty() || image.channels != 4) return;
  u8* base = image.pixels.data();
  const i32 ch = image.channels;
  for (const DrawCmd& dc : cmds) {
    if (dc.kind == DrawKind::Rect) {
      const i32 x0 = std::max<i32>(0, static_cast<i32>(dc.rect.x));
      const i32 y0 = std::max<i32>(0, static_cast<i32>(dc.rect.y));
      const i32 x1 = std::min<i32>(image.width,
                                   static_cast<i32>(dc.rect.right()));
      const i32 y1 = std::min<i32>(image.height,
                                   static_cast<i32>(dc.rect.bottom()));
      for (i32 y = y0; y < y1; ++y) {
        u8* row = base + (static_cast<usize>(y) *
                          static_cast<usize>(image.width)) *
                          static_cast<usize>(ch);
        for (i32 x = x0; x < x1; ++x) {
          if (!insideRoundedRect(static_cast<f32>(x) + 0.5f,
                                 static_cast<f32>(y) + 0.5f,
                                 dc.rect, dc.corner)) continue;
          blendPixel(row + x * ch, dc.color.r, dc.color.g, dc.color.b,
                     dc.color.a);
        }
      }
    } else {
      // For now Phase 2 rasterises only the ASCII path: extended Glyphs
      // (tool icons, tree carets) will be added once we move to the GL
      // shader path that already knows how to look them up via glyphBitmap.
      const i32 code = static_cast<unsigned char>(dc.ascii);
      if (code < 0x20 || code > 0x7E) continue;
      const i32 col = code - 0x20;
      const i32 scale = std::clamp<i32>(dc.scale, 1, 3);
      const f32 baseX = dc.rect.x;
      const f32 baseY = dc.rect.y;
      for (i32 gy = 0; gy < 7; ++gy) {
        const std::uint8_t bits = kFont5x7[col][gy];
        for (i32 gx = 0; gx < 5; ++gx) {
          if (!((bits >> (4 - gx)) & 1)) continue;
          for (i32 sy = 0; sy < scale; ++sy) {
            for (i32 sx = 0; sx < scale; ++sx) {
              const i32 ix = static_cast<i32>(baseX) + gx * scale + sx;
              const i32 iy = static_cast<i32>(baseY) + gy * scale + sy;
              if (ix < 0 || iy < 0 ||
                  ix >= image.width || iy >= image.height) continue;
              u8* dst = base +
                        (static_cast<usize>(iy) * image.width +
                         static_cast<usize>(ix)) * static_cast<usize>(ch);
              blendPixel(dst, dc.color.r, dc.color.g, dc.color.b,
                         dc.color.a);
            }
          }
        }
      }
    }
  }
}

void paintRoundedRect(::kimia::Image& image, const Rect& r, const Color& color,
                      f32 corner) {
  DrawCmd dc;
  dc.kind = DrawKind::Rect;
  dc.rect = r;
  dc.color = color;
  dc.corner = corner;
  std::vector<DrawCmd> v{dc};
  rasteriseInto(v, image);
}

void paintGlyph(::kimia::Image& image, char ascii, f32 x, f32 y, i32 scale,
                const Color& color) {
  if (image.isEmpty() || image.channels != 4) return;
  if (ascii < 0x20 || ascii > 0x7E) return;
  DrawCmd dc;
  dc.kind = DrawKind::Glyph;
  dc.rect = {x, y, 5.0f * scale, 7.0f * scale};
  dc.color = color;
  dc.ascii = ascii;
  dc.scale = scale;
  std::vector<DrawCmd> v{dc};
  rasteriseInto(v, image);
}

void rasteriseOver(const std::vector<DrawCmd>& cmds, ::kimia::Image& target) {
  if (cmds.empty() || target.isEmpty()) return;
  // Build a same-sized RGBA overlay; raster the editor onto it; then
  // alpha-composite onto the target. The target keeps its RGB (the scene
  // frame) and gains the editor's translucent panels.
  ::kimia::Image overlay;
  overlay.width = target.width;
  overlay.height = target.height;
  overlay.channels = 4;
  overlay.pixels.assign(static_cast<usize>(target.width) *
                            static_cast<usize>(target.height) * 4u,
                        0u);
  rasteriseInto(cmds, overlay);

  const usize count = static_cast<usize>(target.width) *
                      static_cast<usize>(target.height);
  const i32 tCh = target.channels;
  for (usize i = 0; i < count; ++i) {
    const usize o = i * 4u;
    const u8 oa = overlay.pixels[o + 3u];
    if (oa == 0) continue;  // editor didn't touch this pixel
    const f32 sa = oa / 255.0f;
    const f32 sr = overlay.pixels[o + 0u] / 255.0f;
    const f32 sg = overlay.pixels[o + 1u] / 255.0f;
    const f32 sb = overlay.pixels[o + 2u] / 255.0f;
    const usize t = i * static_cast<usize>(tCh);
    f32 dr = target.pixels[t + 0u] / 255.0f;
    f32 dg = target.pixels[t + 1u] / 255.0f;
    f32 db = target.pixels[t + 2u] / 255.0f;
    target.pixels[t + 0u] = static_cast<u8>(std::clamp((sr * sa + dr * (1.0f - sa)) * 255.0f,
                                                       0.0f, 255.0f));
    target.pixels[t + 1u] = static_cast<u8>(std::clamp((sg * sa + dg * (1.0f - sa)) * 255.0f,
                                                       0.0f, 255.0f));
    target.pixels[t + 2u] = static_cast<u8>(std::clamp((sb * sa + db * (1.0f - sa)) * 255.0f,
                                                       0.0f, 255.0f));
    if (tCh == 4) {
      const f32 da = target.pixels[t + 3u] / 255.0f;
      target.pixels[t + 3u] = static_cast<u8>(std::clamp((sa + da * (1.0f - sa)) * 255.0f,
                                                         0.0f, 255.0f));
    }
  }
}

}  // namespace kimia::ui

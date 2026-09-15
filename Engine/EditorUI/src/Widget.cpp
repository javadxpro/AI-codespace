// Widget implementation.
//
// Each widget call:
//   1. Computes its rect (either given or auto-laid-out)
//   2. Pushes one or more DrawCmds into the editor's draw list
//   3. Tests against live pointer state to detect press / drag / release
//   4. Returns interaction result to the caller
//
// Widgets never allocate; everything goes through Frame and the persistent
// widget-state map. Text input keeps its cursor in that map.
#include <kimia/EditorUI.h>
#include <kimia/Theme.h>
#include <kimia/Widget.h>
#include <kimia/Icons.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>

namespace kimia::ui {

// ---- Forward decls + free helpers (visible to Widget internals) ---------

void drawRect(const Rect& rect, const Color& color, f32 cornerRadius);

void drawGlyph(char ascii, f32 x, f32 y, i32 scale, const Color& color);
void drawGlyph(Glyph icon, f32 x, f32 y, i32 scale, const Color& color);

void drawRect(const Rect& rect, const Color& color, f32 cornerRadius) {
  DrawCmd dc;
  dc.kind = DrawKind::Rect;
  dc.rect = rect;
  dc.color = color;
  dc.corner = cornerRadius;
  internal::pushDrawCmd(dc);
}

void drawGlyph(char ascii, f32 x, f32 y, i32 scale, const Color& color) {
  DrawCmd dc;
  dc.kind = DrawKind::Glyph;
  dc.rect = {x, y, 5.0f * scale, 7.0f * scale};
  dc.color = color;
  dc.glyph = Glyph::CaretRight;  // sentinel: ascii path
  dc.ascii = ascii;
  dc.scale = scale;
  internal::pushDrawCmd(dc);
}

void drawGlyph(Glyph icon, f32 x, f32 y, i32 scale, const Color& color) {
  DrawCmd dc;
  dc.kind = DrawKind::Glyph;
  dc.rect = {x, y, 5.0f * scale, 7.0f * scale};
  dc.color = color;
  dc.glyph = icon;
  dc.scale = scale;
  internal::pushDrawCmd(dc);
}

void drawText(const char* text, f32 x, f32 y, i32 scale, const Color& color) {
  if (!text) return;
  const i32 s = std::max(1, std::min(scale, theme::kFontScaleMax));
  const f32 cw = 6.0f * s;
  for (i32 i = 0; text[i]; ++i) {
    if (text[i] >= 0x20) {
      drawGlyph(static_cast<char>(text[i]), x + i * cw, y, s, color);
    }
  }
}

}  // namespace kimia::ui

// ============================================================================
// Anonymous-namespace internals: emit helpers + frame state
// ============================================================================

namespace kimia::ui::detail {

Frame gFrame;
bool gFrameActive = false;
std::vector<Rect> gClipStack;

struct WidgetState {
  std::string text;
  i32 cursor = 0;
  f32 scrollOffset = 0.0f;
};
std::unordered_map<u64, WidgetState>& widgetState() {
  static std::unordered_map<u64, WidgetState> map;
  return map;
}
u64 hashId(const char* s) {
  u64 h = 14695981039346656037ULL;
  for (; *s; ++s) {
    h ^= static_cast<u8>(*s);
    h *= 1099511628211ULL;
  }
  return h;
}

void emitRect(const Rect& r, const Color& c, f32 corner) {
  Rect clipped = r;
  for (const Rect& clip : gClipStack) {
    clipped.x = std::max(clipped.x, clip.x);
    clipped.y = std::max(clipped.y, clip.y);
    clipped.w = std::max(0.0f, std::min(clipped.right(), clip.right()) - clipped.x);
    clipped.h = std::max(0.0f, std::min(clipped.bottom(), clip.bottom()) - clipped.y);
  }
  if (clipped.w <= 0.0f || clipped.h <= 0.0f) return;
  DrawCmd dc;
  dc.kind = DrawKind::Rect;
  dc.rect = clipped;
  dc.color = c;
  dc.corner = corner;
  internal::pushDrawCmd(dc);
}

void emitGlyph(char ascii, f32 x, f32 y, i32 scale, const Color& c) {
  if (ascii < 0x20) return;
  Rect r{x, y, static_cast<f32>(5 * scale), static_cast<f32>(7 * scale)};
  Rect clipped = r;
  for (const Rect& clip : gClipStack) {
    clipped.x = std::max(clipped.x, clip.x);
    clipped.y = std::max(clipped.y, clip.y);
    clipped.w = std::max(0.0f, std::min(clipped.right(), clip.right()) - clipped.x);
    clipped.h = std::max(0.0f, std::min(clipped.bottom(), clip.bottom()) - clipped.y);
  }
  if (clipped.w <= 0.0f || clipped.h <= 0.0f) return;
  DrawCmd dc;
  dc.kind = DrawKind::Glyph;
  dc.rect = clipped;
  dc.color = c;
  dc.glyph = Glyph::CaretRight;  // sentinel: "use ascii"
  dc.ascii = ascii;
  dc.scale = scale;
  internal::pushDrawCmd(dc);
}

void emitIconGlyph(Glyph icon, f32 x, f32 y, i32 scale, const Color& c) {
  Rect r{x, y, static_cast<f32>(5 * scale), static_cast<f32>(7 * scale)};
  Rect clipped = r;
  for (const Rect& clip : gClipStack) {
    clipped.x = std::max(clipped.x, clip.x);
    clipped.y = std::max(clipped.y, clip.y);
    clipped.w = std::max(0.0f, std::min(clipped.right(), clip.right()) - clipped.x);
    clipped.h = std::max(0.0f, std::min(clipped.bottom(), clip.bottom()) - clipped.y);
  }
  if (clipped.w <= 0.0f || clipped.h <= 0.0f) return;
  DrawCmd dc;
  dc.kind = DrawKind::Glyph;
  dc.rect = clipped;
  dc.color = c;
  dc.glyph = icon;
  dc.scale = scale;
  internal::pushDrawCmd(dc);
}

bool tapInRect(const Rect& r) {
  if (!gFrame.released) return false;
  return r.contains(gFrame.releasePos.x, gFrame.releasePos.y);
}

bool pressedInRect(const Rect& r) {
  if (!gFrame.pressed) return false;
  return r.contains(gFrame.pressPos.x, gFrame.pressPos.y);
}

}  // namespace kimia::ui::detail

namespace kimia::ui {

// ============================================================================
// Frame-level API
// ============================================================================

void beginFrame(const Frame& f) {
  detail::gFrame = f;
  detail::gFrameActive = true;
  detail::gClipStack.clear();
}

void endFrame() {
  detail::gClipStack.clear();
  detail::gFrameActive = false;
}

const Frame& currentFrame() { return detail::gFrame; }

void pushClip(const Rect& r) { detail::gClipStack.push_back(r); }
void popClip() {
  if (!detail::gClipStack.empty()) detail::gClipStack.pop_back();
}

// ============================================================================
// Panel
// ============================================================================

bool beginPanel(const char* id, const Rect& rect) {
  using namespace theme;
  detail::emitRect(rect, kPanel, 0.0f);
  const f32 h = dp(static_cast<i32>(kPanelHeaderDp));
  Rect header{rect.x, rect.y, rect.w, h};
  detail::emitRect(header, kTitlebar, 0.0f);
  detail::emitRect({rect.x, rect.y, rect.w, 1.0f}, kBorder, 0.0f);
  detail::emitRect({rect.x, rect.y + h - 1.0f, rect.w, 1.0f}, kBorder, 0.0f);
  label(id, header.x + dp(8), header.y + (h - 7.0f) * 0.5f, kText, 1);
  const f32 iconY = header.y + (h - 7.0f) * 0.5f;
  detail::emitIconGlyph(Glyph::Menu, header.right() - dp(20), iconY, 1,
                        kTextMuted);
  pushClip({rect.x, rect.y + h, rect.w, std::max(0.0f, rect.h - h)});
  return true;
}

void endPanel() {
  popClip();
}

// ============================================================================
// Widgets
// ============================================================================

bool button(const char* text, const Rect& rect) {
  using namespace theme;
  const bool hover = rect.contains(detail::gFrame.pointers[0].x,
                                   detail::gFrame.pointers[0].y)
                     && detail::gFrame.pointerCount > 0;
  const bool held = detail::pressedInRect(rect);
  Color bg = hover ? Color{kPanelAlt.r + 0.04f, kPanelAlt.g + 0.04f,
                            kPanelAlt.b + 0.04f, 1.0f}
                    : kPanelAlt;
  if (held) bg = kAccentDim;
  detail::emitRect(rect, bg, dp(4));
  const i32 scale = (rect.h >= dp(28)) ? 2 : 1;
  const f32 textW = static_cast<f32>(std::strlen(text)) * 6 * scale;
  const f32 textH = 7.0f * scale;
  label(text, rect.x + (rect.w - textW) * 0.5f,
        rect.y + (rect.h - textH) * 0.5f, kText, scale);
  return detail::tapInRect(rect);
}

bool iconButton(Glyph icon, const Rect& rect, const char* tooltip) {
  (void)tooltip;
  using namespace theme;
  const bool hover = rect.contains(detail::gFrame.pointers[0].x,
                                   detail::gFrame.pointers[0].y)
                     && detail::gFrame.pointerCount > 0;
  const bool held = detail::pressedInRect(rect);
  Color bg = hover ? Color{kPanelAlt.r + 0.04f, kPanelAlt.g + 0.04f,
                            kPanelAlt.b + 0.04f, 1.0f}
                    : kPanel;
  if (held) bg = kAccentDim;
  detail::emitRect(rect, bg, dp(4));
  const f32 iconX = rect.x + (rect.w - 5.0f) * 0.5f;
  const f32 iconY = rect.y + (rect.h - 7.0f) * 0.5f;
  detail::emitIconGlyph(icon, iconX, iconY, 1, kText);
  return detail::tapInRect(rect);
}

void label(const char* text, const Rect& rect, const Color& color, i32 scale) {
  label(text, rect.x, rect.y, color, scale);
}

void label(const char* text, f32 x, f32 y, const Color& color, i32 scale) {
  if (!text) return;
  const i32 s = std::max(1, std::min(scale, theme::kFontScaleMax));
  const f32 cw = 6.0f * s;
  for (i32 i = 0; text[i]; ++i) {
    detail::emitGlyph(text[i], x + i * cw, y, s, color);
  }
}

bool checkbox(const char* lab, bool& value, const Rect& rect) {
  using namespace theme;
  const f32 boxSize = std::min(rect.h, dp(18));
  Rect box{rect.x, rect.y + (rect.h - boxSize) * 0.5f, boxSize, boxSize};
  detail::emitRect(box, value ? kAccent : kPanelAlt, dp(3));
  if (value) {
    const f32 pad = boxSize * 0.2f;
    detail::emitIconGlyph(Glyph::Check, box.x + pad, box.y + pad, 1,
                          Color{1, 1, 1, 1});
  }
  label(lab, box.right() + dp(6),
        rect.y + (rect.h - 7.0f) * 0.5f, kText, 1);
  if (detail::tapInRect(rect)) {
    value = !value;
    return true;
  }
  return false;
}

bool slider(const char* lab, f32& value, const Rect& rect,
            f32 minValue, f32 maxValue) {
  using namespace theme;
  const f32 trackH = 6.0f;
  const f32 trackY = rect.y + (rect.h - trackH) * 0.5f;
  Rect track{rect.x, trackY, rect.w, trackH};
  detail::emitRect(track, kPanelAlt, trackH * 0.5f);
  const f32 t = (value - minValue) / std::max(0.0001f, maxValue - minValue);
  Rect fill{track.x, track.y, track.w * std::clamp(t, 0.0f, 1.0f), track.h};
  detail::emitRect(fill, kAccent, trackH * 0.5f);
  const f32 thumbR = std::min(rect.h * 0.4f, dp(10));
  const f32 thumbX = track.x + t * track.w;
  Rect thumb{thumbX - thumbR, rect.y + (rect.h - 2 * thumbR) * 0.5f,
             2 * thumbR, 2 * thumbR};
  detail::emitRect(thumb, kText, thumbR);
  if (lab) label(lab, rect.x, rect.y - dp(14), kTextMuted, 1);
  if (detail::pressedInRect(rect)) {
    const f32 rel = static_cast<f32>(std::clamp<f64>(
        (detail::gFrame.pointers[0].x - track.x) /
            std::max<f32>(1.0f, track.w),
        0.0, 1.0));
    value = minValue + rel * (maxValue - minValue);
    return true;
  }
  return false;
}

bool collapsingHeader(const char* lab, bool& open, const Rect& rect) {
  using namespace theme;
  detail::emitRect(rect, kPanelAlt, 0.0f);
  detail::emitIconGlyph(open ? Glyph::CaretDown : Glyph::CaretRight,
                        rect.x + 4.0f, rect.y + (rect.h - 7.0f) * 0.5f, 1,
                        kText);
  label(lab, rect.x + dp(14),
        rect.y + (rect.h - 7.0f) * 0.5f, kText, 1);
  if (detail::tapInRect(rect)) {
    open = !open;
    return true;
  }
  return false;
}

bool selectable(const char* lab, bool selected, const Rect& rect) {
  using namespace theme;
  if (selected) {
    detail::emitRect(rect, Color{kAccent.r, kAccent.g, kAccent.b, 0.30f}, 0.0f);
  } else {
    const bool hover = rect.contains(detail::gFrame.pointers[0].x,
                                     detail::gFrame.pointers[0].y)
                       && detail::gFrame.pointerCount > 0;
    if (hover) detail::emitRect(rect, kHover, 0.0f);
  }
  label(lab, rect.x + dp(8),
        rect.y + (rect.h - 7.0f) * 0.5f,
        selected ? kText : kText, 1);
  return detail::tapInRect(rect);
}

bool textField(const char* id, std::string& value, const Rect& rect) {
  using namespace theme;
  const u64 key = detail::hashId(id);
  detail::WidgetState& ws = detail::widgetState()[key];
  ws.text = value;
  if (ws.cursor > static_cast<i32>(value.size())) {
    ws.cursor = static_cast<i32>(value.size());
  }
  detail::emitRect(rect, kPanelAlt, dp(2));
  Rect inner{rect.x + dp(6), rect.y + (rect.h - 7.0f) * 0.5f,
             rect.w - dp(12), 7.0f};
  label(ws.text.c_str(), inner.x, inner.y, kText, 1);
  // Phase 1: editing comes in Phase 2 with the keyboard. Today the field
  // just renders the current value and accepts no input.
  return false;
}

bool colorField(const char* lab, Color& value, const Rect& rect) {
  using namespace theme;
  (void)lab;
  const f32 swatchW = dp(20);
  Rect swatch{rect.x, rect.y + (rect.h - swatchW) * 0.5f, swatchW, swatchW};
  detail::emitRect(swatch, value, dp(3));
  Rect sliderRect{swatch.right() + dp(8), rect.y,
                  rect.w - swatchW - dp(12), rect.h};
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%3.0f %3.0f %3.0f",
                value.r * 255.0f, value.g * 255.0f, value.b * 255.0f);
  label(buf, sliderRect.x, sliderRect.y + (sliderRect.h - 7.0f) * 0.5f,
        kText, 1);
  if (detail::tapInRect(sliderRect)) {
    const f32 t = static_cast<f32>(std::clamp<f64>(
        (detail::gFrame.releasePos.x - sliderRect.x) /
            std::max<f32>(1.0f, sliderRect.w),
        0.0, 1.0));
    if (t < 0.33f) {
      value.r = std::clamp(value.r + 0.05f, 0.0f, 1.0f);
      return true;
    } else if (t < 0.66f) {
      value.g = std::clamp(value.g + 0.05f, 0.0f, 1.0f);
      return true;
    } else {
      value.b = std::clamp(value.b + 0.05f, 0.0f, 1.0f);
      return true;
    }
  }
  return false;
}

bool vec3Field(const char* lab, Vec3& value, const Rect& rect) {
  (void)lab;
  using namespace theme;
  const f32 third = (rect.w - dp(8)) / 3.0f;
  const f32 gap = dp(4);
  Rect rx{rect.x, rect.y, third, rect.h};
  Rect ry{rx.right() + gap, rect.y, third, rect.h};
  Rect rz{ry.right() + gap, rect.y, third, rect.h};
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%6.2f", value.x);
  label(buf, rx.x, rx.y + (rx.h - 7) * 0.5f, kText, 1);
  std::snprintf(buf, sizeof(buf), "%6.2f", value.y);
  label(buf, ry.x, ry.y + (ry.h - 7) * 0.5f, kText, 1);
  std::snprintf(buf, sizeof(buf), "%6.2f", value.z);
  label(buf, rz.x, rz.y + (rz.h - 7) * 0.5f, kText, 1);
  bool changed = false;
  if (detail::tapInRect(rx)) { value.x += 0.1f; changed = true; }
  if (detail::tapInRect(ry)) { value.y += 0.1f; changed = true; }
  if (detail::tapInRect(rz)) { value.z += 0.1f; changed = true; }
  return changed;
}

bool quatField(const char* lab, Vec3& eulerDeg, const Rect& rect) {
  return vec3Field(lab, eulerDeg, rect);
}

bool beginScroll(const char* id, const Rect& rect, f32& scrollOffset,
                 f32 contentHeight) {
  (void)id;
  using namespace theme;
  pushClip(rect);
  const f32 viewH = rect.h;
  const f32 maxOffset = std::max(0.0f, contentHeight - viewH);
  if (scrollOffset > maxOffset) scrollOffset = maxOffset;
  if (scrollOffset < 0.0f) scrollOffset = 0.0f;
  if (contentHeight > viewH) {
    const f32 barW = dp(static_cast<i32>(kScrollbarDp));
    Rect track{rect.right() - barW, rect.y, barW, viewH};
    detail::emitRect(track, kPanelAlt, barW * 0.5f);
    const f32 thumbH = std::max(20.0f, viewH * (viewH / contentHeight));
    const f32 thumbY = rect.y + (viewH - thumbH) * (scrollOffset / maxOffset);
    Rect thumb{track.x, thumbY, barW, thumbH};
    detail::emitRect(thumb, kTextMuted, barW * 0.5f);
  }
  return true;
}

void endScroll() {
  popClip();
}

bool splitter(const Rect& handle, Rect& leftOrTop, Rect& rightOrBottom,
              bool vertical) {
  (void)vertical;
  using namespace theme;
  detail::emitRect(handle, kSplitter, 0.0f);
  static bool dragging = false;
  static f32 startMouse = 0.0f;
  static f32 startLeft = 0.0f;
  if (detail::pressedInRect(handle)) {
    dragging = true;
    startMouse = detail::gFrame.pointers[0].x;
    startLeft = leftOrTop.w;
  }
  if (!detail::gFrame.pointerCount && dragging) dragging = false;
  if (dragging) {
    const f32 dx = detail::gFrame.pointers[0].x - startMouse;
    const f32 newW = std::max(dp(60), startLeft + dx);
    const f32 delta = newW - leftOrTop.w;
    leftOrTop.w += delta;
    rightOrBottom.x += delta;
    rightOrBottom.w -= delta;
  }
  return dragging;
}

void separator(const Rect& rect) {
  detail::emitRect(rect, theme::kBorder, 0.0f);
}

}  // namespace kimia::ui

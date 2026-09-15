// Immediate-mode widget set.
//
// Usage pattern (every frame):
//   if (ui::beginPanel("Object Tree", rect)) {
//     for (auto& e : entities) {
//       if (ui::selectable(e.name, e.selected)) ui::selectEntity(e.name);
//     }
//     ui::endPanel();
//   }
//
// Each widget call inspects the current input state (pointers, time) and
// emits zero or more commands into `FrameContext::commands`. Widgets never
// allocate after init; per-widget state (textfield cursor, scroll offset)
// lives in a small hash map keyed by widget id.
#pragma once

#include "EditorUI.h"
#include "Theme.h"
#include "Icons.h"

#include <functional>
#include <string>

namespace kimia::ui {

// --- Frame-level context (built by draw()) -------------------------------
//
// Used by the draw routines to know where the next widget should land and
// what state the input is in.
struct Frame {
  Rect viewport;                  // full surface
  f32 dpi = 2.0f;                 // pixel density factor
  i32 pointerCount = 0;           // number of active pointers this frame
  Vec2 pointers[2];               // up to 2 simultaneous pointers
  bool pressed = false;           // at least one pointer went down this frame
  bool released = false;          // at least one pointer went up this frame
  bool dragging = false;          // at least one pointer moved while down
  Vec2 pressPos;                  // position of the most recent down
  Vec2 releasePos;                // position of the most recent up (or zero)
  f32 timeS = 0.0f;               // seconds since first frame
};

// --- Panel / layout -------------------------------------------------------
//
// `beginPanel` returns true while the panel is visible. Between begin/end
// the widget calls are clipped to the panel rect and use the panel's local
// coordinate system (origin at panel top-left).
//
// `id` is a stable string per panel (typically its title). The docking
// system uses it to track position across frames.
struct PanelState;
bool beginPanel(const char* id, const Rect& rect);
void endPanel();

// --- Widgets --------------------------------------------------------------

// A row that takes one line of height. Returns true if pressed this frame.
bool button(const char* label, const Rect& rect);
bool iconButton(Glyph icon, const Rect& rect, const char* tooltip = nullptr);

// Static text. Supports the existing 5x7 ASCII font at scales 1..3.
void label(const char* text, const Rect& rect, const Color& color = theme::kText,
           i32 scale = 1);

// Emit a single rounded-rect draw command. Used by Panel.cpp for chrome
// (tabs, splitters, drop hints). Honors the current clip stack.
void drawRect(const Rect& rect, const Color& color, f32 cornerRadius = 0.0f);

// Emit a single glyph draw command at the given pixel position.
void drawGlyph(char ascii, f32 x, f32 y, i32 scale, const Color& color);
void drawGlyph(Glyph icon, f32 x, f32 y, i32 scale, const Color& color);
// Render a NUL-terminated ASCII string with the editor's bitmap font.
// One letter per glyph slot; no kerning, no wrapping.
void drawText(const char* text, f32 x, f32 y, i32 scale, const Color& color);

// Label with auto-width based on string length.
void label(const char* text, f32 x, f32 y, const Color& color = theme::kText,
           i32 scale = 1);

// Toggle / checkbox. Returns true if state changed.
bool checkbox(const char* label, bool& value, const Rect& rect);

// Horizontal slider 0..1. Returns true while user is dragging.
bool slider(const char* label, f32& value, const Rect& rect,
            f32 minValue = 0.0f, f32 maxValue = 1.0f);

// Collapsing header (used by sections in the Property Sheet).
bool collapsingHeader(const char* label, bool& open, const Rect& rect);

// Selectable row. Returns true on click. If selected, drawn with accent bg.
bool selectable(const char* label, bool selected, const Rect& rect);

// Text input. Returns true if value changed.
bool textField(const char* id, std::string& value, const Rect& rect);

// Color picker (3 sliders). Returns true if any channel changed.
bool colorField(const char* label, Color& value, const Rect& rect);

// Vec3 input: 3 small text fields next to each other. Returns true if any
// field changed.
bool vec3Field(const char* label, Vec3& value, const Rect& rect);

// Quat input as euler XYZ degrees (most artists think in eulers).
bool quatField(const char* label, Vec3& eulerDeg, const Rect& rect);

// Vertical scrollable area. Returns inner content height (pixels) so the
// caller can pad to allow scroll-bouncing. Pass `scrollOffset` by ref; the
// widget adjusts it when the user drags within the rect.
bool beginScroll(const char* id, const Rect& rect, f32& scrollOffset,
                 f32 contentHeight);
void endScroll();

// Splitter: a draggable handle between two rects. Updates the right rect's
// left edge as the user drags.
bool splitter(const Rect& handle, Rect& leftOrTop, Rect& rightOrBottom,
              bool vertical = false);

// Separator (1 pixel line).
void separator(const Rect& rect);

// --- Layer / clipping -----------------------------------------------------
//
// pushClip / popClip stack. Widgets drawn between push and pop are clipped
// to the union of all clips on the stack. Used by panels + scroll views.
void pushClip(const Rect& r);
void popClip();

// --- Internal: frame access ---------------------------------------------
//
// These are called by EditorUI::draw() and not by user code.
void beginFrame(const Frame& f);
void endFrame();
const Frame& currentFrame();

}  // namespace kimia::ui

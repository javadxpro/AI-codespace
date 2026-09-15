// Theme: "Dark Pro" — Unity-feel palette and spacing, adapted for touch.
//
// Centralised so every widget reads from the same palette. Animations use
// the same durations so the UI feels coherent. All values are in
// surface-pixel units unless suffixed `_dp` (density-independent).
#pragma once

#include "EditorUI.h"

namespace kimia::ui::theme {

// --- Palette --------------------------------------------------------------
// Surfaces
constexpr Color kBg          {0.219f, 0.219f, 0.219f, 1.0f};   // #383838 window bg
constexpr Color kPanel       {0.176f, 0.176f, 0.176f, 1.0f};   // #2D2D2D panels
constexpr Color kPanelAlt    {0.150f, 0.150f, 0.150f, 1.0f};   // #262626 deeper
constexpr Color kTitlebar    {0.122f, 0.122f, 0.122f, 1.0f};   // #1F1F1F title bars
constexpr Color kBorder      {0.102f, 0.102f, 0.102f, 1.0f};   // #1A1A1A borders
constexpr Color kSplitter    {0.078f, 0.078f, 0.078f, 1.0f};   // #141414 splitters

// Text
constexpr Color kText        {0.878f, 0.878f, 0.878f, 1.0f};   // #E0E0E0 primary
constexpr Color kTextMuted   {0.580f, 0.580f, 0.580f, 1.0f};   // #949494 secondary
constexpr Color kTextDim     {0.420f, 0.420f, 0.420f, 1.0f};   // #6B6B6B tertiary

// Accent
constexpr Color kAccent      {0.290f, 0.565f, 0.886f, 1.0f};   // #4A90E2 blue
constexpr Color kAccentHot   {0.380f, 0.650f, 0.960f, 1.0f};   // brighter blue
constexpr Color kAccentDim   {0.180f, 0.380f, 0.640f, 1.0f};   // darker blue

// Semantic
constexpr Color kSuccess     {0.298f, 0.686f, 0.314f, 1.0f};   // #4CAF50
constexpr Color kWarning     {1.000f, 0.722f, 0.180f, 1.0f};   // #FFB82E
constexpr Color kError       {0.957f, 0.263f, 0.212f, 1.0f};   // #F44336

// Gizmo (matches Unity Editor's color scheme)
constexpr Color kGizmoX      {0.890f, 0.255f, 0.290f, 1.0f};   // #E33E4A red
constexpr Color kGizmoY      {0.196f, 0.804f, 0.196f, 1.0f};   // #32CD32 green
constexpr Color kGizmoZ      {0.204f, 0.396f, 0.957f, 1.0f};   // #3465F4 blue

// Selection / hover
constexpr Color kSelection   {0.290f, 0.565f, 0.886f, 0.40f};  // accent @ 40%
constexpr Color kHover       {1.000f, 1.000f, 1.000f, 0.06f};  // white @ 6%

// --- Sizing ---------------------------------------------------------------
// Touch targets are sized for fingers; minimum 44dp per Material guidelines.
constexpr f32 kTouchTargetDp   = 44.0f;
constexpr f32 kPanelHeaderDp   = 28.0f;
constexpr f32 kToolbarDp       = 32.0f;
constexpr f32 kSplitterDp      = 6.0f;     // grab width
constexpr f32 kScrollbarDp     = 6.0f;
constexpr f32 kPaddingDp       = 8.0f;
constexpr f32 kRowHeightDp     = 22.0f;    // one line in Object Tree / Inspector
constexpr f32 kIndentDp        = 14.0f;    // tree indent per level

// --- Animations -----------------------------------------------------------
constexpr f32 kAnimFast    = 0.10f;   // seconds
constexpr f32 kAnimMed     = 0.20f;
constexpr f32 kLongPressS  = 0.45f;   // tap → long-press threshold

// Helpers — convert dp to surface pixels using the current DPI factor
// (set by resize() based on the surface's density).
f32 dp(i32 v);
f32 sp(i32 v);   // scaled px (same as dp() at 1.0 density)
void setDpi(f32 dpi);
f32 dpi();

// --- Glyph metrics --------------------------------------------------------
constexpr i32 kFontGlyphW = 5;          // bitmap font cell width  (px)
constexpr i32 kFontGlyphH = 7;          // bitmap font cell height (px)
constexpr i32 kFontScaleMax = 3;        // largest scale we pre-rasterise

}  // namespace kimia::ui::theme

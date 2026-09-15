// Panel system with dockable tabs and splitters.
//
// A panel belongs to a "slot". A slot is a region of the editor surface
// (left, right, top, bottom, centre). Slots can hold multiple tabs and
// the user can drag tabs between slots. Slots are separated by splitters
// the user can resize.
//
// The layout is computed every frame from:
//   * the dock tree (slots + their tabs)
//   * the splitter positions (as fractions of the surface)
//   * the current surface size
//
// Persistent state lives in a small struct that survives across frames.
// It is owned by the editor; the host reads/writes it via the public API
// below.
#pragma once

#include "EditorUI.h"
#include "Theme.h"
#include "Widget.h"

#include <string>
#include <vector>

namespace kimia::ui {



// Where a tab is currently hosted. Slots are identified by a stable id.
struct DockSlot {
  std::string id;             // e.g. "left", "right", "top", "bottom", "centre"
  Rect rect;                  // computed each frame
  std::vector<std::string> tabs;   // panel titles, in display order
  i32 active = 0;             // index into tabs
};

// Identifies which side of a slot a new tab-drop would land in.
enum class DropZone {
  None,
  Center,        // dropped on the slot itself (add as a tab)
  Left,          // create a new slot to the left, splitting horizontally
  Right,
  Top,
  Bottom,
};

// Persistent layout — what the host saves to disk between sessions.
struct DockLayout {
  std::vector<DockSlot> slots;
  // Fractions of the surface dedicated to each edge slot (0..1).
  f32 leftFrac = 0.22f;
  f32 rightFrac = 0.0f;
  f32 topFrac = 0.06f;        // toolbar
  f32 bottomFrac = 0.22f;
  // Available panels registered with the layout.
  std::vector<std::string> availablePanels;
};

// Register a panel. Safe to call every frame; dedups by title.
void registerPanel(const char* title);

// Compute layout rectangles for the current surface size. Call once per
// frame before drawing any panel.
void layoutDock(const Rect& surface, DockLayout& layout);

// Draw the dock chrome (tab bars + splitters) and handle tab dragging.
// Returns the drag state to the caller; callers can inspect the result
// to decide whether to apply a drop or not.
struct DockInteraction {
  bool draggingTab = false;
  std::string fromSlot;
  std::string draggedTab;
  Vec2 dragPos;
  DropZone hoverZone = DropZone::None;
  std::string hoverSlot;
};
DockInteraction drawDockChrome(DockLayout& layout);

// Render helpers: draw a tab strip + content area for one slot. The user
// code (panels) calls these between beginPanel/endPanel pairs; the dock
// layout sets the right rects automatically.
void drawSlot(DockSlot& slot, bool isActive);

// Apply a drag-drop: caller invokes when DockInteraction says a tab was
// dropped on a zone.
void applyDrop(DockLayout& layout, const std::string& fromSlot,
               const std::string& tab, const std::string& toSlot,
               DropZone zone);

}  // namespace kimia::ui

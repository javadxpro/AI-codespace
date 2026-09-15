// Panel system — dock layout, tab chrome, splitter, tab drag-drop.
//
// Phase 1 keeps interactions simple:
//   * tap a tab → activate it
//   * long-press + drag a tab → enter drag mode, drop on another slot
//   * drag a splitter → resize that slot
//
// All state (drag in progress, splitter drag, slot rects) is recomputed
// each frame from the persistent DockLayout + the live Frame snapshot.
// Widgets call into us via the helpers in <kimia/EditorUI.h>.
#include <kimia/EditorUI.h>
#include <kimia/Theme.h>
#include <kimia/Panel.h>
#include <kimia/Widget.h>
#include <kimia/Icons.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace kimia::ui {

// Forward — declared in EditorUI.h.
using namespace theme;

namespace {

// Persistent drag state for tab DnD.
struct DragState {
  bool active = false;
  std::string fromSlot;
  std::string tab;
  Vec2 startPos;
  Vec2 pos;
  f32 startTime = 0.0f;
};
DragState gDrag;
bool gSplitterDragging = false;
f32 gSplitterStartMouse = 0.0f;
f32 gSplitterStartFrac = 0.0f;

const Frame& frame() { return currentFrame(); }

DockSlot* findSlot(DockLayout& layout, const std::string& id) {
  for (DockSlot& s : layout.slots) if (s.id == id) return &s;
  return nullptr;
}
const DockSlot* findSlot(const DockLayout& layout, const std::string& id) {
  for (const DockSlot& s : layout.slots) if (s.id == id) return &s;
  return nullptr;
}

void computeRects(const Rect& surface, DockLayout& layout) {
  const f32 topH = surface.h * layout.topFrac;
  const f32 botH = surface.h * layout.bottomFrac;
  const f32 leftW = surface.w * layout.leftFrac;
  const f32 rightW = surface.w * layout.rightFrac;
  const f32 centerW = std::max(0.0f, surface.w - leftW - rightW);
  const f32 midH = std::max(0.0f, surface.h - topH - botH);
  DockSlot* top = findSlot(layout, "top");
  DockSlot* bot = findSlot(layout, "bottom");
  DockSlot* left = findSlot(layout, "left");
  DockSlot* centre = findSlot(layout, "centre");
  DockSlot* right = findSlot(layout, "right");
  if (top) top->rect = {0, 0, surface.w, topH};
  if (left) left->rect = {0, topH, leftW, midH};
  if (centre) centre->rect = {leftW, topH, centerW, midH};
  if (right) right->rect = {surface.w - rightW, topH, rightW, midH};
  if (bot) bot->rect = {0, surface.h - botH, surface.w, botH};
}

DropZone hitTestDrop(const Rect& body, Vec2 pos) {
  if (!body.contains(pos.x, pos.y)) return DropZone::None;
  const f32 bandX = body.w * 0.25f;
  const f32 bandY = body.h * 0.25f;
  if (pos.x - body.x < bandX) return DropZone::Left;
  if (body.right() - pos.x < bandX) return DropZone::Right;
  if (pos.y - body.y < bandY) return DropZone::Top;
  if (body.bottom() - pos.y < bandY) return DropZone::Bottom;
  return DropZone::Center;
}

}  // namespace

void registerPanel(const char* title) {
  if (!title) return;
  DockLayout& l = layout();
  for (const std::string& s : l.availablePanels) if (s == title) return;
  l.availablePanels.push_back(title);
}

void layoutDock(const Rect& surface, DockLayout& layout) {
  computeRects(surface, layout);
}

DockInteraction drawDockChrome(DockLayout& layout) {
  DockInteraction out;
  const f32 headerH = dp(static_cast<i32>(kPanelHeaderDp));

  // --- Splitter drag (between left/centre) -----------------------------
  DockSlot* left = findSlot(layout, "left");
  DockSlot* centre = findSlot(layout, "centre");
  DockSlot* bottom = findSlot(layout, "bottom");
  if (left && centre) {
    Rect handle{left->rect.right(), left->rect.y,
                dp(static_cast<i32>(kSplitterDp)), left->rect.h};
    drawRect(handle, kSplitter);
    if (frame().pressed && handle.contains(frame().pressPos.x, frame().pressPos.y)) {
      gSplitterDragging = true;
      gSplitterStartMouse = frame().pressPos.x;
      gSplitterStartFrac = layout.leftFrac;
    }
    if (gSplitterDragging && frame().pointerCount > 0) {
      const f32 surfaceW = layout.leftFrac > 0 ? left->rect.w / layout.leftFrac : 1.0f;
      const f32 dx = frame().pointers[0].x - gSplitterStartMouse;
      const f32 newW = std::clamp(gSplitterStartFrac * surfaceW + dx,
                                  dp(120), surfaceW - dp(120));
      layout.leftFrac = newW / surfaceW;
    }
    if (frame().released || !frame().pointerCount) gSplitterDragging = false;
  }
  if (centre && bottom) {
    Rect handle{centre->rect.x, centre->rect.bottom(),
                centre->rect.w, dp(static_cast<i32>(kSplitterDp))};
    drawRect(handle, kSplitter);
    if (frame().pressed && handle.contains(frame().pressPos.x, frame().pressPos.y)) {
      gSplitterDragging = true;
      gSplitterStartMouse = frame().pressPos.y;
      gSplitterStartFrac = layout.bottomFrac;
    }
    if (gSplitterDragging && frame().pointerCount > 0) {
      const f32 surfaceH = layout.bottomFrac > 0 ? bottom->rect.h / layout.bottomFrac : 1.0f;
      const f32 dy = frame().pointers[0].y - gSplitterStartMouse;
      const f32 newH = std::clamp(gSplitterStartFrac * surfaceH + dy,
                                  dp(80), surfaceH - dp(80));
      layout.bottomFrac = newH / surfaceH;
    }
    if (frame().released || !frame().pointerCount) gSplitterDragging = false;
  }

  // --- Tab hit-tests + drag detection ---------------------------------
  struct TabRef { DockSlot* slot; std::size_t index; Rect rect; };
  std::vector<TabRef> tabs;
  for (DockSlot& slot : layout.slots) {
    if (slot.tabs.empty()) continue;
    f32 x = slot.rect.x;
    for (std::size_t i = 0; i < slot.tabs.size(); ++i) {
      const std::string& name = slot.tabs[i];
      const f32 tabW = std::min(slot.rect.w * 0.4f,
                                std::max(dp(80),
                                         static_cast<f32>(name.size()) * 8.0f + dp(16)));
      Rect tabRect{x, slot.rect.y, tabW, headerH};
      tabs.push_back({&slot, i, tabRect});
      x += tabW;
      if (x >= slot.rect.right() - dp(40)) break;
    }
  }

  // Detect a fresh press on a tab.
  if (frame().pressed) {
    for (const TabRef& t : tabs) {
      if (t.rect.contains(frame().pressPos.x, frame().pressPos.y)) {
        gDrag.startPos = frame().pressPos;
        gDrag.fromSlot = t.slot->id;
        gDrag.tab = t.slot->tabs[t.index];
        gDrag.startTime = frame().timeS;
      }
    }
  }
  // Promote press to drag on long-press or distance threshold.
  if (!gDrag.active && frame().pointerCount > 0 && !gDrag.tab.empty()
      && gDrag.startTime > 0.0f) {
    const f32 heldFor = frame().timeS - gDrag.startTime;
    const Vec2 pos = frame().pointers[0];
    const f32 dx = pos.x - gDrag.startPos.x;
    const f32 dy = pos.y - gDrag.startPos.y;
    const f32 dist = std::sqrt(dx * dx + dy * dy);
    if (heldFor > kLongPressS || dist > dp(8)) {
      gDrag.active = true;
      gDrag.pos = pos;
    }
  }
  if (gDrag.active && frame().pointerCount > 0) {
    gDrag.pos = frame().pointers[0];
    out.draggingTab = true;
    out.dragPos = gDrag.pos;
    out.fromSlot = gDrag.fromSlot;
    out.draggedTab = gDrag.tab;
    for (DockSlot& slot : layout.slots) {
      Rect body{slot.rect.x, slot.rect.y + headerH, slot.rect.w,
                std::max(0.0f, slot.rect.h - headerH)};
      DropZone z = hitTestDrop(body, gDrag.pos);
      if (z != DropZone::None) {
        out.hoverZone = z;
        out.hoverSlot = slot.id;
        break;
      }
    }
  }
  // Drop on release.
  if (gDrag.active && frame().released && frame().pointerCount == 0) {
    if (!out.hoverSlot.empty() && out.hoverSlot != gDrag.fromSlot) {
      applyDrop(layout, gDrag.fromSlot, gDrag.tab, out.hoverSlot, out.hoverZone);
    }
    gDrag = DragState{};
  }
  if (!frame().pointerCount) {
    // do not clear here — wait one frame for the drop to settle
  }

  // --- Draw tabs + collect active panels ------------------------------
  std::vector<ActivePanel> active;
  for (DockSlot& slot : layout.slots) {
    if (slot.tabs.empty()) continue;
    Rect tabStrip{slot.rect.x, slot.rect.y, slot.rect.w, headerH};
    drawRect(tabStrip, kTitlebar);
    drawRect({tabStrip.x, tabStrip.bottom() - 1, tabStrip.w, 1}, kBorder);
    f32 x = slot.rect.x;
    for (std::size_t i = 0; i < slot.tabs.size(); ++i) {
      const std::string& name = slot.tabs[i];
      const f32 tabW = std::min(slot.rect.w * 0.4f,
                                std::max(dp(80),
                                         static_cast<f32>(name.size()) * 8.0f + dp(16)));
      Rect tabRect{x, slot.rect.y, tabW, headerH};
      const bool isActive = static_cast<i32>(i) == slot.active;
      drawRect(tabRect, isActive ? kPanel : kTitlebar);
      if (frame().released && tabRect.contains(frame().releasePos.x,
                                                frame().releasePos.y)) {
        if (!gDrag.active) slot.active = static_cast<i32>(i);
      }
      drawText(name.c_str(), tabRect.x + dp(8),
                tabRect.y + (headerH - 7.0f) * 0.5f, 1,
                isActive ? kText : kTextMuted);
      x += tabW;
      if (x >= slot.rect.right() - dp(40)) break;
    }
    ActivePanel ap;
    ap.id = slot.tabs[static_cast<std::size_t>(slot.active)];
    ap.rect = slot.rect;
    ap.contentRect = {slot.rect.x, slot.rect.y + headerH, slot.rect.w,
                      std::max(0.0f, slot.rect.h - headerH)};
    active.push_back(ap);
  }
  internal::setActivePanels(std::move(active));

  // --- Drag preview overlay -------------------------------------------
  if (gDrag.active) {
    const f32 previewW = dp(120);
    const f32 previewH = dp(28);
    Rect preview{static_cast<f32>(gDrag.pos.x) - previewW * 0.5f,
                 static_cast<f32>(gDrag.pos.y) - previewH * 0.5f,
                 previewW, previewH};
    drawRect(preview, Color{kAccent.r, kAccent.g, kAccent.b, 0.85f}, dp(4));
    drawText(gDrag.tab.c_str(), preview.x + dp(8),
              preview.y + (preview.h - 7.0f) * 0.5f, 1,
              Color{1, 1, 1, 1});
    if (!out.hoverSlot.empty()) {
      const DockSlot* target = findSlot(layout, out.hoverSlot);
      if (target) {
        Rect body{target->rect.x, target->rect.y + headerH, target->rect.w,
                  std::max(0.0f, target->rect.h - headerH)};
        Rect hint = body;
        switch (out.hoverZone) {
          case DropZone::Left:   hint.w *= 0.5f; break;
          case DropZone::Right:  hint.x += hint.w * 0.5f; hint.w *= 0.5f; break;
          case DropZone::Top:    hint.h *= 0.5f; break;
          case DropZone::Bottom: hint.y += hint.h * 0.5f; hint.h *= 0.5f; break;
          default: break;
        }
        drawRect(hint, Color{kAccent.r, kAccent.g, kAccent.b, 0.25f}, 0.0f);
      }
    }
  }
  return out;
}

void applyDrop(DockLayout& layout, const std::string& fromSlot,
               const std::string& tab, const std::string& toSlot,
               DropZone zone) {
  DockSlot* from = findSlot(layout, fromSlot);
  DockSlot* to = findSlot(layout, toSlot);
  if (!from || !to) return;
  from->tabs.erase(std::remove(from->tabs.begin(), from->tabs.end(), tab),
                   from->tabs.end());
  if (from->tabs.empty()) {
    from->active = 0;
  } else if (from->active >= static_cast<i32>(from->tabs.size())) {
    from->active = static_cast<i32>(from->tabs.size()) - 1;
  }
  switch (zone) {
    case DropZone::Center:
      to->tabs.push_back(tab);
      to->active = static_cast<i32>(to->tabs.size()) - 1;
      break;
    case DropZone::Left:
    case DropZone::Right:
    case DropZone::Top:
    case DropZone::Bottom: {
      DockSlot ns;
      ns.id = toSlot + "_split";
      ns.tabs = {tab};
      layout.slots.push_back(ns);
      break;
    }
    default: break;
  }
}

void drawSlot(DockSlot&, bool) {
  // Reserved for future per-slot chrome.
}

}  // namespace kimia::ui

// EditorUI implementation — Phase 1.
//
// Phase 1 is intentionally split in two:
//   * The CPU-side logic (input tracking, layout, hit-testing, panel
//     dispatch) is fully implemented and testable without GL.
//   * The GL renderer (program, atlas, vertex buffers) is stubbed for
//     now and will be wired up via JNI on Android. The desktop
//     build skips GL and runs the same editor logic against the CPU
//     path; the editor itself stays portable.
//
// All internal state lives in this file (anonymous namespace). Widgets,
// panels and the host talk to us through the public functions in
// <kimia/EditorUI.h>; the Widget/Panel code uses the internal helpers
// `pushDrawCmd` / `setActivePanels` declared in the same header.
#include <kimia/EditorUI.h>
#include <kimia/GLFunctions.h>
#include <kimia/Theme.h>
#include <kimia/Widget.h>
#include <kimia/Panel.h>
#include <kimia/Icons.h>
#include <kimia/BitmapFont.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace kimia::ui {

// ============================================================================
// Internal state — anonymous namespace so it stays private to this TU.
// ============================================================================

namespace {

std::vector<DrawCmd> gDrawCmds;
std::mutex gDrawMutex;

struct PendingPointer { PointerEvent ev; };
std::mutex gPointerMutex;
std::vector<PendingPointer> gPointerQueue;

struct LivePointer {
  bool down = false;
  Vec2 pos;
  Vec2 startPos;
  f32 startTime = 0.0f;
  f32 lastClickTime = -1.0f;
};
std::unordered_map<i32, LivePointer> gLive;

DockLayout gLayout;

struct WidgetState {
  std::string text;
  i32 cursor = 0;
  f32 scrollOffset = 0.0f;
  bool active = false;
};
std::unordered_map<u64, WidgetState> gWidgetState;

Frame gFrame;
bool gFrameActive = false;
std::vector<ActivePanel> gActivePanels;

std::deque<std::string> gLogRing;
constexpr std::size_t kLogMax = 256;
std::mutex gLogMutex;

struct GLState {
  bool ready = false;
};
GLState gGL;

void drainPointers(f32 nowS) {
  std::vector<PendingPointer> drained;
  {
    std::lock_guard<std::mutex> lk(gPointerMutex);
    drained.swap(gPointerQueue);
  }
  for (const PendingPointer& pp : drained) {
    const PointerEvent& ev = pp.ev;
    LivePointer& lp = gLive[ev.id];
    switch (ev.action) {
      case PointerAction::Down:
        lp.down = true;
        lp.pos = {ev.x, ev.y};
        lp.startPos = lp.pos;
        lp.startTime = nowS;
        break;
      case PointerAction::Move:
        lp.pos = {ev.x, ev.y};
        break;
      case PointerAction::Up:
        lp.down = false;
        lp.lastClickTime = nowS;
        break;
      case PointerAction::Cancel:
        lp.down = false;
        break;
    }
  }
}

Frame buildFrame(f32 nowS) {
  drainPointers(nowS);
  Frame f = gFrame;
  f.timeS = nowS;
  i32 n = 0;
  Vec2 pressPos{};
  Vec2 releasePos{};
  bool pressed = false, released = false, dragging = false;
  for (auto& kv : gLive) {
    if (n >= 2) break;
    f.pointers[n] = kv.second.pos;
    if (kv.second.down) {
      ++n;
      if (kv.second.startTime >= nowS - 0.05f) pressed = true;
      pressPos = kv.second.startPos;
      if ((kv.second.pos.x != kv.second.startPos.x) ||
          (kv.second.pos.y != kv.second.startPos.y)) {
        dragging = true;
      }
    } else if (kv.second.lastClickTime >= nowS - 0.05f) {
      released = true;
      releasePos = kv.second.pos;
    }
  }
  f.pointerCount = n;
  f.pressed = pressed;
  f.released = released;
  f.dragging = dragging;
  f.pressPos = pressPos;
  f.releasePos = releasePos;
  return f;
}

// ============================================================================
// Per-panel renderers (CPU-only for Phase 1)
// ============================================================================

void renderObjectTree(const ActivePanel& ap, FrameContext& ctx) {
  beginPanel(ap.id.c_str(), ap.contentRect);
  using namespace theme;
  f32 y = ap.contentRect.y;
  const f32 lineH = dp(static_cast<i32>(kRowHeightDp));
  label("Entities", ap.contentRect.x + dp(8), y + 4.0f, kTextMuted, 1);
  y += lineH + 4.0f;
  drawRect({ap.contentRect.x, y, ap.contentRect.w, 1.0f}, kBorder);
  y += 4.0f;
  const u64 scrollId = std::hash<std::string>{}(ap.id + ":scroll");
  WidgetState& ws = gWidgetState[scrollId];
  const f32 contentH = static_cast<f32>(ctx.scene.entities.size()) * lineH;
  const f32 viewH = ap.contentRect.bottom() - y;
  Rect viewRect{ap.contentRect.x, y, ap.contentRect.w, viewH};
  beginScroll(ap.id.c_str(), viewRect, ws.scrollOffset, contentH);
  f32 rowY = y - ws.scrollOffset;
  for (const EntityRef& e : ctx.scene.entities) {
    bool isSel = false;
    for (const std::string& n : ctx.scene.selectedNames) {
      if (n == e.name) { isSel = true; break; }
    }
    Rect rowRect{ap.contentRect.x, rowY, ap.contentRect.w, lineH};
    if (selectable(e.name.c_str(), isSel, rowRect)) {
      UiCommand c;
      c.kind = UiCommandKind::SelectEntity;
      c.name = e.name;
      ctx.commands.push_back(c);
    }
    rowY += lineH;
  }
  endScroll();
  endPanel();
}

void renderPropertySheet(const ActivePanel& ap, FrameContext& ctx) {
  beginPanel(ap.id.c_str(), ap.contentRect);
  using namespace theme;
  f32 y = ap.contentRect.y;
  const f32 lineH = dp(static_cast<i32>(kRowHeightDp));
  label("Selection", ap.contentRect.x + dp(8), y + 4.0f, kTextMuted, 1);
  y += lineH + 4.0f;
  drawRect({ap.contentRect.x, y, ap.contentRect.w, 1.0f}, kBorder);
  y += 8.0f;
  if (ctx.scene.selectedNames.empty()) {
    label("(no selection)", ap.contentRect.x + dp(8), y, kTextDim, 1);
  } else {
    for (const std::string& name : ctx.scene.selectedNames) {
      const EntityRef* ent = nullptr;
      for (const EntityRef& e : ctx.scene.entities) {
        if (e.name == name) { ent = &e; break; }
      }
      if (!ent) continue;
      label(ent->name.c_str(), ap.contentRect.x + dp(8), y, kText, 2);
      y += dp(28);
      drawRect({ap.contentRect.x, y - 2, ap.contentRect.w, 1.0f}, kBorder);
      y += 6.0f;
      label("Position", ap.contentRect.x + dp(8), y, kTextMuted, 1);
      y += lineH;
      Rect vecRect{ap.contentRect.x + dp(8), y,
                   ap.contentRect.w - dp(16), lineH};
      Vec3 pos{ent->posX, ent->posY, ent->posZ};
      if (vec3Field("pos", pos, vecRect)) {
        UiCommand c;
        c.kind = UiCommandKind::SetPosition;
        c.name = ent->name;
        c.x = static_cast<f32>(pos.x);
        c.y = static_cast<f32>(pos.y);
        c.z = static_cast<f32>(pos.z);
        ctx.commands.push_back(c);
      }
      y += lineH + 6.0f;
      label("Color", ap.contentRect.x + dp(8), y, kTextMuted, 1);
      y += lineH;
      Rect colorRect{ap.contentRect.x + dp(8), y,
                     ap.contentRect.w - dp(16), lineH};
      Color col{ent->colorR, ent->colorG, ent->colorB};
      if (colorField("color", col, colorRect)) {
        UiCommand c;
        c.kind = UiCommandKind::SetColor;
        c.name = ent->name;
        c.r = col.r; c.g = col.g; c.b = col.b;
        ctx.commands.push_back(c);
      }
      y += lineH + 6.0f;
    }
  }
  endPanel();
}

void renderScene(const ActivePanel& ap, const FrameContext& ctx) {
  beginPanel(ap.id.c_str(), ap.contentRect);
  using namespace theme;
  label("Scene", ap.contentRect.x + dp(8),
        ap.contentRect.y + dp(8), kTextMuted, 1);
  label(ctx.scene.playing ? "Playing" : "Editing",
        ap.contentRect.x + dp(8),
        ap.contentRect.bottom() - dp(20),
        ctx.scene.playing ? kSuccess : kWarning, 1);
  endPanel();
}

void renderToolbar(const ActivePanel& ap, FrameContext& ctx) {
  beginPanel(ap.id.c_str(), ap.contentRect);
  using namespace theme;
  const f32 btn = dp(36);
  const f32 gap = dp(4);
  f32 x = ap.contentRect.x + dp(8);
  const f32 y = ap.contentRect.y + (ap.contentRect.h - btn) * 0.5f;
  static i32 activeTool = 0;
  const char* toolLabels[4] = {"Sel", "Move", "Rot", "Scale"};
  for (i32 i = 0; i < 4; ++i) {
    Rect r{x, y, btn, btn};
    if (selectable(toolLabels[i], activeTool == i, r)) {
      activeTool = i;
      UiCommand c;
      c.kind = UiCommandKind::ToolChanged;
      c.tool = i;
      ctx.commands.push_back(c);
    }
    x += btn + gap;
  }
  x += dp(8);
  drawRect({x, ap.contentRect.y + 4.0f, 1.0f, ap.contentRect.h - 8.0f}, kBorder);
  x += dp(12);
  const char* playLabels[3] = {"Play", "Pause", "Step"};
  for (i32 i = 0; i < 3; ++i) {
    Rect r{x, y, btn * 1.4f, btn};
    if (button(playLabels[i], r)) {
      UiCommand c;
      c.kind = (i == 0) ? UiCommandKind::PlayPressed
          :  (i == 1) ? UiCommandKind::PausePressed
                      : UiCommandKind::StepPressed;
      ctx.commands.push_back(c);
    }
    x += btn * 1.4f + gap;
  }
  endPanel();
}

void renderLog(const ActivePanel& ap, FrameContext& /*ctx*/) {
  beginPanel(ap.id.c_str(), ap.contentRect);
  using namespace theme;
  f32 y = ap.contentRect.y;
  const f32 lineH = dp(static_cast<i32>(kRowHeightDp));
  label("Log", ap.contentRect.x + dp(8), y + 4.0f, kTextMuted, 1);
  y += lineH + 4.0f;
  drawRect({ap.contentRect.x, y, ap.contentRect.w, 1.0f}, kBorder);
  y += 4.0f;
  const u64 scrollId = std::hash<std::string>{}(ap.id + ":scroll");
  WidgetState& ws = gWidgetState[scrollId];
  const f32 contentH = static_cast<f32>(gLogRing.size()) * lineH;
  const f32 viewH = ap.contentRect.bottom() - y;
  Rect viewRect{ap.contentRect.x, y, ap.contentRect.w, viewH};
  beginScroll(ap.id.c_str(), viewRect, ws.scrollOffset, contentH);
  f32 rowY = y - ws.scrollOffset;
  std::lock_guard<std::mutex> lk(gLogMutex);
  for (const std::string& line : gLogRing) {
    if (rowY > ap.contentRect.bottom()) break;
    drawText(line.c_str(), ap.contentRect.x + dp(8), rowY, 1, kTextDim);
    rowY += lineH;
  }
  endScroll();
  endPanel();
}

void renderDefault(const ActivePanel& ap) {
  beginPanel(ap.id.c_str(), ap.contentRect);
  using namespace theme;
  label(ap.id.c_str(), ap.contentRect.x + dp(8),
        ap.contentRect.y + dp(8), kText, 2);
  endPanel();
}

}  // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

namespace internal {
void pushDrawCmd(const DrawCmd& cmd) { gDrawCmds.push_back(cmd); }
void setActivePanels(std::vector<ActivePanel> panels) {
  gActivePanels = std::move(panels);
}
}  // namespace internal

bool initialize(std::string& error) {
  (void)error;
  if (gLayout.slots.empty()) {
    DockSlot toolbar; toolbar.id = "top"; toolbar.tabs = {"Toolbar"};
    DockSlot left; left.id = "left"; left.tabs = {"Object Tree"};
    DockSlot center; center.id = "centre"; center.tabs = {"Property Sheet"};
    DockSlot right; right.id = "right"; right.tabs = {"Scene View"};
    DockSlot bottom; bottom.id = "bottom"; bottom.tabs = {"Log"};
    gLayout.slots = {toolbar, left, center, right, bottom};
    gLayout.availablePanels = {
        "Object Tree", "Property Sheet", "Scene View",
        "Game View", "Toolbar", "Log",
    };
  }
  gGL.ready = false;
  return true;
}

DockLayout& layout() { return gLayout; }
void setLayout(const DockLayout& l) { gLayout = l; }

void shutdown() {
  gGL = {};
}

void resize(i32 width, i32 height) {
  gFrame.viewport = Rect{0.0f, 0.0f, static_cast<f32>(width),
                         static_cast<f32>(height)};
  const f32 density = std::max(1.5f,
      std::min(3.5f, static_cast<f32>(width) / 540.0f));
  theme::setDpi(density);
  gFrame.dpi = density;
}

void submitPointer(const PointerEvent& ev) {
  std::lock_guard<std::mutex> lk(gPointerMutex);
  gPointerQueue.push_back({ev});
}

void submitKey(const KeyEvent&) {}

void log(const std::string& line) {
  std::lock_guard<std::mutex> lk(gLogMutex);
  gLogRing.push_back(line);
  if (gLogRing.size() > kLogMax) gLogRing.pop_front();
}

const std::vector<std::string>& logLines() {
  static std::vector<std::string> snapshot;
  std::lock_guard<std::mutex> lk(gLogMutex);
  snapshot.assign(gLogRing.begin(), gLogRing.end());
  return snapshot;
}

bool hasPanel(const char* title) {
  if (!title) return false;
  for (const std::string& s : gLayout.availablePanels) {
    if (s == title) return true;
  }
  return false;
}

i32 panelCount() {
  i32 total = 0;
  for (const DockSlot& s : gLayout.slots) {
    total += static_cast<i32>(s.tabs.size());
  }
  return total;
}

void draw(FrameContext& ctx) {
  if (gFrameActive) return;
  gFrameActive = true;

  static f32 startS = 0.0f;
  static auto t0 = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  const f32 nowS = startS + std::chrono::duration<f32>(now - t0).count();
  if (startS == 0.0f) startS = nowS;

  gFrame = buildFrame(nowS);
  beginFrame(gFrame);

  {
    std::lock_guard<std::mutex> lk(gLogMutex);
    for (const std::string& line : ctx.scene.logLines) {
      gLogRing.push_back(line);
      if (gLogRing.size() > kLogMax) gLogRing.pop_front();
    }
  }

  layoutDock(gFrame.viewport, gLayout);
  drawDockChrome(gLayout);

  for (const ActivePanel& ap : gActivePanels) {
    if (ap.id == "Object Tree") renderObjectTree(ap, ctx);
    else if (ap.id == "Property Sheet") renderPropertySheet(ap, ctx);
    else if (ap.id == "Scene View" || ap.id == "Game View") renderScene(ap, ctx);
    else if (ap.id == "Toolbar") renderToolbar(ap, ctx);
    else if (ap.id == "Log") renderLog(ap, ctx);
    else renderDefault(ap);
  }

  endFrame();

  gDrawCmds.clear();
  gActivePanels.clear();
  gFrameActive = false;
}

}  // namespace kimia::ui

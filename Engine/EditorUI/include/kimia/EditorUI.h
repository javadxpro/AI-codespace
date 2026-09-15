// KIMIA EditorUI — public surface.
//
// A native, GLES-rendered, immediate-mode UI toolkit designed for the KIMIA
// editor on Android (and later Windows). The toolkit owns:
//   * the theme (Dark Pro)
//   * the bitmap font (existing 5x7 ASCII)
//   * a panel system with dockable tabs and splitters
//   * an immediate-mode widget set (button, label, slider, ...)
//
// The host wires three things:
//   1. feed touch/keyboard events via `submitTouch()` / `submitPointer()`
//   2. draw every frame with `draw()` (called inside the GLES render loop)
//   3. read panel state (object tree, property sheet) and forward commands
//      back to the engine (WorldEditor).
//
// Layout is computed every frame from the current panel arrangement and the
// surface size; nothing is retained across frames except the docking layout
// (which IS user state) and per-widget text input cursors.
#pragma once

#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Forward declarations so widgets/panels can push draw cmds via these.
namespace kimia::ui {

struct DrawCmd;
struct ActivePanel;

namespace internal {
// Called by Widget / Panel to push draw commands and report active panels.
void pushDrawCmd(const DrawCmd& cmd);
void setActivePanels(std::vector<ActivePanel> panels);
}  // namespace internal

}  // namespace kimia::ui

namespace kimia::ui {

struct Color {
  f32 r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
  constexpr Color() = default;
  constexpr Color(f32 r_, f32 g_, f32 b_, f32 a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
};

// --- Widget primitives ---------------------------------------------------
//
// Widgets push DrawCmds onto a per-frame list; EditorUI owns the GL state
// that consumes them at endFrame(). Kept minimal so Phase 1 stays small.
struct Rect {
  f32 x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  constexpr Rect() = default;
  constexpr Rect(f32 x_, f32 y_, f32 w_, f32 h_) : x(x_), y(y_), w(w_), h(h_) {}
  constexpr bool contains(f32 px, f32 py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
  }
  constexpr f32 right() const { return x + w; }
  constexpr f32 bottom() const { return y + h; }
};

// DrawCmd / ActivePanel — defined in the EditorUI namespace itself so
// Widget.cpp and Panel.cpp can build them and pass them through the
// internal helpers below.
enum class Glyph : std::uint16_t;

enum class DrawKind { Rect, Glyph };

struct DrawCmd {
  DrawKind kind = DrawKind::Rect;
  Rect rect;
  f32 corner = 0.0f;
  Color color;
  Glyph glyph;
  char ascii = 0;
  i32 scale = 1;
};

struct ActivePanel {
  std::string id;
  Rect rect;
  Rect contentRect;
};

// --- Touch / pointer events (host → EditorUI) -----------------------------
//
// EditorUI treats every pointer as a finger (or mouse button) with a stable
// id. The host forwards raw MotionEvents as they arrive; EditorUI turns them
// into UI events (tap, double-tap, long-press, drag, pinch, scroll).
enum class PointerAction { Down, Move, Up, Cancel };

struct PointerEvent {
  i32 id = 0;             // stable per-pointer id (slot for fingers)
  PointerAction action = PointerAction::Down;
  f32 x = 0.0f;           // pixels in surface space (origin top-left)
  f32 y = 0.0f;
};

// --- UI commands (EditorUI → host / engine) -------------------------------
//
// The UI does not own a world; the host feeds it entity lists and reads back
// commands (select, transform, color, etc.). This keeps the engine as the
// single source of truth, just like the desktop WorldEditorApp.
struct EntityRef {
  std::string name;
  f32 posX = 0.0f, posY = 0.0f, posZ = 0.0f;
  f32 scaleX = 0.5f, scaleY = 0.5f, scaleZ = 0.5f;
  f32 rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f, rotW = 1.0f;
  f32 colorR = 1.0f, colorG = 1.0f, colorB = 1.0f;
  f32 rough = 0.5f, metal = 0.0f;
  bool locked = false;     // true for Player/Ball/Ground reserved names
};

enum class UiCommandKind {
  None,
  SelectEntity,         // payload: name (empty = clear)
  ClearSelection,
  BeginDragEntity,      // payload: name
  DragEntityTo,         // payload: name + screen delta (we project to ground)
  EndDragEntity,
  SetPosition,          // payload: name + new (x,y,z)
  SetRotation,          // payload: name + new quat
  SetScale,             // payload: name + new (x,y,z)
  SetColor,             // payload: name + new (r,g,b)
  SetRoughness,         // payload: name + new roughness
  SetMetalness,         // payload: name + new metalness
  CreateCube,
  CreateSphere,
  CreatePlane,
  CreatePlayer,
  CreateBall,
  DeleteSelected,
  DuplicateSelected,
  SaveScene,            // payload: filename (empty = current)
  LoadScene,            // payload: filename
  PlayPressed,
  PausePressed,
  StepPressed,
  StopPressed,
  ToolChanged,          // payload: int (0..3 = Select/Move/Rotate/Scale)
};

struct UiCommand {
  UiCommandKind kind = UiCommandKind::None;
  std::string name;        // entity / file
  f32 x = 0.0f, y = 0.0f, z = 0.0f;
  f32 r = 0.0f, g = 0.0f, b = 0.0f;
  f32 scalar = 0.0f;       // roughness / metalness / etc.
  i32 tool = 0;
};

// --- Scene snapshot fed to the editor each frame -------------------------
struct SceneSnapshot {
  std::vector<EntityRef> entities;
  std::vector<std::string> selectedNames;   // current selection from the engine
  bool valid = false;
  bool playing = false;
  bool paused = false;
  std::string scenePath;                   // e.g. "scenes/main.kimia"
  std::vector<std::string> sceneFiles;     // for the Library dropdown
  std::vector<std::string> logLines;       // recent console lines
};

// --- Frame context -------------------------------------------------------
struct FrameContext {
  SceneSnapshot scene;                     // in: engine state
  std::vector<UiCommand> commands;         // out: commands to apply this frame
  bool wantCaptureKeyboard = false;        // out: a textfield wants keys
};

// --- Public API ----------------------------------------------------------

// One-time setup: compiles shaders and uploads the font atlas into the
// provided GL functions table. Returns false on failure (no GL yet).
bool initialize(std::string& error);

// Tear down GL resources. Safe to call multiple times.
void shutdown();

// Resize the UI for a new surface size. Call from nativeSurfaceChanged().
void resize(i32 width, i32 height);

// Forward a pointer event. EditorUI remembers active pointers and computes
// gestures from them (tap, drag, pinch, long-press).
void submitPointer(const PointerEvent& ev);

// Forward a key event (Bluetooth keyboard support).
struct KeyEvent {
  i32 key = 0;          // Android KeyEvent.KEYCODE_* or ascii
  i32 action = 0;       // 0=down, 1=up
};
void submitKey(const KeyEvent& ev);

// Draw the editor UI for the current frame. `scene` is the engine state
// at the moment of the frame; `ctx.commands` is appended with any commands
// the UI wants executed before the next frame.
void draw(FrameContext& ctx);

// Test/access helpers (Phase 1): the Object Tree and Property Sheet only
// need a primitive in/out API for tests. The panels read `ctx.scene`.
bool hasPanel(const char* title);
i32 panelCount();

// Recent log: log() pushes a line (for tests + future Console panel use).
void log(const std::string& line);
const std::vector<std::string>& logLines();

// Direct access to the persistent dock layout (used by tests + the
// application bootstrap to seed panels). Owning this here keeps Panel.cpp
// free of static state.
struct DockLayout;
DockLayout& layout();

// Drain the per-frame draw-command buffer into a copy the host can
// rasterise. After this call the buffer is empty. The CPU-only
// RasterBridge uses this on Phase 1; the GL path will replace it.
std::vector<DrawCmd> takeDrawCmds();

}  // namespace kimia::ui

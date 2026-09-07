#pragma once

#include <kimia/Key.h>
#include <kimia/Types.h>

namespace kimia {

enum class MouseButton : u8 {
  Left,
  Right,
  Middle,
  Count,
};

enum class GamepadButton : u8 {
  A,
  B,
  X,
  Y,
  LeftShoulder,
  RightShoulder,
  Back,
  Start,
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
  Count,
};

enum class GamepadAxis : u8 {
  LeftX,
  LeftY,
  RightX,
  RightY,
  LeftTrigger,
  RightTrigger,
  Count,
};

// Per-frame input state. `down` is a LEVEL (held), `pressed`/`released` are
// EDGES latched on transitions and cleared by endFrame(). lookX/lookY/zoom are
// accumulated deltas, also cleared by endFrame(). Keyboard, mouse and
// controller state share one frame lifetime so native and remote input can
// feed the same game action map.
class InputState {
public:
  InputState() = default;

  // Latches the held level; a false->true transition sets a pressed edge,
  // true->false sets a released edge.
  void setKeyDown(Key key, bool down);
  // Latches only a pressed edge without touching the held level (tap).
  void tap(Key key);

  bool down(Key key) const;
  bool pressed(Key key) const;
  bool released(Key key) const;

  void setMouseButton(MouseButton button, bool down);
  bool mouseDown(MouseButton button) const;
  bool mousePressed(MouseButton button) const;
  bool mouseReleased(MouseButton button) const;

  void setGamepadButton(GamepadButton button, bool down);
  bool gamepadDown(GamepadButton button) const;
  bool gamepadPressed(GamepadButton button) const;
  bool gamepadReleased(GamepadButton button) const;

  void setGamepadAxis(GamepadAxis axis, f64 value);
  f64 gamepadAxis(GamepadAxis axis) const;

  void addLook(f64 dx, f64 dy) {
    lookX += dx;
    lookY += dy;
  }
  void addZoom(f64 dz) { zoom += dz; }

  // Clears all held device levels when a native window loses focus, avoiding
  // a stuck movement key or controller button after Alt-Tab.
  void clearHeld();
  void endFrame();

  f64 lookX = 0.0;
  f64 lookY = 0.0;
  f64 zoom = 0.0;

private:
  bool held_[static_cast<usize>(Key::Count)] = {};
  bool pressed_[static_cast<usize>(Key::Count)] = {};
  bool released_[static_cast<usize>(Key::Count)] = {};
  bool mouseHeld_[static_cast<usize>(MouseButton::Count)] = {};
  bool mousePressed_[static_cast<usize>(MouseButton::Count)] = {};
  bool mouseReleased_[static_cast<usize>(MouseButton::Count)] = {};
  bool gamepadHeld_[static_cast<usize>(GamepadButton::Count)] = {};
  bool gamepadPressed_[static_cast<usize>(GamepadButton::Count)] = {};
  bool gamepadReleased_[static_cast<usize>(GamepadButton::Count)] = {};
  f64 gamepadAxes_[static_cast<usize>(GamepadAxis::Count)] = {};
};

}  // namespace kimia

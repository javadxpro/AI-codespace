#pragma once

#include <kimia/Key.h>
#include <kimia/Types.h>

namespace kimia {

// Per-frame input state. `down` is a LEVEL (held), `pressed`/`released` are
// EDGES latched on transitions and cleared by endFrame(). lookX/lookY/zoom are
// accumulated deltas, also cleared by endFrame().
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

  void addLook(f64 dx, f64 dy) {
    lookX += dx;
    lookY += dy;
  }
  void addZoom(f64 dz) { zoom += dz; }

  void endFrame();

  f64 lookX = 0.0;
  f64 lookY = 0.0;
  f64 zoom = 0.0;

private:
  bool held_[static_cast<usize>(Key::Count)] = {};
  bool pressed_[static_cast<usize>(Key::Count)] = {};
  bool released_[static_cast<usize>(Key::Count)] = {};
};

}  // namespace kimia

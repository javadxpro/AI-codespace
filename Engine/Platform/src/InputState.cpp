#include <kimia/InputState.h>

#include <cstring>

namespace kimia {

namespace {

usize indexOf(Key key) { return static_cast<usize>(key); }
usize indexOf(MouseButton button) { return static_cast<usize>(button); }
usize indexOf(GamepadButton button) { return static_cast<usize>(button); }
usize indexOf(GamepadAxis axis) { return static_cast<usize>(axis); }

template <typename Enum>
void setButton(Enum button, bool down, bool* held, bool* pressed, bool* released) {
  const usize index = indexOf(button);
  if (down && !held[index]) pressed[index] = true;
  if (!down && held[index]) released[index] = true;
  held[index] = down;
}

template <typename Enum>
bool buttonValue(Enum button, const bool* values) {
  return values[indexOf(button)];
}

}  // namespace

std::optional<Key> keyFromName(const std::string& name) {
  if (name.size() == 1U) {
    const char c = name[0];
    if (c >= 'a' && c <= 'z') return static_cast<Key>(static_cast<i32>(Key::A) + (c - 'a'));
    if (c >= 'A' && c <= 'Z') return static_cast<Key>(static_cast<i32>(Key::A) + (c - 'A'));
    if (c >= '0' && c <= '9') return static_cast<Key>(static_cast<i32>(Key::Num0) + (c - '0'));
  }
  if (name == "up") return Key::Up;
  if (name == "down") return Key::Down;
  if (name == "left") return Key::Left;
  if (name == "right") return Key::Right;
  if (name == "return" || name == "enter") return Key::Return;
  if (name == "space") return Key::Space;
  if (name == "shift") return Key::Shift;
  if (name == "escape" || name == "esc") return Key::Escape;
  if (name == "tab") return Key::Tab;
  if (name == "backspace") return Key::Backspace;
  if (name == "num0") return Key::Num0;
  if (name == "num1") return Key::Num1;
  if (name == "num2") return Key::Num2;
  if (name == "num3") return Key::Num3;
  if (name == "num4") return Key::Num4;
  if (name == "num5") return Key::Num5;
  if (name == "num6") return Key::Num6;
  if (name == "num7") return Key::Num7;
  if (name == "num8") return Key::Num8;
  if (name == "num9") return Key::Num9;
  return std::nullopt;
}

void InputState::setKeyDown(Key key, bool down) {
  setButton(key, down, held_, pressed_, released_);
}

void InputState::tap(Key key) { pressed_[indexOf(key)] = true; }

bool InputState::down(Key key) const { return buttonValue(key, held_); }
bool InputState::pressed(Key key) const { return buttonValue(key, pressed_); }
bool InputState::released(Key key) const { return buttonValue(key, released_); }

void InputState::setMouseButton(MouseButton button, bool down) {
  setButton(button, down, mouseHeld_, mousePressed_, mouseReleased_);
}

bool InputState::mouseDown(MouseButton button) const { return buttonValue(button, mouseHeld_); }
bool InputState::mousePressed(MouseButton button) const { return buttonValue(button, mousePressed_); }
bool InputState::mouseReleased(MouseButton button) const { return buttonValue(button, mouseReleased_); }

void InputState::setGamepadButton(GamepadButton button, bool down) {
  setButton(button, down, gamepadHeld_, gamepadPressed_, gamepadReleased_);
}

bool InputState::gamepadDown(GamepadButton button) const { return buttonValue(button, gamepadHeld_); }
bool InputState::gamepadPressed(GamepadButton button) const {
  return buttonValue(button, gamepadPressed_);
}
bool InputState::gamepadReleased(GamepadButton button) const {
  return buttonValue(button, gamepadReleased_);
}

void InputState::setGamepadAxis(GamepadAxis axis, f64 value) { gamepadAxes_[indexOf(axis)] = value; }

f64 InputState::gamepadAxis(GamepadAxis axis) const { return gamepadAxes_[indexOf(axis)]; }

void InputState::clearHeld() {
  for (usize i = 0; i < static_cast<usize>(Key::Count); ++i) {
    setKeyDown(static_cast<Key>(i), false);
  }
  for (usize i = 0; i < static_cast<usize>(MouseButton::Count); ++i) {
    setMouseButton(static_cast<MouseButton>(i), false);
  }
  for (usize i = 0; i < static_cast<usize>(GamepadButton::Count); ++i) {
    setGamepadButton(static_cast<GamepadButton>(i), false);
  }
  std::memset(gamepadAxes_, 0, sizeof(gamepadAxes_));
}

void InputState::endFrame() {
  std::memset(pressed_, 0, sizeof(pressed_));
  std::memset(released_, 0, sizeof(released_));
  std::memset(mousePressed_, 0, sizeof(mousePressed_));
  std::memset(mouseReleased_, 0, sizeof(mouseReleased_));
  std::memset(gamepadPressed_, 0, sizeof(gamepadPressed_));
  std::memset(gamepadReleased_, 0, sizeof(gamepadReleased_));
  lookX = 0.0;
  lookY = 0.0;
  zoom = 0.0;
}

}  // namespace kimia

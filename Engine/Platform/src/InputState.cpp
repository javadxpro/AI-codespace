#include <kimia/InputState.h>

#include <cstring>

namespace kimia {

namespace {

usize indexOf(Key key) { return static_cast<usize>(key); }

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
  const usize index = indexOf(key);
  if (down && !held_[index]) pressed_[index] = true;
  if (!down && held_[index]) released_[index] = true;
  held_[index] = down;
}

void InputState::tap(Key key) { pressed_[indexOf(key)] = true; }

bool InputState::down(Key key) const { return held_[indexOf(key)]; }
bool InputState::pressed(Key key) const { return pressed_[indexOf(key)]; }
bool InputState::released(Key key) const { return released_[indexOf(key)]; }

void InputState::endFrame() {
  std::memset(pressed_, 0, sizeof(pressed_));
  std::memset(released_, 0, sizeof(released_));
  lookX = 0.0;
  lookY = 0.0;
  zoom = 0.0;
}

}  // namespace kimia

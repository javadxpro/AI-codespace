#pragma once

#include <optional>
#include <string>

namespace kimia {

enum class Key {
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
  Up, Down, Left, Right,
  Return, Space, Shift, Escape, Tab, Backspace,
  Count,
};

// Maps a web/terminal key name ("a", "num1", "space", "return", ...) to a Key.
std::optional<Key> keyFromName(const std::string& name);

}  // namespace kimia

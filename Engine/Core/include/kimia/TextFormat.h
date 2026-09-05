#pragma once

#include <kimia/Types.h>

#include <cstdio>
#include <string>

namespace kimia {

// Shared rules for KIMIA's line-based text formats (world metadata, game
// profiles): fixed 6-decimal numbers (save -> load -> save is byte-identical)
// and single-line escaping of free text (`\` -> `\\`, newline -> `\n`).

inline std::string formatFixed6(f64 value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.6f", value);
  return buffer;
}

// Strict number parse: the whole token must be consumed.
inline bool parseF64Token(const std::string& token, f64& out) {
  if (token.empty()) return false;
  try {
    usize consumed = 0;
    out = std::stod(token, &consumed);
    return consumed == token.size();
  } catch (...) {
    return false;
  }
}

inline std::string escapeLineText(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (const char c : text) {
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '\n') {
      out += "\\n";
    } else {
      out += c;
    }
  }
  return out;
}

inline std::string unescapeLineText(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (usize i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 1U < text.size()) {
      ++i;
      out += text[i] == 'n' ? '\n' : text[i];
    } else {
      out += text[i];
    }
  }
  return out;
}

}  // namespace kimia

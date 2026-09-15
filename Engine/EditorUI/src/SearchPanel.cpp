// SearchPanel implementation — see SearchPanel.h.
#include <kimia/SearchPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <algorithm>

namespace kimia::ui {

namespace {

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

}  // namespace

bool searchMatch(const std::string& needle, const std::string& hay) {
  if (needle.empty()) return true;
  const std::string n = toLower(needle);
  const std::string h = toLower(hay);
  return h.find(n) != std::string::npos;
}

std::vector<usize> searchFilter(const std::string& query,
                                const std::vector<std::string>& names) {
  std::vector<usize> out;
  out.reserve(names.size());
  for (usize i = 0; i < names.size(); ++i) {
    if (searchMatch(query, names[i])) out.push_back(i);
  }
  return out;
}

void drawSearchPanel(const Rect& rect, const std::string& query,
                     const std::string& placeholder) {
  using namespace theme;
  drawRect(rect, kPanelAlt, 2.0f);

  // Magnifying-glass glyph as a left-edge decoration.
  drawText("?", rect.x + 4.0f, rect.y + 4.0f, 1, kTextMuted);

  if (query.empty()) {
    // Draw the placeholder hint in dim text.
    drawText(placeholder.c_str(),
             rect.x + 14.0f, rect.y + 4.0f, 1, kTextDim);
  } else {
    // Draw the live query.
    drawText(query.c_str(),
             rect.x + 14.0f, rect.y + 4.0f, 1, kText);
    // Trailing caret marker so the user knows the input is live.
    const f32 caretX = rect.x + 14.0f +
                       static_cast<f32>(query.size()) * 6.0f;
    drawRect({caretX, rect.y + 3.0f, 1.0f, rect.h - 6.0f},
             kAccent, 0.0f);
  }
}

}  // namespace kimia::ui

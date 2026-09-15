#include <kimia/CheatSheetPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cctype>

namespace kimia::ui {

namespace {
bool ciContains(const std::string& s, const std::string& needle) {
  if (needle.empty()) return true;
  auto lower = [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  };
  std::string ls(s.size(), 0);
  for (std::size_t i = 0; i < s.size(); ++i) ls[i] = lower(s[i]);
  std::string ln(needle.size(), 0);
  for (std::size_t i = 0; i < needle.size(); ++i) ln[i] = lower(needle[i]);
  return ls.find(ln) != std::string::npos;
}
}

void drawCheatSheetPanel(const Rect& rect,
                         const std::vector<CheatEntry>& entries,
                         i32 scrollY,
                         const std::string& searchQuery) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Search bar at the top.
  const f32 searchH = 20.0f;
  drawRect({rect.x + 4.0f, rect.y + 4.0f,
            rect.w - 8.0f, searchH}, kPanelAlt, 2.0f);
  drawText(searchQuery.empty() ? "Search…" : searchQuery.c_str(),
           rect.x + 10.0f, rect.y + 8.0f, 1,
           searchQuery.empty() ? kTextDim : kText);

  // Filtered entries.
  pushClip(rect);
  const f32 startY = rect.y + searchH + 8.0f -
                     static_cast<f32>(scrollY);
  constexpr f32 rowH = 26.0f;
  i32 visible = 0;
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (!searchQuery.empty()) {
      const bool m = ciContains(entries[i].trigger, searchQuery) ||
                     ciContains(entries[i].description, searchQuery);
      if (!m) continue;
    }
    const f32 y = startY + static_cast<float>(visible) * rowH;
    if (y + rowH < rect.y + searchH + 8.0f) { ++visible; continue; }
    if (y > rect.y + rect.h) break;
    drawText(entries[i].action.c_str(),
             rect.x + 6.0f, y + 4.0f, 1, kAccent);
    drawText(entries[i].description.c_str(),
             rect.x + 80.0f, y + 4.0f, 1, kText);
    ++visible;
  }
  popClip();
}

}

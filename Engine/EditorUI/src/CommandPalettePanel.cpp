#include <kimia/CommandPalettePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>
#include <cmath>

namespace kimia::ui {

namespace {
std::string lower(const std::string& s) {
  std::string o;
  o.reserve(s.size());
  for (char c : s) o.push_back(static_cast<char>(std::tolower(c)));
  return o;
}
i32 matchScore(const std::string& text, const std::string& q) {
  // Simple substring match. +1 for prefix.
  if (q.empty()) return 0;
  const std::string lt = lower(text);
  const std::string lq = lower(q);
  const auto p = lt.find(lq);
  if (p == std::string::npos) return -1;
  return 100 - static_cast<i32>(p);
}
}

void drawCommandPalettePanel(const Rect& rect,
                             const std::string& query,
                             const std::vector<PaletteCommand>& allCommands,
                             i32 selectedIndex) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Command Palette", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Input box.
  drawRect({rect.x + 4.0f, rect.y + 18.0f,
            rect.w - 8.0f, 18.0f},
           kPanelAlt, 2.0f);
  const std::string p = "> " + query;
  drawText(p.c_str(),
           rect.x + 8.0f, rect.y + 22.0f, 1,
           query.empty() ? kTextMuted : kText);

  // Filtered results.
  struct Scored {
    i32 idx;
    i32 score;
  };
  std::vector<Scored> results;
  for (std::size_t i = 0; i < allCommands.size(); ++i) {
    const i32 s1 = matchScore(allCommands[i].label, query);
    const i32 s2 = matchScore(allCommands[i].category, query);
    const i32 s = std::max(s1, s2);
    if (s < 0) continue;
    results.push_back({static_cast<i32>(i), s});
  }
  std::sort(results.begin(), results.end(),
            [](const Scored& a, const Scored& b) {
              return a.score > b.score;
            });

  pushClip(rect);
  constexpr f32 rowH = 20.0f;
  const f32 listTop = rect.y + 42.0f;
  const std::size_t maxShown = std::min<std::size_t>(results.size(),
      static_cast<std::size_t>((rect.h - 50.0f) / rowH));
  for (std::size_t i = 0; i < maxShown; ++i) {
    const auto& cmd = allCommands[results[i].idx];
    const f32 y = listTop + static_cast<float>(i) * rowH;
    const bool sel = (static_cast<i32>(i) == selectedIndex);
    if (sel) {
      drawRect({rect.x + 4.0f, y,
                rect.w - 8.0f, rowH - 2.0f},
               kAccent, 0.0f);
    }
    drawText(cmd.label.c_str(),
             rect.x + 8.0f, y + 4.0f, 1,
             sel ? kAccentHot : kText);
    drawText(cmd.category.c_str(),
             rect.x + 140.0f, y + 4.0f, 1, kTextMuted);
    drawText(cmd.shortcut.c_str(),
             rect.x + rect.w - 60.0f, y + 4.0f, 1, kAccent);
  }
  popClip();
}

}

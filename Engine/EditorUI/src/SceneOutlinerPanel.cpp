#include <kimia/SceneOutlinerPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>
#include <cctype>

namespace kimia::ui {

namespace {
std::string lower(const std::string& s) {
  std::string r;
  r.reserve(s.size());
  for (char c : s) r.push_back(static_cast<char>(std::tolower(c)));
  return r;
}
bool matches(const std::string& text, const std::string& q) {
  if (q.empty()) return true;
  return lower(text).find(lower(q)) != std::string::npos;
}
}

void drawSceneOutlinerPanel(const Rect& rect,
                            const std::vector<OutlinerEntry>& entries,
                            const std::string& filter,
                            i32 selectedIndex,
                            i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Scene Outliner", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Filter field.
  drawRect({rect.x + 4.0f, rect.y + 18.0f,
            rect.w - 8.0f, 18.0f},
           kPanelAlt, 2.0f);
  drawText(filter.empty() ? "Filter..." : filter.c_str(),
           rect.x + 8.0f, rect.y + 22.0f, 1,
           filter.empty() ? kTextMuted : kText);

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  const f32 listTop = rect.y + 42.0f;
  const f32 startY = listTop - static_cast<f32>(scrollY);
  f32 y = startY;
  i32 visibleIdx = 0;
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (!matches(entries[i].name, filter) &&
        !matches(entries[i].type, filter)) {
      continue;
    }
    if (y + rowH < listTop) { y += rowH; visibleIdx++; continue; }
    if (y > rect.y + rect.h) break;
    const bool sel = (visibleIdx == selectedIndex);
    if (sel) {
      drawRect({rect.x + 4.0f, y,
                rect.w - 8.0f, rowH - 2.0f},
               kAccent, 0.0f);
    }
    drawText(entries[i].name.c_str(),
             rect.x + 8.0f, y + 2.0f, 1,
             sel ? kAccentHot :
                 (entries[i].isHidden ? kTextMuted : kText));
    drawText(entries[i].type.c_str(),
             rect.x + 100.0f, y + 2.0f, 1, kTextMuted);
    if (entries[i].isStatic) {
      drawText("S",
               rect.x + rect.w - 18.0f, y + 2.0f, 1, kWarning);
    }
    y += rowH;
    visibleIdx++;
  }
  popClip();
}

}

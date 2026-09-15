#include <kimia/HelpPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

namespace {
bool matches(const std::string& text, const std::string& filter) {
  if (filter.empty()) return true;
  std::string lowerText;
  lowerText.reserve(text.size());
  for (char c : text) lowerText.push_back(static_cast<char>(std::tolower(c)));
  std::string lowerFilter;
  lowerFilter.reserve(filter.size());
  for (char c : filter) lowerFilter.push_back(static_cast<char>(std::tolower(c)));
  return lowerText.find(lowerFilter) != std::string::npos;
}
}

void drawHelpPanel(const Rect& rect,
                   const std::vector<HelpEntry>& entries,
                   const std::string& filter,
                   i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Keyboard Shortcuts", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Filter input.
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
  std::string lastCategory;
  i32 visibleIdx = 0;

  for (std::size_t i = 0; i < entries.size(); ++i) {
    const std::string& cat = entries[i].category;
    const std::string& sc  = entries[i].shortcut;
    const std::string& desc = entries[i].description;
    if (!matches(sc, filter) && !matches(desc, filter) && !matches(cat, filter)) {
      continue;
    }

    // Category header.
    if (cat != lastCategory && !cat.empty()) {
      if (y > rect.y + rect.h) break;
      drawText(cat.c_str(),
               rect.x + 4.0f, y + 2.0f, 1, kAccent);
      lastCategory = cat;
      y += rowH;
      visibleIdx = 0;
    }

    if (y + rowH < listTop) { y += rowH; continue; }
    if (y > rect.y + rect.h) break;

    drawText(sc.c_str(),
             rect.x + 8.0f, y + 2.0f, 1, kAccentHot);
    drawText(desc.c_str(),
             rect.x + 100.0f, y + 2.0f, 1, kText);
    y += rowH;
    visibleIdx++;
    (void)visibleIdx;
  }
  popClip();
}

}

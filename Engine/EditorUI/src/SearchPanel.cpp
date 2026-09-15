#include <kimia/SearchPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
Color kindColor(const std::string& k) {
  using namespace theme;
  if (k == "Model")   return kAccent;
  if (k == "Image")   return kSuccess;
  if (k == "Scene")   return kWarning;
  if (k == "Material")return kAccentHot;
  if (k == "Audio")   return Color{1.0f, 0.6f, 0.3f, 1.0f};
  return kTextMuted;
}
}

void drawSearchPanel(const Rect& rect,
                     const std::string& query,
                     const std::vector<SearchResult>& results,
                     i32 selectedIndex,
                     i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Search", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Query field.
  drawRect({rect.x + 4.0f, rect.y + 18.0f,
            rect.w - 8.0f, 18.0f},
           kPanelAlt, 2.0f);
  drawText(query.empty() ? "Type to search..." : query.c_str(),
           rect.x + 8.0f, rect.y + 22.0f, 1,
           query.empty() ? kTextMuted : kText);

  if (results.empty()) return;

  pushClip(rect);
  constexpr f32 rowH = 22.0f;
  const f32 listTop = rect.y + 40.0f;
  const f32 startY = listTop - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < results.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < listTop) continue;
    if (y > rect.y + rect.h) break;

    const bool sel = (static_cast<i32>(i) == selectedIndex);
    if (sel) {
      drawRect({rect.x + 4.0f, y,
                rect.w - 8.0f, rowH - 2.0f},
               kAccent, 0.0f);
    }

    // Kind color dot.
    drawRect({rect.x + 8.0f, y + 6.0f, 6.0f, 6.0f},
             kindColor(results[i].kind), 0.0f);
    drawText(results[i].name.c_str(),
             rect.x + 18.0f, y + 4.0f, 1,
             sel ? kAccentHot : kText);
    drawText(results[i].kind.c_str(),
             rect.x + 100.0f, y + 4.0f, 1, kTextMuted);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", results[i].score);
    drawText(buf,
             rect.x + rect.w - 24.0f, y + 4.0f, 1, kAccent);
  }
  popClip();
}

}

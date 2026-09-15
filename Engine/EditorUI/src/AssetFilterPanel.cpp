#include <kimia/AssetFilterPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
Color kindColor(const std::string& kind) {
  using namespace theme;
  if (kind == "Model") return {0.5f, 0.8f, 1.0f, 1.0f};
  if (kind == "Image") return {0.9f, 0.7f, 0.4f, 1.0f};
  if (kind == "Scene") return {0.6f, 1.0f, 0.6f, 1.0f};
  return kText;
}
}

void drawAssetFilterPanel(const Rect& rect,
                          const std::vector<AssetFilterEntry>& filters) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Filters", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[16];
  for (std::size_t i = 0; i < filters.size(); ++i) {
    if (y + rowH > rect.y + rect.h) break;

    drawText(filters[i].active ? "*" : " ",
             rect.x + 4.0f, y + 4.0f, 1,
             filters[i].active ? kSuccess : kTextDim);
    // Kind dot.
    drawRect({rect.x + 18.0f, y + 5.0f, 6.0f, 6.0f},
             kindColor(filters[i].kind), 1.0f);
    drawText(filters[i].label.c_str(),
             rect.x + 30.0f, y + 4.0f, 1,
             filters[i].active ? kText : kTextDim);
    std::snprintf(buf, sizeof(buf), "%d", filters[i].count);
    drawText(buf, rect.x + rect.w - 24.0f, y + 4.0f, 1, kTextMuted);
    y += rowH;
  }
}

}

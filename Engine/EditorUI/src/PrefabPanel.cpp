#include <kimia/PrefabPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawPrefabPanel(const Rect& rect,
                     const std::vector<PrefabEntry>& prefabs,
                     i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Prefabs", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  const f32 rowH = 20.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  char buf[32];
  for (std::size_t i = 0; i < prefabs.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    drawText(prefabs[i].name.c_str(),
             rect.x + 4.0f, y + 4.0f, 1, kText);

    std::snprintf(buf, sizeof(buf), "x%d",
                  prefabs[i].instanceCount);
    drawText(buf, rect.x + rect.w - 60.0f, y + 4.0f, 1, kAccent);

    std::snprintf(buf, sizeof(buf), "%.0fs ago",
                  static_cast<double>(prefabs[i].lastModifiedSec));
    drawText(buf, rect.x + rect.w - 30.0f, y + 4.0f, 1, kTextMuted);
  }
  popClip();
}

}

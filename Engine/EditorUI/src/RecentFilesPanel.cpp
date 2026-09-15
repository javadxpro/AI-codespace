#include <kimia/RecentFilesPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void formatAge(char* buf, usize n, f64 secAgo) {
  if (secAgo < 60.0) {
    std::snprintf(buf, n, "now");
  } else if (secAgo < 3600.0) {
    std::snprintf(buf, n, "%.0fm ago", secAgo / 60.0);
  } else if (secAgo < 86400.0) {
    std::snprintf(buf, n, "%.0fh ago", secAgo / 3600.0);
  } else {
    std::snprintf(buf, n, "%.0fd ago", secAgo / 86400.0);
  }
}
}

void drawRecentFilesPanel(const Rect& rect,
                          const std::vector<RecentFile>& files) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Recent", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 24.0f;
  const f32 startY = rect.y + 18.0f;
  for (std::size_t i = 0; i < files.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH > rect.y + rect.h) break;

    // Thumbnail swatch.
    drawRect({rect.x + 4.0f, y + 2.0f, 20.0f, rowH - 4.0f},
             kPanelAlt, 2.0f);
    drawText(files[i].thumbnail.c_str(),
             rect.x + 10.0f, y + 6.0f, 1, kAccent);
    // Path.
    drawText(files[i].path.c_str(),
             rect.x + 30.0f, y + 4.0f, 1, kText);
    // Age.
    char buf[16];
    formatAge(buf, sizeof(buf), files[i].lastOpenedSec);
    drawText(buf,
             rect.x + rect.w - 56.0f, y + 4.0f, 1, kTextMuted);
  }
  popClip();
}

}

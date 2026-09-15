#include <kimia/ProjectBrowserPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
Color kindColor(const std::string& k) {
  using namespace theme;
  if (k == "Model")    return kAccent;
  if (k == "Image")    return kSuccess;
  if (k == "Scene")    return kWarning;
  if (k == "Material") return kAccentHot;
  if (k == "Audio")    return Color{1.0f, 0.6f, 0.3f, 1.0f};
  if (k == "Font")     return Color{0.7f, 0.5f, 1.0f, 1.0f};
  return kTextMuted;
}
void formatSize(char* buf, usize n, u64 bytes) {
  if (bytes < 1024ULL) {
    std::snprintf(buf, n, "%llu B", (unsigned long long)bytes);
  } else if (bytes < 1024ULL * 1024) {
    std::snprintf(buf, n, "%.1f KB", bytes / 1024.0);
  } else if (bytes < 1024ULL * 1024 * 1024) {
    std::snprintf(buf, n, "%.1f MB", bytes / (1024.0 * 1024.0));
  } else {
    std::snprintf(buf, n, "%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
  }
}
}

void drawProjectBrowserPanel(const Rect& rect,
                             const std::string& folder,
                             const std::vector<ProjectAsset>& assets,
                             i32 selectedIndex,
                             i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Project", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Folder bar.
  drawRect({rect.x + 4.0f, rect.y + 18.0f,
            rect.w - 8.0f, 18.0f},
           kPanelAlt, 2.0f);
  drawText(folder.c_str(),
           rect.x + 8.0f, rect.y + 22.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  const f32 listTop = rect.y + 42.0f;
  const f32 startY = listTop - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < assets.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < listTop) continue;
    if (y > rect.y + rect.h) break;
    const bool sel = (static_cast<i32>(i) == selectedIndex);
    if (sel) {
      drawRect({rect.x + 4.0f, y,
                rect.w - 8.0f, rowH - 2.0f},
               kAccent, 0.0f);
    }
    drawRect({rect.x + 8.0f, y + 4.0f, 8.0f, 8.0f},
             kindColor(assets[i].kind), 0.0f);
    drawText(assets[i].name.c_str(),
             rect.x + 22.0f, y + 2.0f, 1,
             sel ? kAccentHot : kText);
    char buf[16];
    formatSize(buf, sizeof(buf), assets[i].sizeBytes);
    drawText(buf,
             rect.x + rect.w - 50.0f, y + 2.0f, 1, kTextMuted);
  }
  popClip();
}

}

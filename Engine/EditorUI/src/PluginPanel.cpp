#include <kimia/PluginPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawPluginPanel(const Rect& rect,
                     const std::vector<PluginEntry>& plugins,
                     i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Plugins", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 22.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < plugins.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    // Enabled indicator.
    drawText(plugins[i].enabled ? "*" : " ",
             rect.x + 4.0f, y + 4.0f, 1,
             plugins[i].enabled ? kSuccess : kTextDim);
    // Name + version.
    drawText(plugins[i].name.c_str(),
             rect.x + 18.0f, y + 4.0f, 1,
             plugins[i].enabled ? kText : kTextMuted);
    drawText(plugins[i].version.c_str(),
             rect.x + rect.w - 90.0f, y + 4.0f, 1, kAccent);
    drawText(plugins[i].author.c_str(),
             rect.x + rect.w - 50.0f, y + 4.0f, 1, kTextMuted);

    if (plugins[i].hasUpdate) {
      drawRect({rect.x + rect.w - 12.0f, y + 6.0f, 4.0f, 4.0f},
               kWarning, 2.0f);
    }
  }
  popClip();
}

}

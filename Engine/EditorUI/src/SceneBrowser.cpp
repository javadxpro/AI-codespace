// SceneBrowser implementation — see SceneBrowser.h.
#include <kimia/SceneBrowser.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawSceneBrowser(const Rect& rect,
                      const std::vector<SceneEntry>& scenes,
                      i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Scenes", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  const f32 rowH = 18.0f;
  const f32 padX = 6.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < scenes.size(); ++i) {
    const f32 y = startY + static_cast<f32>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    // Highlight the current scene.
    if (scenes[i].currentScene) {
      drawRect({rect.x, y, rect.w, rowH}, kAccentDim, 0.0f);
    }
    // Dirty marker: an orange asterisk before the name.
    if (scenes[i].dirty) {
      drawText("*", rect.x + padX, y + 4.0f, 1, kWarning);
    }
    const Color textColor = scenes[i].currentScene ? kAccentHot : kText;
    drawText(scenes[i].name.c_str(),
             rect.x + padX + (scenes[i].dirty ? 8.0f : 0.0f),
             y + 4.0f, 1, textColor);
  }
  popClip();
}

}  // namespace kimia::ui

#include <kimia/KeyBindingsPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawKeyBindingsPanel(const Rect& rect,
                          const std::vector<KeyBinding>& bindings,
                          i32 scrollY,
                          const std::string& category) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Key Bindings", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Optional category filter chip.
  if (!category.empty()) {
    drawRect({rect.x + rect.w - 80.0f, rect.y + 2.0f,
              72.0f, 14.0f}, kAccentDim, 2.0f);
    drawText(category.c_str(),
             rect.x + rect.w - 76.0f, rect.y + 4.0f, 1, kAccentHot);
  }

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  std::string lastCat;
  for (std::size_t i = 0; i < bindings.size(); ++i) {
    if (!category.empty() && bindings[i].category != category) continue;
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    // Category header on change.
    if (bindings[i].category != lastCat) {
      drawText(bindings[i].category.c_str(),
               rect.x + 4.0f, y + 4.0f, 1, kAccent);
      lastCat = bindings[i].category;
    }

    drawText(bindings[i].action.c_str(),
             rect.x + 70.0f, y + 4.0f, 1, kText);
    drawText(bindings[i].keys.c_str(),
             rect.x + rect.w - 80.0f, y + 4.0f, 1,
             bindings[i].conflict ? kError : kAccent);
    if (bindings[i].conflict) {
      drawText("!",
               rect.x + rect.w - 8.0f, y + 4.0f, 1, kError);
    }
  }
  popClip();
}

}

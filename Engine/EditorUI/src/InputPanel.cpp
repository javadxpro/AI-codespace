#include <kimia/InputPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawInputPanel(const Rect& rect,
                    const std::vector<InputBinding>& bindings,
                    i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Input", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  constexpr f32 keyColW = 70.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < bindings.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    drawText(bindings[i].action.c_str(),
             rect.x + 4.0f, y + 4.0f, 1, kText);
    drawText(bindings[i].primaryKey.c_str(),
             rect.x + rect.w - keyColW * 2.0f, y + 4.0f,
             1, kAccent);
    drawText(bindings[i].secondaryKey.c_str(),
             rect.x + rect.w - keyColW, y + 4.0f,
             1, kTextMuted);
  }
  popClip();
}

}

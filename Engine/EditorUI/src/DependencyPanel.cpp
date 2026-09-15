#include <kimia/DependencyPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
Color kindTint(const std::string& kind) {
  using namespace theme;
  if (kind == "Material") return {0.9f, 0.5f, 0.2f, 1.0f};
  if (kind == "Texture")  return {0.5f, 0.9f, 0.5f, 1.0f};
  if (kind == "Model")    return {0.5f, 0.7f, 0.9f, 1.0f};
  if (kind == "Script")   return {0.9f, 0.9f, 0.5f, 1.0f};
  return kText;
}
}

void drawDependencyPanel(const Rect& rect,
                         const std::vector<Dependency>& deps,
                         i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Dependencies", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < deps.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    // Indent transitive dependencies.
    const f32 x = rect.x + (deps[i].direct ? 4.0f : 16.0f);
    // Kind dot.
    drawRect({x, y + 5.0f, 6.0f, 6.0f}, kindTint(deps[i].kind), 1.0f);
    drawText(deps[i].name.c_str(),
             x + 12.0f, y + 4.0f, 1,
             deps[i].direct ? kText : kTextMuted);
    drawText(deps[i].kind.c_str(),
             rect.x + rect.w - 60.0f, y + 4.0f, 1, kTextMuted);
  }
  popClip();
}

}

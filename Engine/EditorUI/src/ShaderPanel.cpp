#include <kimia/ShaderPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
const char* typeName(ShaderType t) {
  switch (t) {
    case ShaderType::Vertex:   return "VS";
    case ShaderType::Fragment: return "FS";
    case ShaderType::Compute:  return "CS";
  }
  return "?";
}
Color typeTint(ShaderType t) {
  using namespace theme;
  switch (t) {
    case ShaderType::Vertex:   return {0.5f, 0.8f, 1.0f, 1.0f};
    case ShaderType::Fragment: return {0.9f, 0.6f, 0.3f, 1.0f};
    case ShaderType::Compute:  return {0.6f, 1.0f, 0.6f, 1.0f};
  }
  return kText;
}
}

void drawShaderPanel(const Rect& rect,
                     const std::vector<ShaderEntry>& shaders,
                     i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Shaders", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  const f32 startY = rect.y + 18.0f - static_cast<f32>(scrollY);
  char buf[32];
  for (std::size_t i = 0; i < shaders.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH < rect.y + 18.0f) continue;
    if (y > rect.y + rect.h) break;

    drawText(typeName(shaders[i].type),
             rect.x + 4.0f, y + 4.0f, 1, typeTint(shaders[i].type));
    drawText(shaders[i].name.c_str(),
             rect.x + 24.0f, y + 4.0f, 1, kText);
    drawText(shaders[i].compiled ? "OK" : "ERR",
             rect.x + rect.w - 80.0f, y + 4.0f, 1,
             shaders[i].compiled ? kSuccess : kError);
    std::snprintf(buf, sizeof(buf), "%d ln", shaders[i].lineCount);
    drawText(buf, rect.x + rect.w - 50.0f, y + 4.0f, 1, kTextMuted);
    std::snprintf(buf, sizeof(buf), "%.1fms", shaders[i].compileMs);
    drawText(buf, rect.x + rect.w - 26.0f, y + 4.0f, 1, kTextMuted);

    if (!shaders[i].compiled && !shaders[i].error.empty()) {
      drawText(shaders[i].error.c_str(),
               rect.x + 24.0f, y + rowH - 4.0f, 1, kError);
    }
  }
  popClip();
}

}

// InspectorPanel implementation — see InspectorPanel.h.
#include <kimia/InspectorPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawInspectorPanel(const Rect& rect,
                        const std::string& title,
                        const std::vector<InspectorRow>& rows) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Title row.
  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText(title.c_str(), rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  constexpr f32 rowH = 16.0f;
  constexpr f32 labelW = 80.0f;
  const f32 startY = rect.y + 26.0f;
  pushClip(rect);
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const f32 y = startY + static_cast<f32>(i) * rowH;
    if (y + rowH > rect.y + rect.h) break;
    drawText(rows[i].label.c_str(),
             rect.x + 4.0f, y + 4.0f, 1, kTextMuted);
    drawText(rows[i].value.c_str(),
             rect.x + labelW, y + 4.0f, 1, rows[i].valueColor);
  }
  popClip();
}

}  // namespace kimia::ui

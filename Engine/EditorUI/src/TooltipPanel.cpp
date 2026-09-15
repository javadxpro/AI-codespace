// TooltipPanel implementation — see TooltipPanel.h.
#include <kimia/TooltipPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <algorithm>

namespace kimia::ui {

void drawTooltipPanel(f32 x, f32 y, const std::string& text) {
  using namespace theme;
  if (text.empty()) return;

  // Approximate width: 6 px per glyph (5x7 font + 1 px tracking).
  const f32 width = static_cast<f32>(text.size()) * 6.0f + 8.0f;
  const f32 height = 16.0f;
  const Rect bg{x, y, width, height};
  drawRect(bg, kPanel, 2.0f);
  drawText(text.c_str(), x + 4.0f, y + 4.0f, 1, kText);
  // Phase 5+ will also add a 1-pixel border so the tooltip stands
  // out against a matching panel.
  (void)std::max(0.0f, 0.0f);
}

}  // namespace kimia::ui

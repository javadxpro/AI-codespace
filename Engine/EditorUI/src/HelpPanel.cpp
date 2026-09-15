// HelpPanel implementation — see HelpPanel.h.
#include <kimia/HelpPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <kimia/Version.h>

namespace kimia::ui {

void drawHelpPanel(const Rect& rect) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Title bar.
  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText("KIMIA Editor — Shortcuts",
           rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  // Two columns: keys on the left, description on the right.
  const f32 padX = 12.0f;
  const f32 col1W = 120.0f;
  const f32 startY = rect.y + 28.0f;
  const f32 rowH = 14.0f;

  pushClip(rect);
  for (usize i = 0; i < sizeof(kShortcuts) / sizeof(kShortcuts[0]); ++i) {
    const f32 y = startY + static_cast<f32>(i) * rowH;
    if (y + rowH > rect.y + rect.h - 18.0f) break;
    drawText(kShortcuts[i].keys, rect.x + padX, y, 1, kAccent);
    drawText(kShortcuts[i].description,
             rect.x + padX + col1W, y, 1, kText);
  }
  popClip();

  // Bottom bar: engine version.
  const std::string ver = "v" + std::string(kEngineVersion);
  drawText(ver.c_str(),
           rect.x + rect.w - 60.0f,
           rect.y + rect.h - 14.0f,
           1, kTextDim);
}

}  // namespace kimia::ui

// VersionPanel implementation — see VersionPanel.h.
#include <kimia/VersionPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {

void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 80.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kTextMuted);
  drawText(value, row.x + labelW, row.y + 4.0f, 1, kText);
}

}  // namespace

void drawVersionPanel(const Rect& rect, const VersionInfo& info) {
  using namespace theme;
  drawRect(rect, kPanel, 4.0f);

  // Title row.
  drawRect({rect.x, rect.y, rect.w, 22.0f}, kTitlebar, 0.0f);
  drawText("Version Info", rect.x + 8.0f, rect.y + 6.0f, 1, kText);

  constexpr f32 rowH = 16.0f;
  f32 y = rect.y + 26.0f;

  drawRow({rect.x, y, rect.w, rowH}, "Engine", info.version);
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Built", info.buildDate);
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Commit", info.gitCommit);
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Device", info.targetDevice);
  y += rowH;

  // Hint at the bottom: tap-and-hold to copy.
  drawText("(hold to copy)",
           rect.x + 4.0f, rect.y + rect.h - 14.0f, 1, kTextDim);
}

}  // namespace kimia::ui

// DialogPanel implementation — see DialogPanel.h.
#include <kimia/DialogPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <algorithm>

namespace kimia::ui {

void drawDialogPanel(const Rect& rect, const DialogState& state) {
  using namespace theme;
  if (!state.open) return;

  // Dim the whole panel first (modal backdrop).
  drawRect(rect, {0.0f, 0.0f, 0.0f, 0.5f}, 0.0f);

  // The dialog itself is centered, sized to its content.
  const f32 dialogW = std::min(rect.w - 40.0f, 360.0f);
  const f32 dialogH = 120.0f;
  const Rect dialog{
      rect.x + (rect.w - dialogW) * 0.5f,
      rect.y + (rect.h - dialogH) * 0.5f,
      dialogW, dialogH};

  drawRect(dialog, kPanel, 4.0f);
  drawRect({dialog.x, dialog.y, dialog.w, 22.0f}, kTitlebar, 0.0f);
  drawText(state.title.c_str(), dialog.x + 8.0f, dialog.y + 6.0f, 1, kText);

  // Wrap the message into multiple lines based on the dialog width.
  // Phase 4+ uses a fixed wrap at ~40 chars per line; Phase 5+ will
  // measure glyphs properly.
  drawText(state.message.c_str(),
           dialog.x + 12.0f, dialog.y + 32.0f, 1, kText);

  // Buttons at the bottom — right-aligned.
  const f32 btnW = 80.0f;
  const f32 btnH = 24.0f;
  const f32 pad = 8.0f;
  f32 x = dialog.x + dialog.w - pad - btnW;
  const f32 by = dialog.y + dialog.h - btnH - pad;
  for (std::size_t i = 0; i < state.buttons.size() && i < 4; ++i) {
    if (button(state.buttons[i].label.c_str(), {x, by, btnW, btnH})) {
      // Phase 5+ will route the chosen id through a UiCommand.
    }
    x -= btnW + 4.0f;
  }
}

}  // namespace kimia::ui

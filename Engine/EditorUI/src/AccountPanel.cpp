#include <kimia/AccountPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 80.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
}
}

void drawAccountPanel(const Rect& rect, const AccountInfo& info) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Avatar.
  constexpr f32 avatar = 48.0f;
  drawRect({rect.x + 8.0f, rect.y + 8.0f, avatar, avatar},
           kAccent, 4.0f);
  drawText(info.avatarGlyph.empty() ? "?" : info.avatarGlyph.c_str(),
           rect.x + 24.0f, rect.y + 22.0f, 2, kAccentHot);

  // Username + email.
  drawText(info.username.c_str(),
           rect.x + 70.0f, rect.y + 12.0f, 1, kText);
  drawText(info.email.c_str(),
           rect.x + 70.0f, rect.y + 28.0f, 1, kTextMuted);
  // Status dot.
  drawRect({rect.x + rect.w - 14.0f, rect.y + 14.0f,
            6.0f, 6.0f},
           info.isOnline ? kSuccess : kTextDim, 3.0f);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 70.0f;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%d", info.projectCount);
  drawRow({rect.x, y, rect.w, rowH}, "Projects", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d", info.publishedGameCount);
  drawRow({rect.x, y, rect.w, rowH}, "Published", buf);
  y += rowH + 4.0f;

  if (button("Sign out", {rect.x + 8.0f, y,
                          rect.w - 16.0f, 22.0f})) {
  }
}

}

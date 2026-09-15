#include <kimia/AccountLoginPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void drawField(const Rect& rect, const char* label,
               const std::string& value, bool password) {
  using namespace theme;
  drawText(label, rect.x + 4.0f, rect.y + 4.0f, 1, kTextMuted);
  drawRect({rect.x + 70.0f, rect.y + 1.0f,
            rect.w - 74.0f, rect.h - 2.0f},
           kPanelAlt, 2.0f);
  std::string display = value;
  if (password && !value.empty()) {
    display.assign(value.size(), '*');
  }
  if (display.empty()) {
    drawText(password ? "(password)" : "(empty)",
             rect.x + 74.0f, rect.y + 4.0f, 1, kTextMuted);
  } else {
    drawText(display.c_str(),
             rect.x + 74.0f, rect.y + 4.0f, 1, kText);
  }
}
}

void drawAccountLoginPanel(const Rect& rect,
                           const std::string& username,
                           const std::string& password,
                           bool rememberMe,
                           const std::string& status) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Account Login", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 22.0f;
  f32 y = rect.y + 22.0f;

  drawField({rect.x, y, rect.w, rowH}, "User", username, false);
  y += rowH + 4.0f;
  drawField({rect.x, y, rect.w, rowH}, "Pass", password, true);
  y += rowH + 4.0f;

  bool rem = rememberMe;
  if (checkbox("Remember me", rem,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH + 6.0f;

  const Rect btn = {rect.x + 4.0f, y, rect.w - 8.0f, 26.0f};
  drawRect(btn, kAccent, 3.0f);
  drawText("Sign In", btn.x + btn.w * 0.5f - 18.0f, btn.y + 8.0f, 1, kAccentHot);
  y += 26.0f + 6.0f;

  if (!status.empty()) {
    drawText(status.c_str(),
             rect.x + 6.0f, y, 1,
             status.find("error") != std::string::npos
                 || status.find("fail") != std::string::npos
                 ? kError : kSuccess);
  }
}

}

#include <kimia/NotificationPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
Color kindColor(NotificationKind k) {
  using namespace theme;
  switch (k) {
    case NotificationKind::Info:    return kAccent;
    case NotificationKind::Warning: return kWarning;
    case NotificationKind::Error:   return kError;
    case NotificationKind::Success: return kSuccess;
  }
  return kText;
}
const char* kindGlyph(NotificationKind k) {
  switch (k) {
    case NotificationKind::Info:    return "i";
    case NotificationKind::Warning: return "!";
    case NotificationKind::Error:   return "x";
    case NotificationKind::Success: return "+";
  }
  return "?";
}
}

void drawNotificationPanel(const Rect& rect,
                           const std::vector<Notification>& notes) {
  using namespace theme;
  // The notifications stack at the top of the rect (so the rect
  // is sized by the caller to fit the longest expected message).
  const f32 itemH = 24.0f;
  const f32 padX = 4.0f;
  f32 y = rect.y;
  for (std::size_t i = 0; i < notes.size(); ++i) {
    if (y + itemH > rect.y + rect.h) break;
    const f32 w = std::min(rect.w,
                           static_cast<float>(notes[i].message.size()) * 6.0f +
                           30.0f);
    drawRect({rect.x, y, w, itemH}, kPanel, 2.0f);
    drawRect({rect.x, y, 4.0f, itemH}, kindColor(notes[i].kind), 0.0f);
    drawText(kindGlyph(notes[i].kind),
             rect.x + padX + 2.0f, y + 6.0f, 1, kindColor(notes[i].kind));
    drawText(notes[i].message.c_str(),
             rect.x + padX + 14.0f, y + 6.0f, 1, kText);
    y += itemH + 2.0f;
  }
}

}

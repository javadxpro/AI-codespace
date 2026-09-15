#pragma once
#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

enum class NotificationKind {
  Info,
  Warning,
  Error,
  Success,
};

struct Notification {
  std::string message;
  NotificationKind kind = NotificationKind::Info;
  f32 lifetimeSec = 3.0f;  // 0 = sticky
};

void drawNotificationPanel(const Rect& rect,
                           const std::vector<Notification>& notes);

}

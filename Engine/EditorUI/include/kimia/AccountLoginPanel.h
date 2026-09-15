#pragma once
#include "EditorUI.h"

namespace kimia::ui {

void drawAccountLoginPanel(const Rect& rect,
                           const std::string& username,
                           const std::string& password,
                           bool rememberMe,
                           const std::string& status);

}

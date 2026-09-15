#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class ThemeName { DarkPro, LightPro, Solarized, Monokai };

void drawThemePanel(const Rect& rect, ThemeName theme);

}

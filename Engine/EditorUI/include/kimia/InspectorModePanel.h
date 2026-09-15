#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class InspectorTab { Properties, Physics, Material, Particle, Script };

void drawInspectorModePanel(const Rect& rect, InspectorTab activeTab);

}

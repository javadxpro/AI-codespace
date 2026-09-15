#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class DebugDrawMode { Off, Wireframe, Normals, Contacts, All };

void drawPhysicsDebugPanel(const Rect& rect, DebugDrawMode mode);

}

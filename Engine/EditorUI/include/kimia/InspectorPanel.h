#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

enum class InspectorKind { Float, Int, Bool, String, Vec2, Vec3, Vec4, Color };

struct InspectorProp {
  std::string name;
  InspectorKind kind = InspectorKind::Float;
  f32 v0 = 0;
  f32 v1 = 0;
  f32 v2 = 0;
  f32 v3 = 0;
  std::string s;
  bool enabled = true;
};

void drawInspectorPanel(const Rect& rect,
                        const std::string& objectName,
                        const std::string& objectType,
                        const std::vector<InspectorProp>& props,
                        i32 scrollY);

}

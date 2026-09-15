#pragma once
#include "EditorUI.h"

namespace kimia::ui {

enum class GizmoMode { None, Move, Rotate, Scale };
enum class ViewportShading { Lit, Unlit, Wireframe, Albedo, Normals };

void drawViewportPanel(const Rect& rect,
                       GizmoMode gizmo,
                       ViewportShading shading,
                       bool playing,
                       const std::string& cameraName,
                       f32 gridSize,
                       bool showGrid,
                       bool showStats,
                       f32 zoom);

}

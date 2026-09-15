// InspectorPanel — a generic inspector that renders any number of
// labelled rows. Used by Phase 5+ to wire all the other panels
// (PropertySheet, PhysicsPanel, ParticlePanel) through a single
// render path. Phase 4+ exposes the type; Phase 5+ will start
// using it.
//
// Phase 4+ lands the API: a flat list of (label, value) pairs
// plus optional accent colour per row.
#pragma once

#include "EditorUI.h"
#include <string>
#include <vector>

namespace kimia::ui {

struct InspectorRow {
  std::string label;
  std::string value;
  Color valueColor{0.878f, 0.878f, 0.878f, 1.0f};  // theme::kText
};

void drawInspectorPanel(const Rect& rect,
                        const std::string& title,
                        const std::vector<InspectorRow>& rows);

}  // namespace kimia::ui

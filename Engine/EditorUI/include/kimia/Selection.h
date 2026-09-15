// Selection — multi-entity selection helpers used by the Object
// Tree and the Scene View. The editor keeps the canonical
// selection in std::vector<std::string> selectedNames (one entry
// per selected entity name); these helpers operate on that list.
//
// Phase 4+: shift-tap toggles membership in the selection list;
// box-select replaces the list with every entity whose screen-
// space bounding rect intersects the box. Both produce a single
// UiCommand of kind SelectEntity that the HostBridge translates
// into a WorldEditor operation.
#pragma once

#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

// Shift-tap: if `name` is already in `selection`, remove it; else
// add it. The result list is the new selection.
std::vector<std::string> shiftTapSelect(
    const std::vector<std::string>& selection,
    const std::string& name);

// Replace the entire selection with every entity whose screen-space
// rect intersects the given box. The screen rect is computed by the
// caller (Phase 5+: through the Scene View's projection pipeline).
std::vector<std::string> boxSelect(
    const std::vector<std::string>& allNames,
    const std::vector<Rect>& allScreenRects,
    const Rect& box);

// Plain tap: replace selection with a single entity (the standard
// Unity / Blender "click-to-select" behaviour).
std::vector<std::string> singleSelect(
    const std::vector<std::string>& /*prev*/,
    const std::string& name);

}  // namespace kimia::ui

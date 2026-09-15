#pragma once
#include "EditorUI.h"
#include "HierarchyPanel.h"
#include "SceneOutlinerPanel.h"
#include "ProjectBrowserPanel.h"
#include "InspectorPanel.h"
#include "ViewportPanel.h"
#include "ConsolePanel.h"
#include "MainMenuBarPanel.h"
#include "StatusBarPanel.h"
#include "RulerPanel.h"
#include "MiniMapPanel.h"
#include "HistoryPanel.h"
#include "SearchPanel.h"
#include "TimelinePanel.h"
#include "DebugOverlayPanel.h"
#include "HelpPanel.h"
#include "PerformancePanel.h"

namespace kimia::ui {

// Aggregate state for the full editor layout.
struct EditorState {
  f32 windowW = 800;
  f32 windowH = 600;

  i32 selectedNode = -1;
  i32 selectedOutliner = -1;
  i32 selectedAsset = -1;
  i32 selectedSearch = -1;

  std::vector<HierarchyNode> hierarchy;
  std::vector<OutlinerEntry> outliner;
  std::vector<ProjectAsset>  assets;
  std::vector<SearchResult>  searchResults;
  std::string projectFolder = "/assets";
  std::string searchQuery;
  i32 hierarchyScroll = 0;
  i32 outlinerScroll = 0;
  i32 projectScroll = 0;
  i32 searchScroll = 0;
  i32 consoleScroll = -1;

  std::vector<LogLine> consoleLines;
  std::string consoleInput;

  std::string inspectedName = "Hero";
  std::string inspectedType = "Character";
  std::vector<InspectorProp> inspectedProps;
  i32 inspectorScroll = 0;

  GizmoMode gizmo = GizmoMode::Move;
  ViewportShading shading = ViewportShading::Lit;
  bool playing = false;
  std::string viewportCamera = "Main";
  f32 viewportZoom = 32.0f;

  std::vector<TimelineBlock> timeline;
  f32 timelineStart = 0;
  f32 timelineEnd = 10;
  f32 playhead = 0;

  std::vector<Menu> menus;

  StatusInfo status;

  DebugReadout debug;

  std::vector<HistoryEntry> history;
};

void drawUnityLayout(const EditorState& state);

}

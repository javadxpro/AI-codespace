#include <kimia/UnityLayout.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
constexpr f32 kMenuH     = 22.0f;
constexpr f32 kStatusH   = 22.0f;
constexpr f32 kTimelineH = 100.0f;
constexpr f32 kConsoleH  = 140.0f;
constexpr f32 kLeftW     = 240.0f;
constexpr f32 kRightW    = 280.0f;
}

void drawUnityLayout(const EditorState& state) {
  using namespace theme;

  const f32 W = state.windowW;
  const f32 H = state.windowH;
  if (W <= 0.0f || H <= 0.0f) return;

  // Top menu.
  drawMainMenuBarPanel({0, 0, W, kMenuH}, state.menus);

  // Bottom status bar.
  drawStatusBarPanel({0, H - kStatusH, W, kStatusH}, state.status);

  // Calculate vertical regions.
  const f32 topY    = kMenuH;
  const f32 bottomY = H - kStatusH - kTimelineH - kConsoleH;
  const f32 midH    = std::max(0.0f, bottomY - topY);

  // Left column: hierarchy (top) + project (middle) + search (bottom).
  const f32 leftH = midH;
  const f32 leftTopH    = leftH * 0.55f;
  const f32 leftMiddleH = leftH * 0.25f;
  const f32 leftBottomH = leftH - leftTopH - leftMiddleH;

  drawHierarchyPanel({0, topY, kLeftW, leftTopH},
                     state.hierarchy,
                     state.selectedNode,
                     state.hierarchyScroll);

  drawProjectBrowserPanel({0, topY + leftTopH,
                           kLeftW, leftMiddleH},
                          state.projectFolder,
                          state.assets,
                          state.selectedAsset,
                          state.projectScroll);

  drawSearchPanel({0, topY + leftTopH + leftMiddleH,
                   kLeftW, leftBottomH},
                  state.searchQuery,
                  state.searchResults,
                  state.selectedSearch,
                  state.searchScroll);

  // Center: viewport + minimap.
  const f32 centerX = kLeftW;
  const f32 centerW = W - kLeftW - kRightW;
  const f32 viewportH = midH - 100.0f; // leave space for history
  drawViewportPanel({centerX, topY, centerW, std::max(50.0f, viewportH)},
                    state.gizmo,
                    state.shading,
                    state.playing,
                    state.viewportCamera,
                    1.0f, true, true,
                    state.viewportZoom);

  // Mini map at top right of center region.
  drawMiniMapPanel({centerX + centerW - 140.0f, topY + 4.0f,
                    130.0f, 90.0f},
                   {}, 0.0f, 0.0f, 10.0f, 10.0f,
                   2.0f, 2.0f, 4.0f, 4.0f);

  // History under viewport.
  drawHistoryPanel({centerX, topY + viewportH + 2.0f,
                    centerW, std::max(20.0f, midH - viewportH - 4.0f)},
                   state.history, state.hierarchyScroll);

  // Right column: inspector + outliner + perf.
  const f32 rightX = W - kRightW;
  const f32 rightH = midH;
  const f32 inspectorH  = rightH * 0.5f;
  const f32 outlinerH   = rightH * 0.3f;
  const f32 perfH       = rightH - inspectorH - outlinerH;

  drawInspectorPanel({rightX, topY, kRightW, inspectorH},
                     state.inspectedName,
                     state.inspectedType,
                     state.inspectedProps,
                     state.inspectorScroll);

  drawSceneOutlinerPanel({rightX, topY + inspectorH,
                          kRightW, outlinerH},
                         state.outliner,
                         std::string(),
                         state.selectedOutliner,
                         state.outlinerScroll);

  drawPerformancePanel({rightX, topY + inspectorH + outlinerH,
                        kRightW, perfH},
                       {60.0f, 16.0f, 4.0f, 8.0f, 256.0f,
                        200, 100000, 50000});

  // Timeline below main area.
  const f32 tlY = H - kStatusH - kConsoleH - kTimelineH;
  drawTimelinePanel({0, tlY, W, kTimelineH},
                    state.timeline,
                    state.timelineStart,
                    state.timelineEnd,
                    state.playhead);

  // Console at bottom.
  drawConsolePanel({0, H - kStatusH - kConsoleH, W, kConsoleH},
                   state.consoleLines,
                   state.consoleInput,
                   state.consoleScroll,
                   true);

  // Debug overlay top-left of center region (small).
  drawDebugOverlayPanel({centerX + 4.0f, topY + 26.0f, 200.0f, 90.0f},
                        state.debug);

  // Help at far top right corner (small).
  drawHelpPanel({W - 200.0f, kMenuH + 4.0f, 180.0f, 80.0f},
                {}, std::string(), 0);
}

}

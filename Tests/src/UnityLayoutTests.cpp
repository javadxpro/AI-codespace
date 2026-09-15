#include <kimia_test.h>
#include <kimia/UnityLayout.h>

KIMIA_TEST(UnityLayout_DrawDefaultDoesNotCrash) {
  kimia::ui::EditorState s;
  s.windowW = 800;
  s.windowH = 600;
  kimia::ui::drawUnityLayout(s);
}

KIMIA_TEST(UnityLayout_DrawWithFullState) {
  kimia::ui::EditorState s;
  s.windowW = 1024;
  s.windowH = 720;

  // Hierarchy.
  s.hierarchy.push_back({"World", 0, true, true, -1});
  s.hierarchy.push_back({"Player", 1, true, true, 0});
  s.hierarchy.push_back({"Mesh", 2, true, true, 1});
  s.hierarchy.push_back({"Camera", 1, true, true, 0});
  s.hierarchy.push_back({"Light", 1, true, false, 0});
  s.selectedNode = 1;

  // Outliner.
  s.outliner.push_back({"Player",  "Character", false, false});
  s.outliner.push_back({"Cube_1",  "Mesh",      false, false});
  s.outliner.push_back({"Cube_2",  "Mesh",      false, true});
  s.outliner.push_back({"Light",   "Light",     true,  false});
  s.selectedOutliner = 0;

  // Assets.
  s.assets.push_back({"Player.glb", "/a/p.glb", "Model", 4ULL * 1024 * 1024});
  s.assets.push_back({"Sky.png",    "/a/s.png", "Image", 8ULL * 1024 * 1024});
  s.assets.push_back({"Main.k",     "/s/m.k",   "Scene", 16ULL * 1024});
  s.selectedAsset = 0;

  // Search.
  s.searchQuery = "cube";
  s.searchResults.push_back({"Cube_1", "/a/cube.glb", "Model", 95});
  s.searchResults.push_back({"Cube_2", "/a/c2.glb",   "Model", 88});

  // Inspector.
  s.inspectedName = "Player";
  s.inspectedType = "Character";
  s.inspectedProps.push_back({});
  s.inspectedProps.back().name = "Health";
  s.inspectedProps.back().kind = kimia::ui::InspectorKind::Int;
  s.inspectedProps.back().v0 = 100;
  s.inspectedProps.push_back({});
  s.inspectedProps.back().name = "Speed";
  s.inspectedProps.back().kind = kimia::ui::InspectorKind::Float;
  s.inspectedProps.back().v0 = 5.0f;

  // Timeline.
  s.timeline.push_back({"Intro", 0, 2, 200, 80, 80, false});
  s.timeline.push_back({"Game",  2, 5, 80, 200, 80, true});
  s.playhead = 1.5f;

  // Console.
  s.consoleLines.push_back({kimia::ui::LogLevel::Info, "ready"});
  s.consoleLines.push_back({kimia::ui::LogLevel::Warn, "slow frame"});
  s.consoleInput = "play";

  // Viewport.
  s.gizmo = kimia::ui::GizmoMode::Move;
  s.shading = kimia::ui::ViewportShading::Lit;
  s.playing = true;

  // Status.
  s.status.sceneName = "Soccer";
  s.status.version = "v0.30.0";
  s.status.fps = 60.0f;
  s.status.triCount = 50000;
  s.status.branch = "main";
  s.status.buildConfig = "Debug";

  // Debug.
  s.debug.fps = 60.0f;
  s.debug.frameMs = 16.0f;
  s.debug.scene = "Soccer";

  // History.
  s.history.push_back({"Move Player", 0, false});
  s.history.push_back({"Set Color", 0, true});

  // Menus.
  s.menus.push_back({"File", {{"New"}, {"Open"}, {"Save"}, {"Quit"}}});
  s.menus.push_back({"Edit", {{"Undo"}, {"Redo"}}});
  s.menus[0].selectedIndex = 0;

  kimia::ui::drawUnityLayout(s);
}

KIMIA_TEST(UnityLayout_DrawAtPhonePortrait) {
  kimia::ui::EditorState s;
  s.windowW = 240;
  s.windowH = 320;
  s.hierarchy.push_back({"Root", 0, true, true, -1});
  s.assets.push_back({"A", "/a", "Model", 1024});
  s.consoleLines.push_back({kimia::ui::LogLevel::Info, "ok"});
  kimia::ui::drawUnityLayout(s);
}

KIMIA_TEST(UnityLayout_DrawAtTabletLandscape) {
  kimia::ui::EditorState s;
  s.windowW = 800;
  s.windowH = 480;
  for (int i = 0; i < 10; ++i) {
    s.hierarchy.push_back({"N" + std::to_string(i),
                           i / 3, true, true, -1});
  }
  for (int i = 0; i < 5; ++i) {
    s.timeline.push_back({"T" + std::to_string(i),
                          static_cast<kimia::f32>(i),
                          1.0f,
                          static_cast<kimia::u8>(i * 50),
                          static_cast<kimia::u8>(i * 30),
                          static_cast<kimia::u8>(i * 70),
                          i == 2});
  }
  s.playhead = 3.0f;
  kimia::ui::drawUnityLayout(s);
}

KIMIA_TEST(UnityLayout_DrawAtTinySize) {
  constexpr kimia::f32 W = 100.0f;
  constexpr kimia::f32 H = 100.0f;
  // Below layout thresholds, should still not crash.
  kimia::ui::EditorState s;
  s.windowW = W;
  s.windowH = H;
  kimia::ui::drawUnityLayout(s);
}

KIMIA_TEST(UnityLayout_DrawAtZeroSize) {
  // Should early-out.
  kimia::ui::EditorState s;
  s.windowW = 0;
  s.windowH = 0;
  kimia::ui::drawUnityLayout(s);

  s.windowW = 100;
  s.windowH = 0;
  kimia::ui::drawUnityLayout(s);

  s.windowW = 0;
  s.windowH = 100;
  kimia::ui::drawUnityLayout(s);
}

KIMIA_TEST(UnityLayout_DrawWithEmptyState) {
  kimia::ui::EditorState s;
  s.windowW = 640;
  s.windowH = 480;
  s.hierarchy.clear();
  s.outliner.clear();
  s.assets.clear();
  s.consoleLines.clear();
  s.timeline.clear();
  s.history.clear();
  s.searchResults.clear();
  s.menus.clear();
  kimia::ui::drawUnityLayout(s);
}

KIMIA_TEST(UnityLayout_DrawWithHighFps) {
  kimia::ui::EditorState s;
  s.windowW = 800;
  s.windowH = 600;
  s.status.fps = 144.0f;
  s.debug.fps = 144.0f;
  kimia::ui::drawUnityLayout(s);
}

KIMIA_TEST(UnityLayout_DrawWithLowFps) {
  // Exercises fps < 30 -> warning, < 15 -> error color branches.
  kimia::ui::EditorState s;
  s.windowW = 800;
  s.windowH = 600;
  s.status.fps = 8.0f;
  s.debug.fps = 8.0f;
  kimia::ui::drawUnityLayout(s);
}

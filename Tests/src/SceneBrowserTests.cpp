// SceneBrowser tests — see Engine/EditorUI/include/kimia/SceneBrowser.h.

#include <kimia_test.h>
#include <kimia/SceneBrowser.h>

KIMIA_TEST(SceneBrowser_DrawEmptyDoesNotCrash) {
  kimia::ui::drawSceneBrowser({0, 0, 200, 300}, {}, 0);
}

KIMIA_TEST(SceneBrowser_DrawOneScene) {
  std::vector<kimia::ui::SceneEntry> v(1);
  v[0].name = "main";
  v[0].fullPath = "scenes/main.kimia";
  v[0].currentScene = true;
  v[0].dirty = false;
  kimia::ui::drawSceneBrowser({0, 0, 200, 300}, v, 0);
}

KIMIA_TEST(SceneBrowser_DrawDirtyScene) {
  std::vector<kimia::ui::SceneEntry> v(1);
  v[0].name = "untitled";
  v[0].dirty = true;
  v[0].currentScene = true;
  kimia::ui::drawSceneBrowser({0, 0, 200, 300}, v, 0);
}

KIMIA_TEST(SceneBrowser_DrawManyScenes) {
  std::vector<kimia::ui::SceneEntry> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::SceneEntry e;
    e.name = "scene_" + std::to_string(i);
    e.dirty = (i % 2 == 0);
    e.currentScene = (i == 5);
    v.push_back(e);
  }
  kimia::ui::drawSceneBrowser({0, 0, 200, 300}, v, 0);
  kimia::ui::drawSceneBrowser({0, 0, 200, 300}, v, -100);
  kimia::ui::drawSceneBrowser({0, 0, 200, 300}, v, 200);
}

KIMIA_TEST(SceneBrowser_DrawWithVeryLongName) {
  std::vector<kimia::ui::SceneEntry> v(1);
  v[0].name = "A_Very_Long_Scene_Name_That_Should_Still_Render_Without_Crashing_Or_Overflowing_The_Panel_1234567890";
  kimia::ui::drawSceneBrowser({0, 0, 200, 300}, v, 0);
}

KIMIA_TEST(SceneBrowser_DrawAtPhonePortrait) {
  std::vector<kimia::ui::SceneEntry> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::SceneEntry e;
    e.name = "s" + std::to_string(i);
    v.push_back(e);
  }
  kimia::ui::drawSceneBrowser({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(SceneBrowser_DrawAtTabletLandscape) {
  std::vector<kimia::ui::SceneEntry> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::SceneEntry e;
    e.name = "s" + std::to_string(i);
    v.push_back(e);
  }
  kimia::ui::drawSceneBrowser({0, 0, 800, 400}, v, 0);
}

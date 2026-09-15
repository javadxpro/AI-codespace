#include <kimia_test.h>
#include <kimia/SceneOutlinerPanel.h>

KIMIA_TEST(SceneOutliner_DrawEmptyDoesNotCrash) {
  kimia::ui::drawSceneOutlinerPanel({0, 0, 280, 240}, {}, "", -1, 0);
}

KIMIA_TEST(SceneOutliner_DrawOneEntry) {
  std::vector<kimia::ui::OutlinerEntry> v(1);
  v[0] = {"Cube_1", "Mesh", false, false};
  kimia::ui::drawSceneOutlinerPanel({0, 0, 280, 240}, v, "", 0, 0);
}

KIMIA_TEST(SceneOutliner_DrawManyEntries) {
  std::vector<kimia::ui::OutlinerEntry> v;
  v.push_back({"Player",   "Character", true,  false});
  v.push_back({"Cube_1",   "Mesh",      false, false});
  v.push_back({"Cube_2",   "Mesh",      false, true});
  v.push_back({"Light_1",  "Light",     true,  false});
  v.push_back({"Camera_1", "Camera",    false, false});
  v.push_back({"Trigger",  "Volume",    false, true});
  v.push_back({"BG_Music", "Audio",     true,  false});
  v.push_back({"Sky",      "Sky",       true,  false});
  kimia::ui::drawSceneOutlinerPanel({0, 0, 320, 280}, v, "", 0, 0);
}

KIMIA_TEST(SceneOutliner_DrawWithFilter) {
  std::vector<kimia::ui::OutlinerEntry> v;
  v.push_back({"Player",  "Character", true,  false});
  v.push_back({"Cube_1",  "Mesh",      false, false});
  v.push_back({"Cube_2",  "Mesh",      false, true});
  v.push_back({"Light_1", "Light",     true,  false});
  kimia::ui::drawSceneOutlinerPanel({0, 0, 320, 280}, v, "cube", 0, 0);
  kimia::ui::drawSceneOutlinerPanel({0, 0, 320, 280}, v, "Player", 0, 0);
  kimia::ui::drawSceneOutlinerPanel({0, 0, 320, 280}, v, "noMatch", 0, 0);
}

KIMIA_TEST(SceneOutliner_DrawAtScroll) {
  std::vector<kimia::ui::OutlinerEntry> v;
  for (int i = 0; i < 30; ++i) {
    v.push_back({"E" + std::to_string(i),
                 "Mesh", false, (i % 2 == 0)});
  }
  kimia::ui::drawSceneOutlinerPanel({0, 0, 280, 200}, v, "", 0, -50);
  kimia::ui::drawSceneOutlinerPanel({0, 0, 280, 200}, v, "", 0, 100);
}

KIMIA_TEST(SceneOutliner_DrawAtPhonePortrait) {
  std::vector<kimia::ui::OutlinerEntry> v;
  v.push_back({"A", "Mesh", false, false});
  v.push_back({"B", "Mesh", false, true});
  kimia::ui::drawSceneOutlinerPanel({0, 0, 240, 320}, v, "", 0, 0);
}

KIMIA_TEST(SceneOutliner_DrawAtTabletLandscape) {
  std::vector<kimia::ui::OutlinerEntry> v;
  for (int i = 0; i < 20; ++i) {
    v.push_back({"Obj" + std::to_string(i),
                 "Type" + std::to_string(i % 3),
                 (i % 2 == 0), (i % 4 == 0)});
  }
  kimia::ui::drawSceneOutlinerPanel({0, 0, 480, 320}, v, "Obj", 0, 0);
}

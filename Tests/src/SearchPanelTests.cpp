#include <kimia_test.h>
#include <kimia/SearchPanel.h>

KIMIA_TEST(Search_DrawEmptyDoesNotCrash) {
  kimia::ui::drawSearchPanel({0, 0, 280, 200}, "", {}, -1, 0);
}

KIMIA_TEST(Search_DrawQueryWithoutResults) {
  kimia::ui::drawSearchPanel({0, 0, 280, 200}, "noMatch", {}, -1, 0);
}

KIMIA_TEST(Search_DrawOneResult) {
  std::vector<kimia::ui::SearchResult> v(1);
  v[0].name = "Cube";
  v[0].path = "/assets/models/cube.obj";
  v[0].kind = "Model";
  v[0].score = 100;
  kimia::ui::drawSearchPanel({0, 0, 280, 200}, "cub", v, 0, 0);
}

KIMIA_TEST(Search_DrawManyResults) {
  std::vector<kimia::ui::SearchResult> v;
  v.push_back({"Cube",       "/a/cube.obj",   "Model",    100});
  v.push_back({"CubeRed",    "/a/cube_red",   "Material", 95});
  v.push_back({"SkyTex",     "/a/sky.png",    "Image",    88});
  v.push_back({"Scene1",     "/s/scene1.k",   "Scene",    76});
  v.push_back({"CubeAudio", "/a/cube.ogg",   "Audio",    70});
  v.push_back({"MusicTrack", "/a/mus.ogg",    "Audio",    64});
  v.push_back({"CubeMat",    "/a/cmat.mat",   "Material", 60});
  v.push_back({"Unknown",    "/a/x.bin",      "",         50});
  kimia::ui::drawSearchPanel({0, 0, 320, 280}, "cube", v, 1, 0);
}

KIMIA_TEST(Search_DrawAtScroll) {
  std::vector<kimia::ui::SearchResult> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::SearchResult r;
    r.name = "Result_" + std::to_string(i);
    r.kind = "Model";
    r.score = 100 - i;
    v.push_back(r);
  }
  kimia::ui::drawSearchPanel({0, 0, 280, 200}, "r", v, 0, -50);
  kimia::ui::drawSearchPanel({0, 0, 280, 200}, "r", v, 5, 100);
}

KIMIA_TEST(Search_DrawAtPhonePortrait) {
  std::vector<kimia::ui::SearchResult> v;
  v.push_back({"A", "/a", "Model", 1});
  v.push_back({"B", "/b", "Image", 2});
  kimia::ui::drawSearchPanel({0, 0, 240, 320}, "a", v, 0, 0);
}

KIMIA_TEST(Search_DrawAtTabletLandscape) {
  std::vector<kimia::ui::SearchResult> v;
  for (int i = 0; i < 8; ++i) {
    kimia::ui::SearchResult r;
    r.name = "T_" + std::to_string(i);
    r.kind = (i % 2 == 0) ? "Model" : "Image";
    r.score = i;
    v.push_back(r);
  }
  kimia::ui::drawSearchPanel({0, 0, 480, 320}, "T", v, 3, 0);
}

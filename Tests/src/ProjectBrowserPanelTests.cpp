#include <kimia_test.h>
#include <kimia/ProjectBrowserPanel.h>

KIMIA_TEST(Project_DrawEmptyDoesNotCrash) {
  kimia::ui::drawProjectBrowserPanel({0, 0, 280, 240},
                                     "/", {}, -1, 0);
}

KIMIA_TEST(Project_DrawOneAsset) {
  std::vector<kimia::ui::ProjectAsset> v(1);
  v[0] = {"cube.obj", "/assets/cube.obj", "Model",
          1024ULL * 1024};
  kimia::ui::drawProjectBrowserPanel({0, 0, 280, 240},
                                     "/assets", v, 0, 0);
}

KIMIA_TEST(Project_DrawManyAssets) {
  std::vector<kimia::ui::ProjectAsset> v;
  v.push_back({"Player",     "/a/player.glb",  "Model",    4ULL * 1024 * 1024});
  v.push_back({"SkyTex",     "/a/sky.png",     "Image",    8ULL * 1024 * 1024});
  v.push_back({"MainScene",  "/s/main.k",      "Scene",    16ULL * 1024});
  v.push_back({"RedMat",     "/m/red.mat",     "Material", 4ULL * 1024});
  v.push_back({"Track",      "/a/mus.ogg",    "Audio",    2ULL * 1024 * 1024});
  v.push_back({"Header",     "/f/roboto.ttf",  "Font",     64ULL * 1024});
  v.push_back({"Unknown",    "/x/x.bin",      "",         128ULL});
  v.push_back({"BigAsset",   "/big.b",         "Model",    512ULL * 1024 * 1024});
  v.push_back({"Tiny",       "/t",             "Image",    32ULL});
  kimia::ui::drawProjectBrowserPanel({0, 0, 320, 320},
                                     "/assets", v, 0, 0);
}

KIMIA_TEST(Project_DrawAtScroll) {
  std::vector<kimia::ui::ProjectAsset> v;
  for (int i = 0; i < 60; ++i) {
    v.push_back({"A" + std::to_string(i),
                 "/p/" + std::to_string(i),
                 "Model", static_cast<kimia::u64>(i * 1024)});
  }
  kimia::ui::drawProjectBrowserPanel({0, 0, 280, 200}, "/p",
                                     v, 0, -100);
  kimia::ui::drawProjectBrowserPanel({0, 0, 280, 200}, "/p",
                                     v, 0, 100);
}

KIMIA_TEST(Project_DrawWithZeroSize) {
  std::vector<kimia::ui::ProjectAsset> v;
  v.push_back({"A", "/a", "Model", 0});
  v.push_back({"B", "/b", "Image", 1024});
  v.push_back({"C", "/c", "Model", 1024ULL * 1024});
  v.push_back({"D", "/d", "Image", 1024ULL * 1024 * 1024});
  v.push_back({"E", "/e", "Model",
               1024ULL * 1024 * 1024 * 1024});
  kimia::ui::drawProjectBrowserPanel({0, 0, 280, 200}, "/", v, 1, 0);
}

KIMIA_TEST(Project_DrawAtPhonePortrait) {
  std::vector<kimia::ui::ProjectAsset> v;
  v.push_back({"A", "/a", "Model", 1000});
  v.push_back({"B", "/b", "Image", 2000});
  v.push_back({"C", "/c", "Scene", 3000});
  kimia::ui::drawProjectBrowserPanel({0, 0, 240, 320}, "/", v, 0, 0);
}

KIMIA_TEST(Project_DrawAtTabletLandscape) {
  std::vector<kimia::ui::ProjectAsset> v;
  for (int i = 0; i < 30; ++i) {
    v.push_back({"X" + std::to_string(i),
                 "/x/" + std::to_string(i),
                 (i % 3 == 0) ? "Model" :
                 (i % 3 == 1) ? "Image" : "Audio",
                 static_cast<kimia::u64>(i * 10000)});
  }
  kimia::ui::drawProjectBrowserPanel({0, 0, 480, 320}, "/", v, 5, 0);
}

// AssetBrowser tests — see Engine/EditorUI/include/kimia/AssetBrowser.h.

#include <kimia_test.h>
#include <kimia/AssetBrowser.h>

KIMIA_TEST(AssetBrowser_ClassifyObjModel) {
  KIMIA_REQUIRE(kimia::ui::classifyFile("foo.obj") == kimia::ui::AssetKind::Model);
  KIMIA_REQUIRE(kimia::ui::classifyFile("foo.fbx") == kimia::ui::AssetKind::Model);
  KIMIA_REQUIRE(kimia::ui::classifyFile("FOO.OBJ") == kimia::ui::AssetKind::Model);
}

KIMIA_TEST(AssetBrowser_ClassifyImage) {
  KIMIA_REQUIRE(kimia::ui::classifyFile("a.png") == kimia::ui::AssetKind::Image);
  KIMIA_REQUIRE(kimia::ui::classifyFile("b.jpg") == kimia::ui::AssetKind::Image);
  KIMIA_REQUIRE(kimia::ui::classifyFile("c.jpeg") == kimia::ui::AssetKind::Image);
}

KIMIA_TEST(AssetBrowser_ClassifyScene) {
  KIMIA_REQUIRE(kimia::ui::classifyFile("main.kimia") == kimia::ui::AssetKind::Scene);
}

KIMIA_TEST(AssetBrowser_ClassifyOther) {
  KIMIA_REQUIRE(kimia::ui::classifyFile("readme.txt") == kimia::ui::AssetKind::Other);
  KIMIA_REQUIRE(kimia::ui::classifyFile("noext") == kimia::ui::AssetKind::Other);
  KIMIA_REQUIRE(kimia::ui::classifyFile("") == kimia::ui::AssetKind::Other);
}

KIMIA_TEST(AssetBrowser_ScanEmptyDirectory) {
  const auto out = kimia::ui::scanAssets("");
  KIMIA_REQUIRE(out.empty());
}

KIMIA_TEST(AssetBrowser_DrawEmptyPanelDoesNotCrash) {
  // Just exercise the entry point: an empty asset list + a tiny rect.
  kimia::ui::drawAssetBrowser({0, 0, 100, 100}, {}, 0);
}

KIMIA_TEST(AssetBrowser_DrawWithAssetsDoesNotCrash) {
  std::vector<kimia::ui::AssetEntry> v;
  kimia::ui::AssetEntry a;
  a.name = "main.kimia";
  a.kind = kimia::ui::AssetKind::Scene;
  v.push_back(std::move(a));
  a.name = "tree.obj";
  a.kind = kimia::ui::AssetKind::Model;
  v.push_back(std::move(a));
  a.name = "ground.png";
  a.kind = kimia::ui::AssetKind::Image;
  v.push_back(std::move(a));
  kimia::ui::drawAssetBrowser({0, 0, 200, 200}, v, 0);
  kimia::ui::drawAssetBrowser({0, 0, 200, 200}, v, -20);  // negative scroll
  kimia::ui::drawAssetBrowser({0, 0, 200, 200}, v, 100);  // positive scroll
}

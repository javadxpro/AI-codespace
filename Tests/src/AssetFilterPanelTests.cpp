#include <kimia_test.h>
#include <kimia/AssetFilterPanel.h>

KIMIA_TEST(AssetFilter_DrawEmptyDoesNotCrash) {
  kimia::ui::drawAssetFilterPanel({0, 0, 200, 200}, {});
}

KIMIA_TEST(AssetFilter_DrawOneFilter) {
  std::vector<kimia::ui::AssetFilterEntry> v(1);
  v[0].label = "Models";
  v[0].kind = "Model";
  v[0].count = 12;
  v[0].active = true;
  kimia::ui::drawAssetFilterPanel({0, 0, 200, 200}, v);
}

KIMIA_TEST(AssetFilter_DrawManyFilters) {
  std::vector<kimia::ui::AssetFilterEntry> v;
  v.push_back({"All",       "",       100, true});
  v.push_back({"Models",    "Model",  20,  true});
  v.push_back({"Images",    "Image",  35,  true});
  v.push_back({"Scenes",    "Scene",  5,   true});
  v.push_back({"Materials", "",       40,  false});
  v.push_back({"Fonts",     "",       8,   true});
  v.push_back({"Audio",     "",       12,  false});
  kimia::ui::drawAssetFilterPanel({0, 0, 200, 240}, v);
}

KIMIA_TEST(AssetFilter_DrawWithAllDisabled) {
  std::vector<kimia::ui::AssetFilterEntry> v;
  v.push_back({"A", "",  5, false});
  v.push_back({"B", "X", 10, false});
  kimia::ui::drawAssetFilterPanel({0, 0, 200, 200}, v);
}

KIMIA_TEST(AssetFilter_DrawAtPhonePortrait) {
  std::vector<kimia::ui::AssetFilterEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::AssetFilterEntry f;
    f.label = "F" + std::to_string(i);
    f.kind = "Model";
    f.count = i;
    f.active = (i % 2 == 0);
    v.push_back(f);
  }
  kimia::ui::drawAssetFilterPanel({0, 0, 200, 320}, v);
}

KIMIA_TEST(AssetFilter_DrawAtTabletLandscape) {
  std::vector<kimia::ui::AssetFilterEntry> v;
  v.push_back({"All",      "",       100, true});
  v.push_back({"Models",   "Model",  20,  true});
  v.push_back({"Images",   "Image",  35,  true});
  v.push_back({"Scenes",   "Scene",  5,   true});
  v.push_back({"Audio",    "",       12,  false});
  kimia::ui::drawAssetFilterPanel({0, 0, 240, 280}, v);
}

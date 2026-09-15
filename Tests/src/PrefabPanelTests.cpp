#include <kimia_test.h>
#include <kimia/PrefabPanel.h>

KIMIA_TEST(Prefab_DrawEmptyDoesNotCrash) {
  kimia::ui::drawPrefabPanel({0, 0, 240, 200}, {}, 0);
}

KIMIA_TEST(Prefab_DrawOnePrefab) {
  std::vector<kimia::ui::PrefabEntry> v(1);
  v[0].name = "tree";
  v[0].instanceCount = 12;
  v[0].lastModifiedSec = 30.0f;
  kimia::ui::drawPrefabPanel({0, 0, 240, 200}, v, 0);
}

KIMIA_TEST(Prefab_DrawManyPrefabs) {
  std::vector<kimia::ui::PrefabEntry> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::PrefabEntry p;
    p.name = "p_" + std::to_string(i);
    p.instanceCount = i;
    p.lastModifiedSec = static_cast<float>(i) * 10.0f;
    v.push_back(p);
  }
  kimia::ui::drawPrefabPanel({0, 0, 240, 240}, v, 0);
  kimia::ui::drawPrefabPanel({0, 0, 240, 240}, v, -50);
  kimia::ui::drawPrefabPanel({0, 0, 240, 240}, v, 100);
}

KIMIA_TEST(Prefab_DrawAtPhonePortrait) {
  std::vector<kimia::ui::PrefabEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::PrefabEntry p;
    p.name = "p" + std::to_string(i);
    p.instanceCount = i * 2;
    v.push_back(p);
  }
  kimia::ui::drawPrefabPanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(Prefab_DrawAtTabletLandscape) {
  std::vector<kimia::ui::PrefabEntry> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::PrefabEntry p;
    p.name = "prefab_" + std::to_string(i);
    p.instanceCount = i;
    v.push_back(p);
  }
  kimia::ui::drawPrefabPanel({0, 0, 480, 240}, v, 0);
}

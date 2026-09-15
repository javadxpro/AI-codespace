#include <kimia_test.h>
#include <kimia/TabBarPanel.h>

KIMIA_TEST(TabBar_DrawEmptyDoesNotCrash) {
  kimia::ui::drawTabBarPanel({0, 0, 320, 24}, {}, -1);
}

KIMIA_TEST(TabBar_DrawOneTab) {
  std::vector<kimia::ui::Tab> v(1);
  v[0].title = "Scene";
  v[0].glyph = "S";
  v[0].closable = true;
  v[0].dirty = true;
  kimia::ui::drawTabBarPanel({0, 0, 320, 24}, v, 0);
}

KIMIA_TEST(TabBar_DrawManyTabs) {
  std::vector<kimia::ui::Tab> v;
  const char* titles[] = {"Scene", "Materials", "Animations", "Audio", "Settings"};
  for (int i = 0; i < 5; ++i) {
    kimia::ui::Tab t;
    t.title = titles[i];
    t.glyph = titles[i][0];
    t.closable = true;
    t.dirty = (i % 2 == 0);
    v.push_back(t);
  }
  kimia::ui::drawTabBarPanel({0, 0, 600, 24}, v, 2);
}

KIMIA_TEST(TabBar_DrawOverflow) {
  std::vector<kimia::ui::Tab> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::Tab t;
    t.title = "T" + std::to_string(i);
    v.push_back(t);
  }
  kimia::ui::drawTabBarPanel({0, 0, 400, 24}, v, 0);
}

KIMIA_TEST(TabBar_DrawAtPhonePortrait) {
  std::vector<kimia::ui::Tab> v;
  for (int i = 0; i < 3; ++i) {
    kimia::ui::Tab t;
    t.title = "T" + std::to_string(i);
    v.push_back(t);
  }
  kimia::ui::drawTabBarPanel({0, 0, 240, 24}, v, 0);
}

KIMIA_TEST(TabBar_DrawAtTabletLandscape) {
  std::vector<kimia::ui::Tab> v;
  for (int i = 0; i < 8; ++i) {
    kimia::ui::Tab t;
    t.title = "tab_" + std::to_string(i);
    t.closable = true;
    v.push_back(t);
  }
  kimia::ui::drawTabBarPanel({0, 0, 800, 24}, v, 3);
}

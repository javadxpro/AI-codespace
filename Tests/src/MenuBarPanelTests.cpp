#include <kimia_test.h>
#include <kimia/MenuBarPanel.h>

KIMIA_TEST(MenuBar_DrawNoMenusDoesNotCrash) {
  kimia::ui::drawMenuBarPanel({0, 0, 320, 22}, {}, -1);
}

KIMIA_TEST(MenuBar_DrawClosedMenu) {
  std::vector<kimia::ui::Menu> v;
  kimia::ui::Menu m;
  m.title = "File";
  m.items.push_back({"New", "Ctrl+N", 1});
  m.items.push_back({"Open", "Ctrl+O", 2});
  m.items.push_back({"Save", "Ctrl+S", 3});
  m.items.push_back({"", "", 0, true});
  m.items.push_back({"Quit", "Ctrl+Q", 4});
  v.push_back(m);
  kimia::ui::drawMenuBarPanel({0, 0, 320, 22}, v, -1);
}

KIMIA_TEST(MenuBar_DrawOpenMenu) {
  std::vector<kimia::ui::Menu> v;
  kimia::ui::Menu m;
  m.title = "Edit";
  m.items.push_back({"Undo", "Ctrl+Z", 1});
  m.items.push_back({"Redo", "Ctrl+Y", 2});
  m.items.push_back({"", "", 0, true});
  m.items.push_back({"Cut", "Ctrl+X", 3});
  m.items.push_back({"Paste", "Ctrl+V", 4, false, /*disabled=*/true});
  v.push_back(m);
  kimia::ui::drawMenuBarPanel({0, 0, 320, 22}, v, 0);
}

KIMIA_TEST(MenuBar_DrawMultipleMenus) {
  std::vector<kimia::ui::Menu> v;
  const char* titles[] = {"File", "Edit", "View", "Help"};
  for (int i = 0; i < 4; ++i) {
    kimia::ui::Menu m;
    m.title = titles[i];
    m.items.push_back({"Item 1", "Ctrl+1", i * 10 + 1});
    m.items.push_back({"Item 2", "Ctrl+2", i * 10 + 2});
    v.push_back(m);
  }
  kimia::ui::drawMenuBarPanel({0, 0, 400, 22}, v, 1);
}

KIMIA_TEST(MenuBar_DrawAtPhonePortrait) {
  std::vector<kimia::ui::Menu> v;
  kimia::ui::Menu m;
  m.title = "File";
  m.items.push_back({"New", "N", 1});
  v.push_back(m);
  kimia::ui::drawMenuBarPanel({0, 0, 240, 22}, v, 0);
}

KIMIA_TEST(MenuBar_DrawAtTabletLandscape) {
  std::vector<kimia::ui::Menu> v;
  const char* titles[] = {"File", "Edit", "View", "Tools", "Help"};
  for (size_t i = 0; i < 5; ++i) {
    kimia::ui::Menu m;
    m.title = titles[i];
    m.items.push_back({"Item", "", 1});
    v.push_back(m);
  }
  kimia::ui::drawMenuBarPanel({0, 0, 800, 22}, v, 2);
}

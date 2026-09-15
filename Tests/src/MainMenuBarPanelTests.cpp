#include <kimia_test.h>
#include <kimia/MainMenuBarPanel.h>

KIMIA_TEST(MenuBar_DrawEmptyDoesNotCrash) {
  kimia::ui::drawMainMenuBarPanel({0, 0, 800, 24}, {});
}

KIMIA_TEST(MenuBar_DrawStandardMenus) {
  std::vector<kimia::ui::Menu> menus;
  {
    kimia::ui::Menu m; m.name = "File";
    m.items.push_back({"New",   "Ctrl+N"});
    m.items.push_back({"Open",  "Ctrl+O"});
    m.items.push_back({"Save",  "Ctrl+S"});
    m.items.push_back({"Save As", "Ctrl+Shift+S"});
    m.items.push_back({""});
    m.items.push_back({"Quit", "Ctrl+Q"});
    menus.push_back(m);
  }
  {
    kimia::ui::Menu m; m.name = "Edit";
    m.items.push_back({"Undo",   "Ctrl+Z"});
    m.items.push_back({"Redo",   "Ctrl+Y"});
    m.items.push_back({""});
    m.items.push_back({"Cut",    "Ctrl+X"});
    m.items.push_back({"Copy",   "Ctrl+C"});
    m.items.push_back({"Paste",  "Ctrl+V"});
    menus.push_back(m);
  }
  {
    kimia::ui::Menu m; m.name = "GameObject";
    m.items.push_back({"Create Empty"});
    m.items.push_back({"Create Cube"});
    m.items.push_back({"Create Sphere"});
    menus.push_back(m);
  }
  {
    kimia::ui::Menu m; m.name = "Window";
    m.items.push_back({"Hierarchy",   "Ctrl+1"});
    m.items.push_back({"Inspector",   "Ctrl+2"});
    m.items.push_back({"Project",     "Ctrl+3"});
    m.items.push_back({"Console",     "Ctrl+`"});
    menus.push_back(m);
  }
  {
    kimia::ui::Menu m; m.name = "Help";
    m.items.push_back({"Docs"});
    m.items.push_back({"About"});
    menus.push_back(m);
  }
  // All closed.
  kimia::ui::drawMainMenuBarPanel({0, 0, 800, 24}, menus);
  // File open.
  menus[0].selectedIndex = 1;
  kimia::ui::drawMainMenuBarPanel({0, 0, 800, 24}, menus);
  // Edit open.
  menus[0].selectedIndex = -1;
  menus[1].selectedIndex = 3;
  kimia::ui::drawMainMenuBarPanel({0, 0, 800, 24}, menus);
}

KIMIA_TEST(MenuBar_DrawWithDisabledAndSeparator) {
  std::vector<kimia::ui::Menu> menus(1);
  menus[0].name = "Edit";
  menus[0].items.push_back({"Undo", "Ctrl+Z"});
  menus[0].items.push_back({"Redo", "Ctrl+Y", false, true}); // disabled
  menus[0].items.push_back({""});
  menus[0].items.push_back({"Cut",  "Ctrl+X"});
  menus[0].selectedIndex = 2;
  kimia::ui::drawMainMenuBarPanel({0, 0, 600, 24}, menus);
}

KIMIA_TEST(MenuBar_DrawWithMultipleOpen) {
  // Implementation should only show the first one.
  std::vector<kimia::ui::Menu> menus(2);
  menus[0].name = "File";
  menus[0].items.push_back({"New"});
  menus[0].selectedIndex = 0;
  menus[1].name = "Edit";
  menus[1].items.push_back({"Undo"});
  menus[1].selectedIndex = 0;
  kimia::ui::drawMainMenuBarPanel({0, 0, 600, 24}, menus);
}

KIMIA_TEST(MenuBar_DrawWithEmptyMenu) {
  std::vector<kimia::ui::Menu> menus(1);
  menus[0].name = "EmptyMenu";
  menus[0].selectedIndex = 0;
  kimia::ui::drawMainMenuBarPanel({0, 0, 600, 24}, menus);
}

KIMIA_TEST(MenuBar_DrawAtPhonePortrait) {
  std::vector<kimia::ui::Menu> menus;
  menus.push_back({"File", {{"New"}, {"Open"}, {"Save"}}});
  menus.push_back({"Edit", {{"Undo"}}});
  menus.push_back({"View", {{"Reset"}}});
  kimia::ui::drawMainMenuBarPanel({0, 0, 240, 24}, menus);
}

KIMIA_TEST(MenuBar_DrawAtTabletLandscape) {
  std::vector<kimia::ui::Menu> menus;
  menus.push_back({"File", {{"New"}, {"Open"}, {"Save"}, {"Quit"}}});
  menus.push_back({"Edit", {{"Undo"}, {"Redo"}}});
  menus.push_back({"GameObject", {{"Empty"}, {"Cube"}, {"Sphere"}}});
  menus.push_back({"Window", {{"Hierarchy"}, {"Inspector"}}});
  menus.push_back({"Help", {{"About"}}});
  menus[2].selectedIndex = 1;
  kimia::ui::drawMainMenuBarPanel({0, 0, 1024, 24}, menus);
}

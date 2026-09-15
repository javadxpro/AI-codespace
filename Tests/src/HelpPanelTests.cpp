#include <kimia_test.h>
#include <kimia/HelpPanel.h>

KIMIA_TEST(Help_DrawEmptyDoesNotCrash) {
  kimia::ui::drawHelpPanel({0, 0, 320, 240}, {}, "", 0);
}

KIMIA_TEST(Help_DrawOneEntry) {
  std::vector<kimia::ui::HelpEntry> v(1);
  v[0] = {"Ctrl+S", "Save scene", "File"};
  kimia::ui::drawHelpPanel({0, 0, 320, 240}, v, "", 0);
}

KIMIA_TEST(Help_DrawManyEntries) {
  std::vector<kimia::ui::HelpEntry> v;
  v.push_back({"Ctrl+S",       "Save scene",       "File"});
  v.push_back({"Ctrl+O",       "Open scene",       "File"});
  v.push_back({"Ctrl+N",       "New scene",        "File"});
  v.push_back({"Ctrl+Z",       "Undo",             "Edit"});
  v.push_back({"Ctrl+Y",       "Redo",             "Edit"});
  v.push_back({"Ctrl+X",       "Cut",              "Edit"});
  v.push_back({"Ctrl+C",       "Copy",             "Edit"});
  v.push_back({"Ctrl+V",       "Paste",            "Edit"});
  v.push_back({"F",            "Focus selected",   "View"});
  v.push_back({"Ctrl+1",       "Switch to scene",  "View"});
  v.push_back({"Ctrl+2",       "Switch to game",   "View"});
  v.push_back({"W",            "Move tool",        "Tools"});
  v.push_back({"E",            "Rotate tool",      "Tools"});
  v.push_back({"R",            "Scale tool",       "Tools"});
  kimia::ui::drawHelpPanel({0, 0, 360, 320}, v, "", 0);
}

KIMIA_TEST(Help_DrawWithFilter) {
  std::vector<kimia::ui::HelpEntry> v;
  v.push_back({"Ctrl+S", "Save",   "File"});
  v.push_back({"Ctrl+Z", "Undo",   "Edit"});
  v.push_back({"Ctrl+C", "Copy",   "Edit"});
  v.push_back({"S",      "Scale",  "Tools"});
  kimia::ui::drawHelpPanel({0, 0, 360, 240}, v, "ctrl", 0);
  kimia::ui::drawHelpPanel({0, 0, 360, 240}, v, "sca", 0);
  kimia::ui::drawHelpPanel({0, 0, 360, 240}, v, "noMatch", 0);
}

KIMIA_TEST(Help_DrawAtScroll) {
  std::vector<kimia::ui::HelpEntry> v;
  for (int i = 0; i < 30; ++i) {
    v.push_back({"Key" + std::to_string(i),
                 "Desc " + std::to_string(i),
                 "Cat"});
  }
  kimia::ui::drawHelpPanel({0, 0, 320, 240}, v, "", -100);
  kimia::ui::drawHelpPanel({0, 0, 320, 240}, v, "", 200);
}

KIMIA_TEST(Help_DrawAtPhonePortrait) {
  std::vector<kimia::ui::HelpEntry> v;
  v.push_back({"A", "Do thing", "Tools"});
  v.push_back({"B", "Do other", "Tools"});
  kimia::ui::drawHelpPanel({0, 0, 240, 320}, v, "", 0);
}

KIMIA_TEST(Help_DrawAtTabletLandscape) {
  std::vector<kimia::ui::HelpEntry> v;
  for (int i = 0; i < 20; ++i) {
    v.push_back({"K" + std::to_string(i),
                 "Desc" + std::to_string(i),
                 "Cat" + std::to_string(i / 5)});
  }
  kimia::ui::drawHelpPanel({0, 0, 480, 320}, v, "", 0);
}

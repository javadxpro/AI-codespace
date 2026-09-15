#include <kimia_test.h>
#include <kimia/CheatSheetPanel.h>

KIMIA_TEST(CheatSheet_DrawEmptyDoesNotCrash) {
  kimia::ui::drawCheatSheetPanel({0, 0, 240, 200}, {}, 0, "");
}

KIMIA_TEST(CheatSheet_DrawOneEntry) {
  std::vector<kimia::ui::CheatEntry> v(1);
  v[0].trigger = "select";
  v[0].action = "Q";
  v[0].description = "Select tool";
  kimia::ui::drawCheatSheetPanel({0, 0, 240, 200}, v, 0, "");
}

KIMIA_TEST(CheatSheet_DrawManyEntries) {
  std::vector<kimia::ui::CheatEntry> v;
  const char* actions[] = {"Q", "W", "E", "R", "T", "Y"};
  const char* descs[] = {"Select", "Move", "Rotate", "Scale", "Test", "Yield"};
  for (int i = 0; i < 6; ++i) {
    kimia::ui::CheatEntry e;
    e.trigger = actions[i];
    e.action = actions[i];
    e.description = descs[i];
    v.push_back(e);
  }
  kimia::ui::drawCheatSheetPanel({0, 0, 280, 280}, v, 0, "");
}

KIMIA_TEST(CheatSheet_DrawWithSearch) {
  std::vector<kimia::ui::CheatEntry> v;
  v.push_back({"select", "Q", "Select tool"});
  v.push_back({"move", "W", "Move tool"});
  v.push_back({"rotate", "E", "Rotate tool"});
  v.push_back({"scale", "R", "Scale tool"});
  v.push_back({"undo", "Ctrl+Z", "Undo last action"});
  v.push_back({"save", "Ctrl+S", "Save scene"});
  // Filter for "tool" — only 4 should pass.
  kimia::ui::drawCheatSheetPanel({0, 0, 240, 200}, v, 0, "tool");
  // Filter for "select" — 1.
  kimia::ui::drawCheatSheetPanel({0, 0, 240, 200}, v, 0, "SELECT");
  // Filter that doesn't match.
  kimia::ui::drawCheatSheetPanel({0, 0, 240, 200}, v, 0, "xyz");
}

KIMIA_TEST(CheatSheet_DrawAtPhonePortrait) {
  std::vector<kimia::ui::CheatEntry> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::CheatEntry e;
    e.trigger = "t" + std::to_string(i);
    e.action = "A";
    e.description = "d";
    v.push_back(e);
  }
  kimia::ui::drawCheatSheetPanel({0, 0, 240, 320}, v, 0, "");
}

KIMIA_TEST(CheatSheet_DrawAtTabletLandscape) {
  std::vector<kimia::ui::CheatEntry> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::CheatEntry e;
    e.trigger = "t" + std::to_string(i);
    e.action = "A" + std::to_string(i);
    e.description = "desc " + std::to_string(i);
    v.push_back(e);
  }
  kimia::ui::drawCheatSheetPanel({0, 0, 480, 320}, v, 0, "");
}

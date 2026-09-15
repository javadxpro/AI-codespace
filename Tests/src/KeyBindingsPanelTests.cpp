#include <kimia_test.h>
#include <kimia/KeyBindingsPanel.h>

KIMIA_TEST(KeyBindings_DrawEmptyDoesNotCrash) {
  kimia::ui::drawKeyBindingsPanel({0, 0, 280, 200}, {}, 0, "");
}

KIMIA_TEST(KeyBindings_DrawOneBinding) {
  std::vector<kimia::ui::KeyBinding> v(1);
  v[0].category = "File";
  v[0].action = "New";
  v[0].keys = "Ctrl+N";
  v[0].conflict = false;
  kimia::ui::drawKeyBindingsPanel({0, 0, 280, 200}, v, 0, "");
}

KIMIA_TEST(KeyBindings_DrawManyBindings) {
  std::vector<kimia::ui::KeyBinding> v;
  v.push_back({"File", "New",       "Ctrl+N",   false});
  v.push_back({"File", "Open",      "Ctrl+O",   false});
  v.push_back({"File", "Save",      "Ctrl+S",   false});
  v.push_back({"Edit", "Undo",      "Ctrl+Z",   false});
  v.push_back({"Edit", "Redo",      "Ctrl+Y",   true});  // conflict!
  v.push_back({"View", "Toggle UI", "F1",       false});
  v.push_back({"Tools","Reset",     "Ctrl+R",   true});
  kimia::ui::drawKeyBindingsPanel({0, 0, 320, 240}, v, 0, "");
}

KIMIA_TEST(KeyBindings_DrawWithCategoryFilter) {
  std::vector<kimia::ui::KeyBinding> v;
  v.push_back({"File", "New",  "Ctrl+N", false});
  v.push_back({"Edit", "Undo", "Ctrl+Z", false});
  v.push_back({"View", "UI",   "F1",     false});
  // Only File rows.
  kimia::ui::drawKeyBindingsPanel({0, 0, 280, 200}, v, 0, "File");
  // No filter.
  kimia::ui::drawKeyBindingsPanel({0, 0, 280, 200}, v, 0, "");
}

KIMIA_TEST(KeyBindings_DrawAtScroll) {
  std::vector<kimia::ui::KeyBinding> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::KeyBinding b;
    b.category = (i % 3 == 0) ? "File" : (i % 3 == 1) ? "Edit" : "View";
    b.action = "Action_" + std::to_string(i);
    b.keys = "Ctrl+" + std::to_string(i);
    b.conflict = (i % 7 == 0);
    v.push_back(b);
  }
  kimia::ui::drawKeyBindingsPanel({0, 0, 280, 200}, v, -50, "");
  kimia::ui::drawKeyBindingsPanel({0, 0, 280, 200}, v, 100, "");
}

KIMIA_TEST(KeyBindings_DrawAtPhonePortrait) {
  std::vector<kimia::ui::KeyBinding> v;
  for (int i = 0; i < 6; ++i) {
    kimia::ui::KeyBinding b;
    b.category = "X";
    b.action = "a" + std::to_string(i);
    b.keys = "K";
    v.push_back(b);
  }
  kimia::ui::drawKeyBindingsPanel({0, 0, 240, 320}, v, 0, "");
}

KIMIA_TEST(KeyBindings_DrawAtTabletLandscape) {
  std::vector<kimia::ui::KeyBinding> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::KeyBinding b;
    b.category = (i % 2 == 0) ? "File" : "Edit";
    b.action = "action_" + std::to_string(i);
    b.keys = "Ctrl+" + std::to_string(i);
    v.push_back(b);
  }
  kimia::ui::drawKeyBindingsPanel({0, 0, 480, 320}, v, 0, "");
}

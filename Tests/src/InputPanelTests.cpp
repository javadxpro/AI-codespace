#include <kimia_test.h>
#include <kimia/InputPanel.h>

KIMIA_TEST(Input_DrawEmptyDoesNotCrash) {
  kimia::ui::drawInputPanel({0, 0, 240, 200}, {}, 0);
}

KIMIA_TEST(Input_DrawOneBinding) {
  std::vector<kimia::ui::InputBinding> v(1);
  v[0].action = "Move forward";
  v[0].primaryKey = "W";
  v[0].secondaryKey = "Up";
  kimia::ui::drawInputPanel({0, 0, 240, 200}, v, 0);
}

KIMIA_TEST(Input_DrawManyBindings) {
  std::vector<kimia::ui::InputBinding> v;
  const char* actions[] = {
    "Move forward", "Move back", "Strafe left", "Strafe right",
    "Jump", "Crouch", "Sprint", "Reload", "Fire", "Aim",
    "Pause", "Inventory"
  };
  const char* primary[] = {
    "W", "S", "A", "D", "Space", "Ctrl", "Shift", "R", "LMB", "RMB",
    "Esc", "I"
  };
  const char* secondary[] = {
    "Up", "Down", "Left", "Right",
    "",  "",  "",  "",  "",  "",
    "P",  "Tab"
  };
  for (int i = 0; i < 12; ++i) {
    kimia::ui::InputBinding b;
    b.action = actions[i];
    b.primaryKey = primary[i];
    b.secondaryKey = secondary[i];
    v.push_back(b);
  }
  kimia::ui::drawInputPanel({0, 0, 280, 300}, v, 0);
}

KIMIA_TEST(Input_DrawAtScroll) {
  std::vector<kimia::ui::InputBinding> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::InputBinding b;
    b.action = "Action_" + std::to_string(i);
    b.primaryKey = "P" + std::to_string(i);
    v.push_back(b);
  }
  kimia::ui::drawInputPanel({0, 0, 240, 200}, v, -50);
  kimia::ui::drawInputPanel({0, 0, 240, 200}, v, 100);
}

KIMIA_TEST(Input_DrawAtPhonePortrait) {
  std::vector<kimia::ui::InputBinding> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::InputBinding b;
    b.action = "A" + std::to_string(i);
    b.primaryKey = "K";
    v.push_back(b);
  }
  kimia::ui::drawInputPanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(Input_DrawAtTabletLandscape) {
  std::vector<kimia::ui::InputBinding> v;
  for (int i = 0; i < 12; ++i) {
    kimia::ui::InputBinding b;
    b.action = "Action_" + std::to_string(i);
    b.primaryKey = "P" + std::to_string(i);
    b.secondaryKey = "S";
    v.push_back(b);
  }
  kimia::ui::drawInputPanel({0, 0, 480, 320}, v, 0);
}

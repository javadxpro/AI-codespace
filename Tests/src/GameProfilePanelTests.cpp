#include <kimia_test.h>
#include <kimia/GameProfilePanel.h>

KIMIA_TEST(GameProfile_DrawEmptyDoesNotCrash) {
  kimia::ui::drawGameProfilePanel({0, 0, 240, 200}, {}, 0);
}

KIMIA_TEST(GameProfile_DrawOneProfile) {
  std::vector<kimia::ui::GameProfileEntry> v(1);
  v[0].name = "Football";
  v[0].description = "5v5 street football";
  v[0].glyph = "F";
  v[0].isCurrent = true;
  kimia::ui::drawGameProfilePanel({0, 0, 240, 200}, v, 0);
}

KIMIA_TEST(GameProfile_DrawManyProfiles) {
  std::vector<kimia::ui::GameProfileEntry> v;
  const char* names[] = {"Football", "Golf", "Street kids", "Bowling", "Tennis"};
  const char* descs[] = {
    "5v5 street football",
    "9-hole minigolf",
    "1v1 street fight",
    "10-pin bowling",
    "2-player tennis"
  };
  for (int i = 0; i < 5; ++i) {
    kimia::ui::GameProfileEntry p;
    p.name = names[i];
    p.description = descs[i];
    p.glyph = std::string(1, names[i][0]);
    p.isCurrent = (i == 0);
    v.push_back(p);
  }
  kimia::ui::drawGameProfilePanel({0, 0, 280, 240}, v, 0);
}

KIMIA_TEST(GameProfile_DrawWithLongDescription) {
  std::vector<kimia::ui::GameProfileEntry> v(1);
  v[0].name = "X";
  v[0].description = "A_Very_Long_Description_That_Goes_On_For_A_Long_Time_And_Tests_Truncation";
  v[0].glyph = "L";
  kimia::ui::drawGameProfilePanel({0, 0, 240, 200}, v, 0);
}

KIMIA_TEST(GameProfile_DrawAtScroll) {
  std::vector<kimia::ui::GameProfileEntry> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::GameProfileEntry p;
    p.name = "P" + std::to_string(i);
    p.description = "d";
    p.glyph = "X";
    v.push_back(p);
  }
  kimia::ui::drawGameProfilePanel({0, 0, 240, 200}, v, -50);
  kimia::ui::drawGameProfilePanel({0, 0, 240, 200}, v, 50);
}

KIMIA_TEST(GameProfile_DrawAtPhonePortrait) {
  std::vector<kimia::ui::GameProfileEntry> v;
  for (int i = 0; i < 3; ++i) {
    kimia::ui::GameProfileEntry p;
    p.name = "P" + std::to_string(i);
    p.description = "d" + std::to_string(i);
    p.glyph = "X";
    v.push_back(p);
  }
  kimia::ui::drawGameProfilePanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(GameProfile_DrawAtTabletLandscape) {
  std::vector<kimia::ui::GameProfileEntry> v;
  for (int i = 0; i < 8; ++i) {
    kimia::ui::GameProfileEntry p;
    p.name = "Profile_" + std::to_string(i);
    p.description = "Description " + std::to_string(i);
    p.glyph = std::string(1, 'A' + i);
    p.isCurrent = (i == 3);
    v.push_back(p);
  }
  kimia::ui::drawGameProfilePanel({0, 0, 480, 320}, v, 0);
}

#include <kimia_test.h>
#include <kimia/AccountPanel.h>

KIMIA_TEST(Account_DrawEmptyDoesNotCrash) {
  kimia::ui::AccountInfo a;
  kimia::ui::drawAccountPanel({0, 0, 240, 200}, a);
}

KIMIA_TEST(Account_DrawWithInfo) {
  kimia::ui::AccountInfo a;
  a.username = "alice";
  a.email = "alice@kimia.studio";
  a.avatarGlyph = "A";
  a.projectCount = 5;
  a.publishedGameCount = 2;
  a.isOnline = true;
  kimia::ui::drawAccountPanel({0, 0, 280, 240}, a);
}

KIMIA_TEST(Account_DrawOffline) {
  kimia::ui::AccountInfo a;
  a.username = "bob";
  a.email = "bob@example.com";
  a.avatarGlyph = "B";
  a.isOnline = false;
  a.projectCount = 12;
  a.publishedGameCount = 8;
  kimia::ui::drawAccountPanel({0, 0, 280, 240}, a);
}

KIMIA_TEST(Account_DrawWithEmptyFields) {
  kimia::ui::AccountInfo a;
  a.username = "";
  a.email = "";
  a.avatarGlyph = "";
  kimia::ui::drawAccountPanel({0, 0, 240, 200}, a);
}

KIMIA_TEST(Account_DrawAtPhonePortrait) {
  kimia::ui::AccountInfo a;
  a.username = "u";
  a.avatarGlyph = "U";
  kimia::ui::drawAccountPanel({0, 0, 240, 320}, a);
}

KIMIA_TEST(Account_DrawAtTabletLandscape) {
  kimia::ui::AccountInfo a;
  a.username = "studio";
  a.email = "studio@kimia.games";
  a.avatarGlyph = "S";
  a.projectCount = 100;
  a.publishedGameCount = 50;
  a.isOnline = true;
  kimia::ui::drawAccountPanel({0, 0, 480, 320}, a);
}

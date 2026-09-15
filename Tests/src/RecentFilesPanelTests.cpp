#include <kimia_test.h>
#include <kimia/RecentFilesPanel.h>

KIMIA_TEST(Recent_DrawEmptyDoesNotCrash) {
  kimia::ui::drawRecentFilesPanel({0, 0, 240, 200}, {});
}

KIMIA_TEST(Recent_DrawOneFile) {
  std::vector<kimia::ui::RecentFile> v(1);
  v[0].path = "scenes/main.kimia";
  v[0].thumbnail = "S";
  v[0].lastOpenedSec = 30.0;
  kimia::ui::drawRecentFilesPanel({0, 0, 240, 200}, v);
}

KIMIA_TEST(Recent_DrawManyFiles) {
  std::vector<kimia::ui::RecentFile> v;
  const char* paths[] = {
    "scenes/main.kimia",
    "scenes/test.kimia",
    "scenes/level1.kimia",
    "scenes/level2.kimia",
    "scenes/boss.kimia",
  };
  for (int i = 0; i < 5; ++i) {
    kimia::ui::RecentFile f;
    f.path = paths[i];
    f.thumbnail = "S";
    f.lastOpenedSec = static_cast<kimia::f64>(i) * 100.0;
    v.push_back(f);
  }
  kimia::ui::drawRecentFilesPanel({0, 0, 280, 200}, v);
}

KIMIA_TEST(Recent_DrawWithOldFiles) {
  std::vector<kimia::ui::RecentFile> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::RecentFile f;
    f.path = "old_" + std::to_string(i) + ".kimia";
    f.thumbnail = "O";
    f.lastOpenedSec = static_cast<kimia::f64>(i + 1) * 86400.0;
    v.push_back(f);
  }
  kimia::ui::drawRecentFilesPanel({0, 0, 240, 200}, v);
}

KIMIA_TEST(Recent_DrawAtPhonePortrait) {
  std::vector<kimia::ui::RecentFile> v;
  for (int i = 0; i < 4; ++i) {
    kimia::ui::RecentFile f;
    f.path = "p" + std::to_string(i);
    f.thumbnail = "X";
    v.push_back(f);
  }
  kimia::ui::drawRecentFilesPanel({0, 0, 240, 320}, v);
}

KIMIA_TEST(Recent_DrawAtTabletLandscape) {
  std::vector<kimia::ui::RecentFile> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::RecentFile f;
    f.path = "scenes/file_" + std::to_string(i) + ".kimia";
    f.thumbnail = std::string(1, 'A' + i);
    f.lastOpenedSec = static_cast<kimia::f64>(i * 1000);
    v.push_back(f);
  }
  kimia::ui::drawRecentFilesPanel({0, 0, 480, 320}, v);
}

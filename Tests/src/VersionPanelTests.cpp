// VersionPanel tests — see Engine/EditorUI/include/kimia/VersionPanel.h.

#include <kimia_test.h>
#include <kimia/VersionPanel.h>

KIMIA_TEST(VersionPanel_DrawEmptyDoesNotCrash) {
  kimia::ui::VersionInfo v;
  kimia::ui::drawVersionPanel({0, 0, 320, 120}, v);
}

KIMIA_TEST(VersionPanel_DrawWithInfoDoesNotCrash) {
  kimia::ui::VersionInfo v;
  v.version = "0.30.0";
  v.buildDate = "2026-09-15";
  v.gitCommit = "a09125f";
  v.targetDevice = "Poco X3 Pro (arm64-v8a)";
  kimia::ui::drawVersionPanel({0, 0, 320, 120}, v);
}

KIMIA_TEST(VersionPanel_DrawWithLongCommitDoesNotCrash) {
  // A 40-char SHA1 commit — exercises the layout.
  kimia::ui::VersionInfo v;
  v.version = "0.30.0";
  v.gitCommit = "0123456789abcdef0123456789abcdef01234567";
  v.buildDate = "2026-09-15";
  v.targetDevice = "Poco X3 Pro";
  kimia::ui::drawVersionPanel({0, 0, 360, 120}, v);
}

KIMIA_TEST(VersionPanel_DrawWithEmptyFieldsDoesNotCrash) {
  kimia::ui::VersionInfo v;
  v.version = "0.30.0";
  v.gitCommit = "";
  v.buildDate = "";
  v.targetDevice = "";
  kimia::ui::drawVersionPanel({0, 0, 320, 120}, v);
}

KIMIA_TEST(VersionPanel_DrawAtPhonePortrait) {
  kimia::ui::VersionInfo v;
  v.version = "0.30.0";
  v.targetDevice = "Poco X3 Pro";
  kimia::ui::drawVersionPanel({0, 0, 240, 140}, v);
}

KIMIA_TEST(VersionPanel_DrawAtTabletLandscape) {
  kimia::ui::VersionInfo v;
  v.version = "0.30.0";
  v.targetDevice = "Poco X3 Pro";
  kimia::ui::drawVersionPanel({0, 0, 480, 140}, v);
}

KIMIA_TEST(VersionPanel_DefaultInfoHasEmptyStrings) {
  // Default-constructed VersionInfo: every pointer is null.
  // (The header defaults them to "" via in-class initializers.)
  kimia::ui::VersionInfo v;
  KIMIA_REQUIRE(v.version != nullptr);
  KIMIA_REQUIRE(v.buildDate != nullptr);
  KIMIA_REQUIRE(v.gitCommit != nullptr);
  KIMIA_REQUIRE(v.targetDevice != nullptr);
}

#include <kimia_test.h>
#include <kimia/BuildSettingsPanel.h>

KIMIA_TEST(BuildSettings_DrawDefaultDoesNotCrash) {
  kimia::ui::BuildSettingsProps p;
  kimia::ui::drawBuildSettingsPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(BuildSettings_DrawAndroidRelease) {
  // The default Poco X3 Pro target.
  kimia::ui::BuildSettingsProps p;
  p.platform = kimia::ui::BuildPlatform::Android;
  p.config = kimia::ui::BuildConfig::Release;
  p.embedAssets = true;
  p.arm64Only = true;
  p.compressAssets = true;
  p.outputName = "kimia-game.apk";
  p.version = "0.30.0";
  kimia::ui::drawBuildSettingsPanel({0, 0, 280, 280}, p);
}

KIMIA_TEST(BuildSettings_DrawWindowsDebug) {
  kimia::ui::BuildSettingsProps p;
  p.platform = kimia::ui::BuildPlatform::Windows;
  p.config = kimia::ui::BuildConfig::Debug;
  p.arm64Only = false;
  p.stripDebugSymbols = false;
  p.outputName = "kimia-game.exe";
  kimia::ui::drawBuildSettingsPanel({0, 0, 280, 280}, p);
}

KIMIA_TEST(BuildSettings_DrawLinuxProfile) {
  kimia::ui::BuildSettingsProps p;
  p.platform = kimia::ui::BuildPlatform::Linux;
  p.config = kimia::ui::BuildConfig::Profile;
  p.arm64Only = false;
  p.compressAssets = false;
  kimia::ui::drawBuildSettingsPanel({0, 0, 280, 280}, p);
}

KIMIA_TEST(BuildSettings_DrawWebRelease) {
  kimia::ui::BuildSettingsProps p;
  p.platform = kimia::ui::BuildPlatform::Web;
  p.config = kimia::ui::BuildConfig::Release;
  p.outputName = "kimia-game.html";
  kimia::ui::drawBuildSettingsPanel({0, 0, 280, 280}, p);
}

KIMIA_TEST(BuildSettings_DrawAtPhonePortrait) {
  kimia::ui::BuildSettingsProps p;
  kimia::ui::drawBuildSettingsPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(BuildSettings_DrawAtTabletLandscape) {
  kimia::ui::BuildSettingsProps p;
  p.platform = kimia::ui::BuildPlatform::Android;
  p.config = kimia::ui::BuildConfig::Release;
  kimia::ui::drawBuildSettingsPanel({0, 0, 480, 280}, p);
}

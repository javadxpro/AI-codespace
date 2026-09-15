#include <kimia_test.h>
#include <kimia/PreferencesPanel.h>

KIMIA_TEST(Prefs_DrawDefaultDoesNotCrash) {
  kimia::ui::PrefsValues p;
  kimia::ui::drawPreferencesPanel({0, 0, 280, 240}, p);
}

KIMIA_TEST(Prefs_DrawEverythingOn) {
  kimia::ui::PrefsValues p;
  p.autosave = true;
  p.autoReloadOnChange = true;
  p.showLineNumbers = true;
  p.enableVibration = true;
  p.enableSounds = true;
  p.uiScale = 1.25f;
  p.language = 1; // Persian
  p.renderBackend = 0; // GLES3
  kimia::ui::drawPreferencesPanel({0, 0, 320, 280}, p);
}

KIMIA_TEST(Prefs_DrawEverythingOff) {
  kimia::ui::PrefsValues p;
  p.autosave = false;
  p.autoReloadOnChange = false;
  p.showLineNumbers = false;
  p.enableVibration = false;
  p.enableSounds = false;
  p.uiScale = 0.75f;
  p.language = 2; // Japanese
  p.renderBackend = 2; // Software
  kimia::ui::drawPreferencesPanel({0, 0, 320, 280}, p);
}

KIMIA_TEST(Prefs_DrawWithUnknownLanguageAndBackend) {
  // Out-of-range values should not crash the formatter.
  kimia::ui::PrefsValues p;
  p.language = 99;
  p.renderBackend = -5;
  kimia::ui::drawPreferencesPanel({0, 0, 280, 240}, p);
}

KIMIA_TEST(Prefs_DrawAtPhonePortrait) {
  kimia::ui::PrefsValues p;
  kimia::ui::drawPreferencesPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(Prefs_DrawAtTabletLandscape) {
  kimia::ui::PrefsValues p;
  p.uiScale = 1.5f;
  p.language = 1;
  p.renderBackend = 1;
  kimia::ui::drawPreferencesPanel({0, 0, 480, 320}, p);
}

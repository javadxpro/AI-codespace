#include <kimia_test.h>
#include <kimia/SceneSettingsPanel.h>

KIMIA_TEST(SceneSettings_DrawDefaultDoesNotCrash) {
  kimia::ui::SceneSettingsProps p;
  kimia::ui::drawSceneSettingsPanel({0, 0, 240, 280}, p);
}

KIMIA_TEST(SceneSettings_DrawWithFog) {
  kimia::ui::SceneSettingsProps p;
  p.sceneName = "main";
  p.gravity = -20.0f;
  p.ambientColor = {0.1, 0.1, 0.1};
  p.ambientIntensity = 0.5f;
  p.fogEnabled = true;
  p.fogStart = 10.0f;
  p.fogEnd = 100.0f;
  p.fogColor = {0.5, 0.5, 0.5};
  p.physicsEnabled = true;
  p.autoSave = true;
  p.autoSaveIntervalSec = 30;
  kimia::ui::drawSceneSettingsPanel({0, 0, 280, 320}, p);
}

KIMIA_TEST(SceneSettings_DrawWithNoPhysics) {
  kimia::ui::SceneSettingsProps p;
  p.physicsEnabled = false;
  p.gravity = 0.0f;
  kimia::ui::drawSceneSettingsPanel({0, 0, 240, 280}, p);
}

KIMIA_TEST(SceneSettings_DrawAtPhonePortrait) {
  kimia::ui::SceneSettingsProps p;
  kimia::ui::drawSceneSettingsPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(SceneSettings_DrawAtTabletLandscape) {
  kimia::ui::SceneSettingsProps p;
  p.sceneName = "football_match";
  p.fogEnabled = true;
  kimia::ui::drawSceneSettingsPanel({0, 0, 480, 320}, p);
}

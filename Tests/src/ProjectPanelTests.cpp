#include <kimia_test.h>
#include <kimia/ProjectPanel.h>

KIMIA_TEST(Project_DrawDefaultDoesNotCrash) {
  kimia::ui::ProjectProps p;
  kimia::ui::drawProjectPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(Project_DrawWithLongPaths) {
  kimia::ui::ProjectProps p;
  p.projectName = "My Very Long Game Name With Spaces";
  p.scenesDir = "/data/data/com.kimia.world/files/scenes";
  p.assetsDir = "/data/data/com.kimia.world/files/assets";
  p.useEditorUi = true;
  p.useNativeEditor = true;
  kimia::ui::drawProjectPanel({0, 0, 320, 280}, p);
}

KIMIA_TEST(Project_DrawWithEmptyFields) {
  kimia::ui::ProjectProps p;
  p.projectName = "";
  p.companyName = "";
  p.version = "";
  p.scenesDir = "";
  p.assetsDir = "";
  kimia::ui::drawProjectPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(Project_DrawAtPhonePortrait) {
  kimia::ui::ProjectProps p;
  kimia::ui::drawProjectPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(Project_DrawAtTabletLandscape) {
  kimia::ui::ProjectProps p;
  p.projectName = "Street Soccer";
  p.companyName = "KIMIA Studios";
  kimia::ui::drawProjectPanel({0, 0, 480, 280}, p);
}

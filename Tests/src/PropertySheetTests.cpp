// PropertySheet tests — see Engine/EditorUI/include/kimia/PropertySheet.h.

#include <kimia_test.h>
#include <kimia/PropertySheet.h>

KIMIA_TEST(PropertySheet_DrawDefaultDoesNotCrash) {
  kimia::ui::EntityProps p;
  kimia::ui::drawPropertySheet({0, 0, 240, 320}, p);
}

KIMIA_TEST(PropertySheet_DrawPlayerDoesNotCrash) {
  kimia::ui::EntityProps p;
  p.name = "Player_1";
  p.position = {1.0, 2.0, 3.0};
  p.rotationEuler = {0.0, 90.0, 0.0};
  p.scale = {1.0, 1.0, 1.0};
  p.color = {0.2, 0.6, 0.9};
  p.roughness = 0.3;
  p.metalness = 0.0;
  p.isPlayer = true;
  kimia::ui::drawPropertySheet({0, 0, 280, 360}, p);
}

KIMIA_TEST(PropertySheet_DrawBallDoesNotCrash) {
  kimia::ui::EntityProps p;
  p.name = "Ball";
  p.position = {0.0, 1.0, 0.0};
  p.scale = {0.3, 0.3, 0.3};
  p.color = {1.0, 1.0, 1.0};
  p.roughness = 0.7;
  p.metalness = 0.0;
  p.isBall = true;
  kimia::ui::drawPropertySheet({0, 0, 280, 360}, p);
}

KIMIA_TEST(PropertySheet_DrawLockedCubeDoesNotCrash) {
  kimia::ui::EntityProps p;
  p.name = "Cube_Locked";
  p.position = {-2.0, 0.5, 1.0};
  p.scale = {2.0, 2.0, 2.0};
  p.color = {0.8, 0.4, 0.1};
  p.roughness = 0.9;
  p.metalness = 0.2;
  p.locked = true;
  kimia::ui::drawPropertySheet({0, 0, 280, 360}, p);
}

KIMIA_TEST(PropertySheet_DrawWithVeryLongName) {
  kimia::ui::EntityProps p;
  p.name = "Very_Long_Entity_Name_That_Will_Be_Truncated_By_The_Panel_Title_Bar_ABCDEFGHIJ";
  kimia::ui::drawPropertySheet({0, 0, 200, 320}, p);
}

KIMIA_TEST(PropertySheet_DrawAtPhonePortrait) {
  kimia::ui::EntityProps p;
  p.name = "P";
  p.isPlayer = true;
  kimia::ui::drawPropertySheet({0, 0, 240, 320}, p);
}

KIMIA_TEST(PropertySheet_DrawAtTabletLandscape) {
  kimia::ui::EntityProps p;
  p.name = "Big_Cube";
  p.scale = {10.0, 10.0, 10.0};
  kimia::ui::drawPropertySheet({0, 0, 800, 600}, p);
}

KIMIA_TEST(PropertySheet_DrawTinyRectStillDoesNotCrash) {
  // Degenerate rect — should not crash.
  kimia::ui::EntityProps p;
  p.name = "x";
  kimia::ui::drawPropertySheet({0, 0, 10, 10}, p);
}

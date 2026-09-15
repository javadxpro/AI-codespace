#include <kimia_test.h>
#include <kimia/InspectorModePanel.h>

KIMIA_TEST(InspectorMode_DrawProperties) {
  kimia::ui::drawInspectorModePanel({0, 0, 200, 200},
                                    kimia::ui::InspectorTab::Properties);
}

KIMIA_TEST(InspectorMode_DrawPhysics) {
  kimia::ui::drawInspectorModePanel({0, 0, 200, 200},
                                    kimia::ui::InspectorTab::Physics);
}

KIMIA_TEST(InspectorMode_DrawMaterial) {
  kimia::ui::drawInspectorModePanel({0, 0, 200, 200},
                                    kimia::ui::InspectorTab::Material);
}

KIMIA_TEST(InspectorMode_DrawParticle) {
  kimia::ui::drawInspectorModePanel({0, 0, 200, 200},
                                    kimia::ui::InspectorTab::Particle);
}

KIMIA_TEST(InspectorMode_DrawScript) {
  kimia::ui::drawInspectorModePanel({0, 0, 200, 200},
                                    kimia::ui::InspectorTab::Script);
}

KIMIA_TEST(InspectorMode_DrawAtPhonePortrait) {
  kimia::ui::drawInspectorModePanel({0, 0, 200, 320},
                                    kimia::ui::InspectorTab::Properties);
}

KIMIA_TEST(InspectorMode_DrawAtTabletLandscape) {
  kimia::ui::drawInspectorModePanel({0, 0, 280, 240},
                                    kimia::ui::InspectorTab::Material);
}

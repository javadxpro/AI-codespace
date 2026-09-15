#include <kimia_test.h>
#include <kimia/PhysicsDebugPanel.h>

KIMIA_TEST(PhysicsDebug_DrawOff) {
  kimia::ui::drawPhysicsDebugPanel({0, 0, 480, 80},
                                   kimia::ui::DebugDrawMode::Off);
}

KIMIA_TEST(PhysicsDebug_DrawWireframe) {
  kimia::ui::drawPhysicsDebugPanel({0, 0, 480, 80},
                                   kimia::ui::DebugDrawMode::Wireframe);
}

KIMIA_TEST(PhysicsDebug_DrawNormals) {
  kimia::ui::drawPhysicsDebugPanel({0, 0, 480, 80},
                                   kimia::ui::DebugDrawMode::Normals);
}

KIMIA_TEST(PhysicsDebug_DrawContacts) {
  kimia::ui::drawPhysicsDebugPanel({0, 0, 480, 80},
                                   kimia::ui::DebugDrawMode::Contacts);
}

KIMIA_TEST(PhysicsDebug_DrawAll) {
  kimia::ui::drawPhysicsDebugPanel({0, 0, 480, 80},
                                   kimia::ui::DebugDrawMode::All);
}

KIMIA_TEST(PhysicsDebug_DrawAtPhonePortrait) {
  // Phone-portrait might be too narrow for 5 buttons; must not crash.
  kimia::ui::drawPhysicsDebugPanel({0, 0, 240, 100},
                                   kimia::ui::DebugDrawMode::All);
}

KIMIA_TEST(PhysicsDebug_DrawAtTabletLandscape) {
  kimia::ui::drawPhysicsDebugPanel({0, 0, 800, 100},
                                   kimia::ui::DebugDrawMode::All);
}

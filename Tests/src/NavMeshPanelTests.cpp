#include <kimia_test.h>
#include <kimia/NavMeshPanel.h>

KIMIA_TEST(NavMeshPanel_DrawDefaultDoesNotCrash) {
  kimia::ui::NavMeshProps p;
  kimia::ui::drawNavMeshPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(NavMeshPanel_DrawBuilt) {
  kimia::ui::NavMeshProps p;
  p.built = true;
  p.polyCount = 1200;
  p.vertCount = 800;
  kimia::ui::drawNavMeshPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(NavMeshPanel_DrawWithLargeValues) {
  kimia::ui::NavMeshProps p;
  p.cellSize = 0.05f;
  p.tileSize = 256;
  p.agentHeight = 5.0f;
  p.built = true;
  p.polyCount = 50000;
  p.vertCount = 25000;
  kimia::ui::drawNavMeshPanel({0, 0, 280, 280}, p);
}

KIMIA_TEST(NavMeshPanel_DrawAtPhonePortrait) {
  kimia::ui::NavMeshProps p;
  kimia::ui::drawNavMeshPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(NavMeshPanel_DrawAtTabletLandscape) {
  kimia::ui::NavMeshProps p;
  p.built = true;
  p.polyCount = 500;
  kimia::ui::drawNavMeshPanel({0, 0, 480, 280}, p);
}

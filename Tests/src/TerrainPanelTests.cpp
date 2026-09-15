#include <kimia_test.h>
#include <kimia/TerrainPanel.h>

KIMIA_TEST(TerrainPanel_DrawDefaultDoesNotCrash) {
  kimia::ui::TerrainProps p;
  kimia::ui::drawTerrainPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(TerrainPanel_DrawHighResolution) {
  kimia::ui::TerrainProps p;
  p.width = 256;
  p.depth = 256;
  p.spacing = 0.5f;
  p.heightScale = 25.0f;
  p.noiseFrequency = 0.05f;
  p.seed = 42;
  p.wireframe = true;
  kimia::ui::drawTerrainPanel({0, 0, 280, 280}, p);
}

KIMIA_TEST(TerrainPanel_DrawLowResolution) {
  kimia::ui::TerrainProps p;
  p.width = 16;
  p.depth = 16;
  p.spacing = 4.0f;
  kimia::ui::drawTerrainPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(TerrainPanel_DrawWithCustomColor) {
  kimia::ui::TerrainProps p;
  p.baseColor = {0.8, 0.7, 0.5};
  kimia::ui::drawTerrainPanel({0, 0, 240, 240}, p);
}

KIMIA_TEST(TerrainPanel_DrawAtPhonePortrait) {
  kimia::ui::TerrainProps p;
  kimia::ui::drawTerrainPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(TerrainPanel_DrawAtTabletLandscape) {
  kimia::ui::TerrainProps p;
  p.width = 128;
  p.depth = 128;
  kimia::ui::drawTerrainPanel({0, 0, 480, 280}, p);
}

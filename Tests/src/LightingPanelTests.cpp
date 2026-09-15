#include <kimia_test.h>
#include <kimia/LightingPanel.h>

KIMIA_TEST(LightingPanel_DrawDefaultDoesNotCrash) {
  kimia::ui::LightProps p;
  kimia::ui::drawLightingPanel({0, 0, 240, 280}, p);
}

KIMIA_TEST(LightingPanel_DrawDirectional) {
  kimia::ui::LightProps p;
  p.kind = kimia::ui::LightKind::Directional;
  p.direction = {0.5, -1.0, 0.3};
  p.color = {1.0, 0.95, 0.85};
  p.intensity = 2.5f;
  p.castsShadows = true;
  kimia::ui::drawLightingPanel({0, 0, 280, 320}, p);
}

KIMIA_TEST(LightingPanel_DrawPoint) {
  kimia::ui::LightProps p;
  p.kind = kimia::ui::LightKind::Point;
  p.position = {2.0, 5.0, 1.0};
  p.color = {0.4, 0.6, 1.0};
  p.intensity = 5.0f;
  p.range = 8.0f;
  p.castsShadows = false;
  kimia::ui::drawLightingPanel({0, 0, 280, 320}, p);
}

KIMIA_TEST(LightingPanel_DrawSpot) {
  kimia::ui::LightProps p;
  p.kind = kimia::ui::LightKind::Spot;
  p.position = {0.0, 6.0, 0.0};
  p.direction = {0.0, -1.0, 0.0};
  p.spotAngleDeg = 30.0f;
  p.intensity = 10.0f;
  p.range = 20.0f;
  kimia::ui::drawLightingPanel({0, 0, 280, 360}, p);
}

KIMIA_TEST(LightingPanel_DrawAtPhonePortrait) {
  kimia::ui::LightProps p;
  p.kind = kimia::ui::LightKind::Point;
  kimia::ui::drawLightingPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(LightingPanel_DrawAtTabletLandscape) {
  kimia::ui::LightProps p;
  p.kind = kimia::ui::LightKind::Spot;
  p.spotAngleDeg = 60.0f;
  kimia::ui::drawLightingPanel({0, 0, 480, 320}, p);
}

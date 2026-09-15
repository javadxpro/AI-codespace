// ParticlePanel tests — see Engine/EditorUI/include/kimia/ParticlePanel.h.

#include <kimia_test.h>
#include <kimia/ParticlePanel.h>

KIMIA_TEST(ParticlePanel_DrawDefaultDoesNotCrash) {
  kimia::ui::ParticleProps p;
  kimia::ui::ParticleProps e;
  KIMIA_REQUIRE(!kimia::ui::drawParticlePanel({0, 0, 200, 220}, p, e));
}

KIMIA_TEST(ParticlePanel_DrawSmokeEmitter) {
  kimia::ui::ParticleProps p;
  p.spawnRate = 5.0f;
  p.lifeMin = 2.0f;
  p.lifeMax = 4.0f;
  p.sizeMin = 0.5f;
  p.sizeMax = 1.0f;
  p.gravity = 1.0f;        // floats up like smoke
  p.colorStart = {0.8, 0.8, 0.8};
  p.colorEnd = {0.4, 0.4, 0.4};
  p.additive = false;
  kimia::ui::ParticleProps e = p;
  KIMIA_REQUIRE(!kimia::ui::drawParticlePanel({0, 0, 240, 240}, p, e));
}

KIMIA_TEST(ParticlePanel_DrawFireEmitter) {
  kimia::ui::ParticleProps p;
  p.spawnRate = 30.0f;
  p.lifeMin = 0.3f;
  p.lifeMax = 1.0f;
  p.gravity = -2.0f;
  p.colorStart = {1.0, 0.6, 0.1};
  p.colorEnd = {0.4, 0.05, 0.0};
  p.additive = true;
  kimia::ui::ParticleProps e = p;
  KIMIA_REQUIRE(!kimia::ui::drawParticlePanel({0, 0, 240, 240}, p, e));
}

KIMIA_TEST(ParticlePanel_DrawSparkEmitter) {
  kimia::ui::ParticleProps p;
  p.spawnRate = 50.0f;
  p.lifeMin = 0.05f;
  p.lifeMax = 0.2f;
  p.speedMin = 5.0f;
  p.speedMax = 10.0f;
  p.gravity = -9.8f;
  p.colorStart = {1.0, 1.0, 0.8};
  p.colorEnd = {1.0, 0.4, 0.0};
  p.looped = false;
  kimia::ui::ParticleProps e = p;
  KIMIA_REQUIRE(!kimia::ui::drawParticlePanel({0, 0, 240, 240}, p, e));
}

KIMIA_TEST(ParticlePanel_DrawAtPhonePortrait) {
  kimia::ui::ParticleProps p;
  kimia::ui::ParticleProps e;
  KIMIA_REQUIRE(!kimia::ui::drawParticlePanel({0, 0, 240, 320}, p, e));
}

KIMIA_TEST(ParticlePanel_DrawAtTabletLandscape) {
  kimia::ui::ParticleProps p;
  kimia::ui::ParticleProps e;
  KIMIA_REQUIRE(!kimia::ui::drawParticlePanel({0, 0, 800, 280}, p, e));
}

#include <kimia_test.h>
#include <kimia/PostProcessPanel.h>

KIMIA_TEST(PostProcess_DrawDefaultDoesNotCrash) {
  kimia::ui::PostProcessProps p;
  kimia::ui::drawPostProcessPanel({0, 0, 220, 280}, p);
}

KIMIA_TEST(PostProcess_DrawAllOn) {
  kimia::ui::PostProcessProps p;
  p.bloom = true;
  p.bloomIntensity = 1.2f;
  p.bloomThreshold = 0.8f;
  p.tonemap = true;
  p.exposure = 1.5f;
  p.ssao = true;
  p.ssaoRadius = 1.0f;
  p.fxaa = true;
  p.vignette = true;
  p.vignetteIntensity = 0.6f;
  p.motionBlur = true;
  kimia::ui::drawPostProcessPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(PostProcess_DrawAllOff) {
  kimia::ui::PostProcessProps p;
  p.bloom = false;
  p.tonemap = false;
  p.ssao = false;
  p.fxaa = false;
  p.vignette = false;
  p.motionBlur = false;
  kimia::ui::drawPostProcessPanel({0, 0, 220, 280}, p);
}

KIMIA_TEST(PostProcess_DrawAtPhonePortrait) {
  kimia::ui::PostProcessProps p;
  kimia::ui::drawPostProcessPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(PostProcess_DrawAtTabletLandscape) {
  kimia::ui::PostProcessProps p;
  p.bloom = true;
  p.vignette = true;
  kimia::ui::drawPostProcessPanel({0, 0, 320, 280}, p);
}

#include <kimia_test.h>
#include <kimia/RenderSettingsPanel.h>

KIMIA_TEST(RenderSettings_DrawDefaultDoesNotCrash) {
  kimia::ui::RenderSettingsProps p;
  kimia::ui::drawRenderSettingsPanel({0, 0, 240, 220}, p);
}

KIMIA_TEST(RenderSettings_DrawHighQuality) {
  kimia::ui::RenderSettingsProps p;
  p.antialias = kimia::ui::AntialiasMode::MSAA8;
  p.shadows = kimia::ui::ShadowQuality::High;
  p.textures = kimia::ui::TextureQuality::High;
  p.vsync = true;
  p.resolutionScale = 1.0f;
  p.maxFps = 144;
  p.hdr = true;
  p.softParticles = true;
  kimia::ui::drawRenderSettingsPanel({0, 0, 280, 240}, p);
}

KIMIA_TEST(RenderSettings_DrawLowQuality) {
  kimia::ui::RenderSettingsProps p;
  p.antialias = kimia::ui::AntialiasMode::None;
  p.shadows = kimia::ui::ShadowQuality::Off;
  p.textures = kimia::ui::TextureQuality::Low;
  p.vsync = false;
  p.resolutionScale = 0.5f;
  p.maxFps = 30;
  p.hdr = false;
  p.softParticles = false;
  kimia::ui::drawRenderSettingsPanel({0, 0, 240, 220}, p);
}

KIMIA_TEST(RenderSettings_DrawAtPhonePortrait) {
  kimia::ui::RenderSettingsProps p;
  kimia::ui::drawRenderSettingsPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(RenderSettings_DrawAtTabletLandscape) {
  kimia::ui::RenderSettingsProps p;
  p.maxFps = 60;
  kimia::ui::drawRenderSettingsPanel({0, 0, 320, 280}, p);
}

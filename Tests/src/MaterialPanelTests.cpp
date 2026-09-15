#include <kimia_test.h>
#include <kimia/MaterialPanel.h>

KIMIA_TEST(MaterialPanel_DrawDefaultDoesNotCrash) {
  kimia::ui::MaterialProps p;
  p.name = "Default";
  kimia::ui::drawMaterialPanel({0, 0, 240, 280}, p);
}

KIMIA_TEST(MaterialPanel_DrawMetallic) {
  kimia::ui::MaterialProps p;
  p.name = "Metal";
  p.albedo = {0.8, 0.8, 0.85};
  p.roughness = 0.1f;
  p.metalness = 1.0f;
  kimia::ui::drawMaterialPanel({0, 0, 240, 280}, p);
}

KIMIA_TEST(MaterialPanel_DrawEmissiveGlass) {
  kimia::ui::MaterialProps p;
  p.name = "Glow";
  p.albedo = {0.2, 0.4, 0.8};
  p.emissive = {0.0, 1.0, 0.5};
  p.opacity = 0.5f;
  p.blend = kimia::ui::MaterialBlend::Alpha;
  p.albedoTexture = "textures/glow.png";
  p.normalTexture = "textures/glow_n.png";
  p.doubleSided = true;
  kimia::ui::drawMaterialPanel({0, 0, 280, 320}, p);
}

KIMIA_TEST(MaterialPanel_DrawAdditive) {
  kimia::ui::MaterialProps p;
  p.blend = kimia::ui::MaterialBlend::Additive;
  p.emissive = {1.0, 0.5, 0.0};
  kimia::ui::drawMaterialPanel({0, 0, 240, 280}, p);
}

KIMIA_TEST(MaterialPanel_DrawAtPhonePortrait) {
  kimia::ui::MaterialProps p;
  p.name = "X";
  kimia::ui::drawMaterialPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(MaterialPanel_DrawAtTabletLandscape) {
  kimia::ui::MaterialProps p;
  p.name = "Big";
  p.roughness = 0.3f;
  p.metalness = 0.7f;
  kimia::ui::drawMaterialPanel({0, 0, 480, 320}, p);
}

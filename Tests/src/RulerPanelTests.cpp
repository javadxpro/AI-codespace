#include <kimia_test.h>
#include <kimia/RulerPanel.h>

KIMIA_TEST(Ruler_DrawHorizontalDoesNotCrash) {
  kimia::ui::drawRulerHorizontal({0, 0, 400, 16}, 0.0f, 32.0f);
}

KIMIA_TEST(Ruler_DrawVerticalDoesNotCrash) {
  kimia::ui::drawRulerVertical({0, 0, 16, 400}, 0.0f, 32.0f);
}

KIMIA_TEST(Ruler_DrawHorizontalAtZoomIn) {
  // Higher ppu -> smaller steps.
  kimia::ui::drawRulerHorizontal({0, 0, 400, 16}, 200.0f, 128.0f);
}

KIMIA_TEST(Ruler_DrawHorizontalAtZoomOut) {
  // Lower ppu -> bigger steps.
  kimia::ui::drawRulerHorizontal({0, 0, 800, 16}, 400.0f, 4.0f);
}

KIMIA_TEST(Ruler_DrawHorizontalAtNegativeOrigin) {
  kimia::ui::drawRulerHorizontal({0, 0, 400, 16}, -200.0f, 32.0f);
}

KIMIA_TEST(Ruler_DrawVerticalAtNegativeOrigin) {
  kimia::ui::drawRulerVertical({0, 0, 16, 400}, -200.0f, 32.0f);
}

KIMIA_TEST(Ruler_DrawAtPhonePortrait) {
  // Narrow horizontal.
  kimia::ui::drawRulerHorizontal({0, 0, 240, 16}, 120.0f, 32.0f);
  // Tall vertical.
  kimia::ui::drawRulerVertical({0, 0, 16, 320}, 160.0f, 32.0f);
}

KIMIA_TEST(Ruler_DrawAtTabletLandscape) {
  kimia::ui::drawRulerHorizontal({0, 0, 1024, 16}, 512.0f, 32.0f);
  kimia::ui::drawRulerVertical({0, 0, 16, 768}, 384.0f, 32.0f);
}

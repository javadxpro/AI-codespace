#include <kimia_test.h>
#include <kimia/ColorSchemePanel.h>

KIMIA_TEST(ColorScheme_DrawDefaultDoesNotCrash) {
  kimia::ui::drawColorSchemePanel({0, 0, 240, 200});
}

KIMIA_TEST(ColorScheme_DrawWideDoesNotCrash) {
  kimia::ui::drawColorSchemePanel({0, 0, 480, 240});
}

KIMIA_TEST(ColorScheme_DrawTallDoesNotCrash) {
  kimia::ui::drawColorSchemePanel({0, 0, 200, 400});
}

KIMIA_TEST(ColorScheme_DrawAtPhonePortrait) {
  kimia::ui::drawColorSchemePanel({0, 0, 240, 320});
}

KIMIA_TEST(ColorScheme_DrawAtTabletLandscape) {
  kimia::ui::drawColorSchemePanel({0, 0, 480, 320});
}

KIMIA_TEST(ColorScheme_DrawTinyDoesNotCrash) {
  kimia::ui::drawColorSchemePanel({0, 0, 100, 100});
}

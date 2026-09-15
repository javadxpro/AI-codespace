#include <kimia_test.h>
#include <kimia/ThemePanel.h>

KIMIA_TEST(Theme_DrawDarkPro) {
  kimia::ui::drawThemePanel({0, 0, 240, 200},
                            kimia::ui::ThemeName::DarkPro);
}

KIMIA_TEST(Theme_DrawLightPro) {
  kimia::ui::drawThemePanel({0, 0, 240, 200},
                            kimia::ui::ThemeName::LightPro);
}

KIMIA_TEST(Theme_DrawSolarized) {
  kimia::ui::drawThemePanel({0, 0, 240, 200},
                            kimia::ui::ThemeName::Solarized);
}

KIMIA_TEST(Theme_DrawMonokai) {
  kimia::ui::drawThemePanel({0, 0, 240, 200},
                            kimia::ui::ThemeName::Monokai);
}

KIMIA_TEST(Theme_DrawAtPhonePortrait) {
  kimia::ui::drawThemePanel({0, 0, 240, 320},
                            kimia::ui::ThemeName::DarkPro);
}

KIMIA_TEST(Theme_DrawAtTabletLandscape) {
  kimia::ui::drawThemePanel({0, 0, 480, 320},
                            kimia::ui::ThemeName::DarkPro);
}

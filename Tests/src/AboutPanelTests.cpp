#include <kimia_test.h>
#include <kimia/AboutPanel.h>

KIMIA_TEST(About_DrawFullDoesNotCrash) {
  kimia::ui::drawAboutPanel({0, 0, 320, 280},
    "Kimia Engine", "v0.30.0",
    "2026-09-15", "Android arm64", "Adreno 640");
}

KIMIA_TEST(About_DrawEmptyStrings) {
  // All empty -> centered title and labels still draw.
  kimia::ui::drawAboutPanel({0, 0, 320, 280}, "", "", "", "", "");
}

KIMIA_TEST(About_DrawWithLongName) {
  kimia::ui::drawAboutPanel({0, 0, 320, 280},
    "Kimia Engine With A Very Long Name That Might Overflow",
    "v0.30.0", "2026-09-15", "Windows x64", "GeForce RTX 3060");
}

KIMIA_TEST(About_DrawAtPhonePortrait) {
  kimia::ui::drawAboutPanel({0, 0, 240, 320},
    "K", "v0.30", "today", "android", "Adreno");
}

KIMIA_TEST(About_DrawAtTabletLandscape) {
  kimia::ui::drawAboutPanel({0, 0, 480, 320},
    "Kimia Engine", "v0.30.0", "2026-09-15",
    "Android arm64", "Adreno 640");
}

KIMIA_TEST(About_DrawWithVeryTallRect) {
  // Should still center title nicely.
  kimia::ui::drawAboutPanel({0, 0, 320, 600},
    "Kimia Engine", "v0.30.0", "today", "android", "Adreno");
}

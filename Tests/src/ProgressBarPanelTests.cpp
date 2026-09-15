#include <kimia_test.h>
#include <kimia/ProgressBarPanel.h>

KIMIA_TEST(Progress_DrawZeroDoesNotCrash) {
  kimia::ui::drawProgressBarPanel({0, 0, 240, 36}, "Loading", 0.0f, false);
}

KIMIA_TEST(Progress_DrawHalfDoesNotCrash) {
  kimia::ui::drawProgressBarPanel({0, 0, 240, 36}, "Loading", 0.5f, false);
}

KIMIA_TEST(Progress_DrawCompleteDoesNotCrash) {
  // 1.0 → green, success colour.
  kimia::ui::drawProgressBarPanel({0, 0, 240, 36}, "Loading", 1.0f, false);
}

KIMIA_TEST(Progress_DrawOverCompleteClamps) {
  kimia::ui::drawProgressBarPanel({0, 0, 240, 36}, "Loading", 1.5f, false);
  kimia::ui::drawProgressBarPanel({0, 0, 240, 36}, "Loading", -0.5f, false);
}

KIMIA_TEST(Progress_DrawIndeterminateDoesNotCrash) {
  kimia::ui::drawProgressBarPanel({0, 0, 240, 36}, "Loading", 0.0f, true);
}

KIMIA_TEST(Progress_DrawWithEmptyLabel) {
  kimia::ui::drawProgressBarPanel({0, 0, 240, 36}, "", 0.3f, false);
}

KIMIA_TEST(Progress_DrawAtPhonePortrait) {
  kimia::ui::drawProgressBarPanel({0, 0, 240, 36}, "X", 0.7f, false);
}

KIMIA_TEST(Progress_DrawAtTabletLandscape) {
  kimia::ui::drawProgressBarPanel({0, 0, 480, 40}, "Building shaders", 0.2f, false);
}

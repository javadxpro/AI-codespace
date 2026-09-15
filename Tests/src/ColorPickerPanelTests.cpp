#include <kimia_test.h>
#include <kimia/ColorPickerPanel.h>

KIMIA_TEST(ColorPicker_DrawWhiteDoesNotCrash) {
  kimia::Vec3 rgb{1.0, 1.0, 1.0};
  kimia::ui::drawColorPickerPanel({0, 0, 240, 160}, rgb, rgb);
}

KIMIA_TEST(ColorPicker_DrawRedDoesNotCrash) {
  kimia::Vec3 rgb{1.0, 0.0, 0.0};
  kimia::ui::drawColorPickerPanel({0, 0, 240, 160}, rgb, rgb);
}

KIMIA_TEST(ColorPicker_DrawDarkDoesNotCrash) {
  kimia::Vec3 rgb{0.05, 0.05, 0.05};
  kimia::ui::drawColorPickerPanel({0, 0, 240, 160}, rgb, rgb);
}

KIMIA_TEST(ColorPicker_DrawAtPhonePortrait) {
  kimia::Vec3 rgb{0.4, 0.6, 0.8};
  kimia::ui::drawColorPickerPanel({0, 0, 240, 200}, rgb, rgb);
}

KIMIA_TEST(ColorPicker_DrawAtTabletLandscape) {
  kimia::Vec3 rgb{0.8, 0.2, 0.5};
  kimia::ui::drawColorPickerPanel({0, 0, 320, 200}, rgb, rgb);
}

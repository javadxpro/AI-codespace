#include <kimia_test.h>
#include <kimia/ColorPalettePanel.h>

KIMIA_TEST(ColorPalette_DrawEmptyDoesNotCrash) {
  kimia::ui::drawColorPalettePanel({0, 0, 280, 200}, {}, -1);
}

KIMIA_TEST(ColorPalette_DrawOneSwatch) {
  std::vector<kimia::ui::PaletteSwatch> v(1);
  v[0] = {"Red", 255, 0, 0};
  kimia::ui::drawColorPalettePanel({0, 0, 280, 200}, v, 0);
}

KIMIA_TEST(ColorPalette_DrawManySwatches) {
  std::vector<kimia::ui::PaletteSwatch> v;
  v.push_back({"Red",    255,   0,   0});
  v.push_back({"Green",    0, 255,   0});
  v.push_back({"Blue",     0,   0, 255});
  v.push_back({"Yellow", 255, 255,   0});
  v.push_back({"Cyan",     0, 255, 255});
  v.push_back({"Magenta",255,   0, 255});
  v.push_back({"White",  255, 255, 255});
  v.push_back({"Black",    0,   0,   0});
  v.push_back({"Gray",   128, 128, 128});
  v.push_back({"Olive",  128, 128,   0});
  v.push_back({"Teal",     0, 128, 128});
  v.push_back({"Purple", 128,   0, 128});
  kimia::ui::drawColorPalettePanel({0, 0, 280, 240}, v, 5);
}

KIMIA_TEST(ColorPalette_DrawWithNoSelection) {
  std::vector<kimia::ui::PaletteSwatch> v;
  v.push_back({"Red", 255, 0, 0});
  v.push_back({"Green", 0, 255, 0});
  kimia::ui::drawColorPalettePanel({0, 0, 280, 200}, v, -1);
}

KIMIA_TEST(ColorPalette_DrawWithSelectedOutOfRange) {
  std::vector<kimia::ui::PaletteSwatch> v;
  v.push_back({"A", 255, 255, 255});
  kimia::ui::drawColorPalettePanel({0, 0, 280, 200}, v, 999);
}

KIMIA_TEST(ColorPalette_DrawAtPhonePortrait) {
  std::vector<kimia::ui::PaletteSwatch> v;
  for (int i = 0; i < 20; ++i) {
    v.push_back({"C" + std::to_string(i),
                 static_cast<kimia::u8>((i * 12) & 255),
                 static_cast<kimia::u8>((i * 25) & 255),
                 static_cast<kimia::u8>((i * 50) & 255)});
  }
  kimia::ui::drawColorPalettePanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(ColorPalette_DrawAtTabletLandscape) {
  std::vector<kimia::ui::PaletteSwatch> v;
  for (int i = 0; i < 50; ++i) {
    v.push_back({"C" + std::to_string(i),
                 static_cast<kimia::u8>((i * 7) & 255),
                 static_cast<kimia::u8>((i * 13) & 255),
                 static_cast<kimia::u8>((i * 23) & 255)});
  }
  kimia::ui::drawColorPalettePanel({0, 0, 480, 320}, v, 25);
}

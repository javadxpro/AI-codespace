#include <kimia_test.h>
#include <kimia/LayerPanel.h>

KIMIA_TEST(Layer_DrawEmptyDoesNotCrash) {
  kimia::ui::drawLayerPanel({0, 0, 200, 200}, {});
}

KIMIA_TEST(Layer_DrawOneLayer) {
  std::vector<kimia::ui::LayerEntry> v(1);
  v[0].name = "Default";
  v[0].visible = true;
  v[0].locked = false;
  kimia::ui::drawLayerPanel({0, 0, 200, 200}, v);
}

KIMIA_TEST(Layer_DrawManyLayers) {
  std::vector<kimia::ui::LayerEntry> v;
  const char* names[] = {"Default", "UI", "Player", "Enemies",
                         "Props",   "Lights", "Terrain", "Hidden", "Locked"};
  for (int i = 0; i < 9; ++i) {
    kimia::ui::LayerEntry l;
    l.name = names[i];
    l.visible = (i % 3 != 0);
    l.locked = (i % 4 == 0);
    v.push_back(l);
  }
  kimia::ui::drawLayerPanel({0, 0, 200, 240}, v);
}

KIMIA_TEST(Layer_DrawAllHiddenDoesNotCrash) {
  std::vector<kimia::ui::LayerEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::LayerEntry l;
    l.name = "h_" + std::to_string(i);
    l.visible = false;
    v.push_back(l);
  }
  kimia::ui::drawLayerPanel({0, 0, 200, 200}, v);
}

KIMIA_TEST(Layer_DrawAllLockedDoesNotCrash) {
  std::vector<kimia::ui::LayerEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::LayerEntry l;
    l.name = "l_" + std::to_string(i);
    l.locked = true;
    v.push_back(l);
  }
  kimia::ui::drawLayerPanel({0, 0, 200, 200}, v);
}

KIMIA_TEST(Layer_DrawAtPhonePortrait) {
  std::vector<kimia::ui::LayerEntry> v;
  for (int i = 0; i < 6; ++i) {
    kimia::ui::LayerEntry l;
    l.name = "L" + std::to_string(i);
    v.push_back(l);
  }
  kimia::ui::drawLayerPanel({0, 0, 200, 320}, v);
}

KIMIA_TEST(Layer_DrawAtTabletLandscape) {
  std::vector<kimia::ui::LayerEntry> v;
  for (int i = 0; i < 12; ++i) {
    kimia::ui::LayerEntry l;
    l.name = "Layer_" + std::to_string(i);
    v.push_back(l);
  }
  kimia::ui::drawLayerPanel({0, 0, 280, 280}, v);
}

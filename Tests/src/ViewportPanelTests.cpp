#include <kimia_test.h>
#include <kimia/ViewportPanel.h>

KIMIA_TEST(Viewport_DrawPerspective) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::ViewportMode::Perspective, "Scene");
}

KIMIA_TEST(Viewport_DrawTop) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::ViewportMode::Top, "Top View");
}

KIMIA_TEST(Viewport_DrawFront) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::ViewportMode::Front, "");
}

KIMIA_TEST(Viewport_DrawSide) {
  kimia::ui::drawViewportPanel({0, 0, 320, 240},
                               kimia::ui::ViewportMode::Side, "Side");
}

KIMIA_TEST(Viewport_DrawAtPhonePortrait) {
  kimia::ui::drawViewportPanel({0, 0, 240, 320},
                               kimia::ui::ViewportMode::Perspective, "M");
}

KIMIA_TEST(Viewport_DrawAtTabletLandscape) {
  kimia::ui::drawViewportPanel({0, 0, 800, 400},
                               kimia::ui::ViewportMode::Top, "Map");
}

KIMIA_TEST(Viewport_DrawTiny) {
  kimia::ui::drawViewportPanel({0, 0, 80, 60},
                               kimia::ui::ViewportMode::Perspective, "");
}

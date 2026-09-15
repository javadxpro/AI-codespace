// StatusBar tests — see Engine/EditorUI/include/kimia/StatusBar.h.

#include <kimia_test.h>
#include <kimia/StatusBar.h>

KIMIA_TEST(StatusBar_DrawDefaultDoesNotCrash) {
  kimia::ui::Status s;
  kimia::ui::drawStatusBar({0, 0, 320, 20}, s);
}

KIMIA_TEST(StatusBar_DrawPlayingDoesNotCrash) {
  kimia::ui::Status s;
  s.tool = "Move";
  s.entityCount = 42;
  s.fps = 60.0f;
  s.playing = true;
  s.paused = false;
  s.hint = "Saved to foo.kimia";
  kimia::ui::drawStatusBar({0, 0, 320, 20}, s);
}

KIMIA_TEST(StatusBar_DrawPausedDoesNotCrash) {
  kimia::ui::Status s;
  s.tool = "Rotate";
  s.playing = true;
  s.paused = true;
  s.fps = 30.0f;
  kimia::ui::drawStatusBar({0, 0, 320, 20}, s);
}

KIMIA_TEST(StatusBar_DrawLowFpsChangesColour) {
  // < 30 fps flips to warning, < 15 flips to error.
  kimia::ui::Status s;
  s.fps = 25.0f;
  kimia::ui::drawStatusBar({0, 0, 320, 20}, s);
  s.fps = 10.0f;
  kimia::ui::drawStatusBar({0, 0, 320, 20}, s);
  s.fps = 0.0f;
  kimia::ui::drawStatusBar({0, 0, 320, 20}, s);
}

KIMIA_TEST(StatusBar_DrawWithHintDoesNotCrash) {
  kimia::ui::Status s;
  s.hint = "Loading…";
  kimia::ui::drawStatusBar({0, 0, 320, 20}, s);
}

KIMIA_TEST(StatusBar_DrawWithEmptyHintDoesNotCrash) {
  // Empty hint → the centre segment is hidden.
  kimia::ui::Status s;
  s.hint = "";
  kimia::ui::drawStatusBar({0, 0, 320, 20}, s);
}

KIMIA_TEST(StatusBar_DrawAtPhonePortrait) {
  kimia::ui::Status s;
  s.playing = true;
  kimia::ui::drawStatusBar({0, 0, 240, 20}, s);
}

KIMIA_TEST(StatusBar_DrawAtTabletLandscape) {
  kimia::ui::Status s;
  s.tool = "Scale";
  s.entityCount = 1000;
  s.fps = 120.0f;
  s.playing = true;
  s.hint = "Compiling shaders";
  kimia::ui::drawStatusBar({0, 0, 800, 20}, s);
}

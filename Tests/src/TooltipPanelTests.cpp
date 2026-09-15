// TooltipPanel tests — see Engine/EditorUI/include/kimia/TooltipPanel.h.

#include <kimia_test.h>
#include <kimia/TooltipPanel.h>

KIMIA_TEST(TooltipPanel_EmptyTextIsNoOp) {
  // Empty text → no draw. Test passes if it doesn't crash.
  kimia::ui::drawTooltipPanel(10.0f, 10.0f, "");
}

KIMIA_TEST(TooltipPanel_DrawShortDoesNotCrash) {
  kimia::ui::drawTooltipPanel(10.0f, 10.0f, "OK");
  kimia::ui::drawTooltipPanel(0.0f, 0.0f, "Save");
}

KIMIA_TEST(TooltipPanel_DrawLongDoesNotCrash) {
  // A tooltip with a very long text — width grows, but never
  // crashes.
  const std::string s(
      "This is a very long tooltip that would normally extend off "
      "the right edge of the screen if not clipped properly");
  kimia::ui::drawTooltipPanel(100.0f, 100.0f, s);
}

KIMIA_TEST(TooltipPanel_DrawWithSpecialCharacters) {
  kimia::ui::drawTooltipPanel(10.0f, 10.0f, "Ctrl+Z");
  kimia::ui::drawTooltipPanel(10.0f, 10.0f, "Shift+Tap");
  kimia::ui::drawTooltipPanel(10.0f, 10.0f, "Player_1");
}

KIMIA_TEST(TooltipPanel_DrawAtNegativePosition) {
  // Negative positions must not crash (the tooltip will be drawn
  // partly off-screen but the math stays well-defined).
  kimia::ui::drawTooltipPanel(-10.0f, -10.0f, "off-screen");
  kimia::ui::drawTooltipPanel(-100.0f, -50.0f, "way off");
}

KIMIA_TEST(TooltipPanel_DrawAtLargePosition) {
  // Large positions — same as negative, just clipping.
  kimia::ui::drawTooltipPanel(10000.0f, 10000.0f, "way over");
}

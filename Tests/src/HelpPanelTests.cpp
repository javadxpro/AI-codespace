// HelpPanel tests — see Engine/EditorUI/include/kimia/HelpPanel.h.

#include <kimia_test.h>
#include <kimia/HelpPanel.h>

KIMIA_TEST(HelpPanel_KnownShortcutsPresent) {
  // We can't iterate the constexpr array directly without exposing
  // it, so the test pins a few well-known entries by their string
  // contents. The header's kShortcuts is the source of truth — if
  // anyone removes Ctrl+Z this test breaks.
  bool foundUndo = false, foundRedo = false, foundPlay = false;
  for (const auto& s : kimia::ui::kShortcuts) {
    if (std::string(s.keys) == "Ctrl+Z" &&
        std::string(s.description) == "Undo") foundUndo = true;
    if (std::string(s.keys) == "Ctrl+Shift+Z" &&
        std::string(s.description) == "Redo") foundRedo = true;
    if (std::string(s.keys) == "F5" &&
        std::string(s.description) == "Play / Stop") foundPlay = true;
  }
  KIMIA_REQUIRE(foundUndo);
  KIMIA_REQUIRE(foundRedo);
  KIMIA_REQUIRE(foundPlay);
}

KIMIA_TEST(HelpPanel_AllShortcutsHaveNonEmptyFields) {
  for (const auto& s : kimia::ui::kShortcuts) {
    KIMIA_REQUIRE(s.keys != nullptr);
    KIMIA_REQUIRE(std::string(s.keys).size() > 0);
    KIMIA_REQUIRE(s.description != nullptr);
    KIMIA_REQUIRE(std::string(s.description).size() > 0);
  }
}

KIMIA_TEST(HelpPanel_AtLeastTenShortcuts) {
  KIMIA_REQUIRE(sizeof(kimia::ui::kShortcuts) /
                sizeof(kimia::ui::kShortcuts[0]) >= 10);
}

KIMIA_TEST(HelpPanel_DrawDoesNotCrash) {
  kimia::ui::drawHelpPanel({0, 0, 400, 300});
}

KIMIA_TEST(HelpPanel_DrawAtPhonePortrait) {
  kimia::ui::drawHelpPanel({0, 0, 240, 320});
}

KIMIA_TEST(HelpPanel_DrawAtTabletLandscape) {
  kimia::ui::drawHelpPanel({0, 0, 800, 400});
}

KIMIA_TEST(HelpPanel_DrawTinyRectStillDoesNotCrash) {
  // Degenerate rect — should not crash, just clip.
  kimia::ui::drawHelpPanel({0, 0, 10, 10});
}

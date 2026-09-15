// DialogPanel tests — see Engine/EditorUI/include/kimia/DialogPanel.h.

#include <kimia_test.h>
#include <kimia/DialogPanel.h>

KIMIA_TEST(DialogPanel_ClosedDoesNotDraw) {
  // When `open == false`, drawDialogPanel is a no-op. We can't
  // observe that directly, but the test passes if it doesn't crash.
  kimia::ui::DialogState s;
  s.open = false;
  s.title = "Save changes?";
  s.message = "Unsaved changes will be lost.";
  s.buttons.push_back({"OK", 1});
  s.buttons.push_back({"Cancel", 2});
  kimia::ui::drawDialogPanel({0, 0, 320, 240}, s);
}

KIMIA_TEST(DialogPanel_DrawSimpleOkDoesNotCrash) {
  kimia::ui::DialogState s;
  s.open = true;
  s.title = "Info";
  s.message = "Engine started.";
  s.buttons.push_back({"OK", 1});
  kimia::ui::drawDialogPanel({0, 0, 320, 240}, s);
}

KIMIA_TEST(DialogPanel_DrawYesNoCancelDoesNotCrash) {
  kimia::ui::DialogState s;
  s.open = true;
  s.title = "Save changes?";
  s.message = "Do you want to save before closing?";
  s.buttons.push_back({"Yes", 1});
  s.buttons.push_back({"No",  2});
  s.buttons.push_back({"Cancel", 3});
  kimia::ui::drawDialogPanel({0, 0, 320, 240}, s);
}

KIMIA_TEST(DialogPanel_DrawFourButtonsDoesNotCrash) {
  // Up to 4 buttons are supported.
  kimia::ui::DialogState s;
  s.open = true;
  s.title = "Multiple choice";
  s.message = "Pick one:";
  s.buttons.push_back({"A", 1});
  s.buttons.push_back({"B", 2});
  s.buttons.push_back({"C", 3});
  s.buttons.push_back({"D", 4});
  kimia::ui::drawDialogPanel({0, 0, 480, 240}, s);
}

KIMIA_TEST(DialogPanel_DrawAtPhonePortrait) {
  kimia::ui::DialogState s;
  s.open = true;
  s.title = "Info";
  s.message = "Hello";
  s.buttons.push_back({"OK", 1});
  kimia::ui::drawDialogPanel({0, 0, 240, 320}, s);
}

KIMIA_TEST(DialogPanel_DrawAtTabletLandscape) {
  kimia::ui::DialogState s;
  s.open = true;
  s.title = "Confirm";
  s.message = "Are you sure?";
  s.buttons.push_back({"Yes", 1});
  s.buttons.push_back({"No",  2});
  kimia::ui::drawDialogPanel({0, 0, 800, 400}, s);
}

KIMIA_TEST(DialogPanel_DrawWithEmptyMessage) {
  // Empty message must not crash.
  kimia::ui::DialogState s;
  s.open = true;
  s.title = "Note";
  s.message = "";
  s.buttons.push_back({"OK", 1});
  kimia::ui::drawDialogPanel({0, 0, 320, 240}, s);
}

KIMIA_TEST(DialogPanel_DrawWithNoButtons) {
  // A dialog with no buttons — only title + message. Still renders.
  kimia::ui::DialogState s;
  s.open = true;
  s.title = "Loading…";
  s.message = "Please wait.";
  kimia::ui::drawDialogPanel({0, 0, 320, 240}, s);
}

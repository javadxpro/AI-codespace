#include <kimia_test.h>
#include <kimia/AccountLoginPanel.h>

KIMIA_TEST(AccountLogin_DrawEmptyDoesNotCrash) {
  kimia::ui::drawAccountLoginPanel({0, 0, 280, 240},
                                   "", "", false, "");
}

KIMIA_TEST(AccountLogin_DrawWithUsername) {
  kimia::ui::drawAccountLoginPanel({0, 0, 280, 240},
                                   "alice", "", false, "");
}

KIMIA_TEST(AccountLogin_DrawWithPassword) {
  // Password must be masked with stars.
  kimia::ui::drawAccountLoginPanel({0, 0, 280, 240},
                                   "alice", "secret123", true, "");
}

KIMIA_TEST(AccountLogin_DrawWithSuccessStatus) {
  kimia::ui::drawAccountLoginPanel({0, 0, 280, 240},
                                   "alice", "secret123", false, "Signed in");
}

KIMIA_TEST(AccountLogin_DrawWithErrorStatus) {
  kimia::ui::drawAccountLoginPanel({0, 0, 280, 240},
                                   "alice", "wrong", false,
                                   "error: invalid credentials");
}

KIMIA_TEST(AccountLogin_DrawAtPhonePortrait) {
  kimia::ui::drawAccountLoginPanel({0, 0, 240, 320},
                                   "bob", "pass", true, "ok");
}

KIMIA_TEST(AccountLogin_DrawAtTabletLandscape) {
  kimia::ui::drawAccountLoginPanel({0, 0, 480, 320},
                                   "longusername", "verylongpassword", true,
                                   "fail: connection refused");
}

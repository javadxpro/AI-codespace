#include <kimia_test.h>
#include <kimia/ConsolePanel.h>

KIMIA_TEST(Console_DrawEmptyDoesNotCrash) {
  kimia::ui::drawConsolePanel({0, 0, 320, 200}, {}, "", 0, true);
}

KIMIA_TEST(Console_DrawWithLines) {
  std::vector<kimia::ui::ConsoleLine> v;
  v.push_back({"engine ready", 1});
  v.push_back({"warning: shader recompile", 2});
  v.push_back({"error: out of memory", 3});
  v.push_back({"plain line", 0});
  kimia::ui::drawConsolePanel({0, 0, 320, 200}, v, "test", 0, true);
  kimia::ui::drawConsolePanel({0, 0, 320, 200}, v, "test", -50, false);
  kimia::ui::drawConsolePanel({0, 0, 320, 200}, v, "test", 100, false);
}

KIMIA_TEST(Console_DrawWithLongInput) {
  std::vector<kimia::ui::ConsoleLine> v;
  v.push_back({"ready", 1});
  const std::string big(120, 'a');
  kimia::ui::drawConsolePanel({0, 0, 320, 200}, v, big, 0, true);
}

KIMIA_TEST(Console_DrawWithManyLines) {
  std::vector<kimia::ui::ConsoleLine> v;
  for (int i = 0; i < 40; ++i) {
    kimia::ui::ConsoleLine l;
    l.text = "line " + std::to_string(i);
    l.severity = i % 4;
    v.push_back(l);
  }
  kimia::ui::drawConsolePanel({0, 0, 320, 240}, v, "cmd", 0, true);
}

KIMIA_TEST(Console_DrawAtPhonePortrait) {
  std::vector<kimia::ui::ConsoleLine> v;
  v.push_back({"ok", 1});
  kimia::ui::drawConsolePanel({0, 0, 240, 320}, v, "x", 0, true);
}

KIMIA_TEST(Console_DrawAtTabletLandscape) {
  std::vector<kimia::ui::ConsoleLine> v;
  for (int i = 0; i < 5; ++i) {
    v.push_back({"msg " + std::to_string(i), 0});
  }
  kimia::ui::drawConsolePanel({0, 0, 800, 280}, v, "", 0, true);
}

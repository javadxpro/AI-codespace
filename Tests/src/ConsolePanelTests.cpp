#include <kimia_test.h>
#include <kimia/ConsolePanel.h>

KIMIA_TEST(Console_DrawEmptyDoesNotCrash) {
  kimia::ui::drawConsolePanel({0, 0, 360, 240}, {}, "", 0, false);
}

KIMIA_TEST(Console_DrawOneInfo) {
  std::vector<kimia::ui::LogLine> v(1);
  v[0].level = kimia::ui::LogLevel::Info;
  v[0].text = "Hello world";
  kimia::ui::drawConsolePanel({0, 0, 360, 240}, v, "", 0, false);
}

KIMIA_TEST(Console_DrawManyLines) {
  std::vector<kimia::ui::LogLine> v;
  for (int i = 0; i < 15; ++i) {
    kimia::ui::LogLine l;
    l.level = (i % 3 == 0) ? kimia::ui::LogLevel::Error
             : (i % 3 == 1) ? kimia::ui::LogLevel::Warn
             : kimia::ui::LogLevel::Info;
    l.text = "Line " + std::to_string(i) + " of message text here";
    v.push_back(l);
  }
  kimia::ui::drawConsolePanel({0, 0, 360, 240}, v, "help", 0, false);
}

KIMIA_TEST(Console_DrawAtScroll) {
  std::vector<kimia::ui::LogLine> v;
  for (int i = 0; i < 60; ++i) {
    kimia::ui::LogLine l;
    l.level = kimia::ui::LogLevel::Info;
    l.text = "log " + std::to_string(i);
    v.push_back(l);
  }
  kimia::ui::drawConsolePanel({0, 0, 360, 240}, v, "", -100, false);
  kimia::ui::drawConsolePanel({0, 0, 360, 240}, v, "", 100, true);
}

KIMIA_TEST(Console_DrawWithAutoScroll) {
  std::vector<kimia::ui::LogLine> v;
  kimia::ui::LogLine l;
  l.level = kimia::ui::LogLevel::Info;
  l.text = "scrolled to bottom";
  v.push_back(l);
  kimia::ui::drawConsolePanel({0, 0, 360, 240}, v, "", 0, true);
}

KIMIA_TEST(Console_DrawWithLongInput) {
  std::vector<kimia::ui::LogLine> v;
  kimia::ui::LogLine l;
  l.level = kimia::ui::LogLevel::Info;
  l.text = "ready";
  v.push_back(l);
  kimia::ui::drawConsolePanel({0, 0, 360, 240}, v,
    "very long command typed here that might overflow the input box "
    "and should still be drawn without crashing", 0, false);
}

KIMIA_TEST(Console_DrawAtPhonePortrait) {
  std::vector<kimia::ui::LogLine> v;
  for (int i = 0; i < 3; ++i) {
    kimia::ui::LogLine l;
    l.level = (i == 0) ? kimia::ui::LogLevel::Info
             : (i == 1) ? kimia::ui::LogLevel::Warn
             : kimia::ui::LogLevel::Error;
    l.text = (i == 0) ? "ok" : (i == 1) ? "warn" : "fail";
    v.push_back(l);
  }
  kimia::ui::drawConsolePanel({0, 0, 240, 320}, v, "x", 0, false);
}

KIMIA_TEST(Console_DrawAtTabletLandscape) {
  std::vector<kimia::ui::LogLine> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::LogLine l;
    l.level = (i % 3 == 0) ? kimia::ui::LogLevel::Error
             : (i % 3 == 1) ? kimia::ui::LogLevel::Warn
             : kimia::ui::LogLevel::Info;
    l.text = "msg " + std::to_string(i);
    v.push_back(l);
  }
  kimia::ui::drawConsolePanel({0, 0, 480, 320}, v, "command", 0, true);
}

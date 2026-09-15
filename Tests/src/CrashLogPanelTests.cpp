#include <kimia_test.h>
#include <kimia/CrashLogPanel.h>

KIMIA_TEST(CrashLog_DrawEmptyDoesNotCrash) {
  kimia::ui::drawCrashLogPanel({0, 0, 280, 200}, {}, 0);
}

KIMIA_TEST(CrashLog_DrawOneCrash) {
  std::vector<kimia::ui::CrashEntry> v(1);
  v[0].timestamp = "2026-09-15 14:23";
  v[0].thread = "main";
  v[0].message = "SIGSEGV: null pointer deref";
  v[0].stackTrace = "at WorldEditor::update() (World.cpp:432)";
  kimia::ui::drawCrashLogPanel({0, 0, 280, 200}, v, 0);
}

KIMIA_TEST(CrashLog_DrawManyCrashes) {
  std::vector<kimia::ui::CrashEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::CrashEntry c;
    c.timestamp = "2026-09-15 14:23:" + std::to_string(i);
    c.thread = (i % 2 == 0) ? "main" : "render";
    c.message = "Crash " + std::to_string(i);
    c.stackTrace = "at function_" + std::to_string(i) + "()";
    v.push_back(c);
  }
  kimia::ui::drawCrashLogPanel({0, 0, 320, 400}, v, 0);
}

KIMIA_TEST(CrashLog_DrawAtScroll) {
  std::vector<kimia::ui::CrashEntry> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::CrashEntry c;
    c.timestamp = "t" + std::to_string(i);
    c.thread = "th";
    c.message = "msg";
    c.stackTrace = "stack";
    v.push_back(c);
  }
  kimia::ui::drawCrashLogPanel({0, 0, 280, 200}, v, -100);
  kimia::ui::drawCrashLogPanel({0, 0, 280, 200}, v, 100);
}

KIMIA_TEST(CrashLog_DrawWithLongStack) {
  std::vector<kimia::ui::CrashEntry> v(1);
  v[0].timestamp = "now";
  v[0].thread = "main";
  v[0].message = "Error";
  v[0].stackTrace = "at Very::Long::Nested::Call::Chain::Function::Path::That::Goes::On::For::Many::Frames::And::Exceeds::The::Available::Width::Of::The::Panel::So::It::Gets::Truncated::At::The::Right::Place::Without::Crashing::The::Editor::Or::Causing::Memory::Issues::At::All::And::This::Is::The::End";
  kimia::ui::drawCrashLogPanel({0, 0, 280, 100}, v, 0);
}

KIMIA_TEST(CrashLog_DrawAtPhonePortrait) {
  std::vector<kimia::ui::CrashEntry> v(1);
  v[0].timestamp = "t";
  v[0].thread = "th";
  v[0].message = "msg";
  kimia::ui::drawCrashLogPanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(CrashLog_DrawAtTabletLandscape) {
  std::vector<kimia::ui::CrashEntry> v;
  for (int i = 0; i < 3; ++i) {
    kimia::ui::CrashEntry c;
    c.timestamp = "t" + std::to_string(i);
    c.thread = "th";
    c.message = "m" + std::to_string(i);
    c.stackTrace = "stack " + std::to_string(i);
    v.push_back(c);
  }
  kimia::ui::drawCrashLogPanel({0, 0, 480, 320}, v, 0);
}

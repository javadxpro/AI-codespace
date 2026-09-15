// LogPanel tests — see Engine/EditorUI/include/kimia/LogPanel.h.

#include <kimia_test.h>
#include <kimia/LogPanel.h>

KIMIA_TEST(LogPanel_ClassifyError) {
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("[ERR] OOM") ==
                kimia::ui::LogSeverity::Error);
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("error: nope") ==
                kimia::ui::LogSeverity::Error);
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("ERROR: nope") ==
                kimia::ui::LogSeverity::Error);
}

KIMIA_TEST(LogPanel_ClassifyWarning) {
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("[WARN] slow") ==
                kimia::ui::LogSeverity::Warning);
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("warning: x") ==
                kimia::ui::LogSeverity::Warning);
}

KIMIA_TEST(LogPanel_ClassifyInfo) {
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("[INFO] ready") ==
                kimia::ui::LogSeverity::Info);
}

KIMIA_TEST(LogPanel_ClassifyPlain) {
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("hello world") ==
                kimia::ui::LogSeverity::Plain);
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("") ==
                kimia::ui::LogSeverity::Plain);
  KIMIA_REQUIRE(kimia::ui::classifyLogLine("e") ==
                kimia::ui::LogSeverity::Plain);
}

KIMIA_TEST(LogPanel_DrawEmptyDoesNotCrash) {
  kimia::ui::drawLogPanel({0, 0, 320, 80}, {}, 0, true);
  kimia::ui::drawLogPanel({0, 0, 320, 80}, {}, 0, false);
}

KIMIA_TEST(LogPanel_DrawWithMixedLines) {
  std::vector<std::string> v;
  v.push_back("[INFO] engine ready");
  v.push_back("[WARN] shader recompile");
  v.push_back("[ERR] out of memory");
  v.push_back("plain line without severity");
  v.push_back("");  // empty
  kimia::ui::drawLogPanel({0, 0, 320, 200}, v, 0, true);
  kimia::ui::drawLogPanel({0, 0, 320, 200}, v, -10, false);
  kimia::ui::drawLogPanel({0, 0, 320, 200}, v, 100, false);
}

KIMIA_TEST(LogPanel_DrawAtPhonePortrait) {
  std::vector<std::string> v{
    "[INFO] phone-portrait", "[ERR] something"};
  kimia::ui::drawLogPanel({0, 0, 240, 200}, v, 0, true);
}

KIMIA_TEST(LogPanel_DrawAtTabletLandscape) {
  std::vector<std::string> v{
    "[INFO] tablet", "[WARN] another"};
  kimia::ui::drawLogPanel({0, 0, 800, 200}, v, 0, true);
}

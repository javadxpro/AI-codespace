#include <kimia_test.h>
#include <kimia/ScriptEditorPanel.h>

KIMIA_TEST(ScriptEditor_DrawEmptyDoesNotCrash) {
  kimia::ui::drawScriptEditorPanel({0, 0, 240, 200}, {}, 0, -1, "");
}

KIMIA_TEST(ScriptEditor_DrawOneLine) {
  std::vector<kimia::ui::ScriptLine> v(1);
  v[0].text = "print(\"hello\")";
  v[0].highlight = false;
  kimia::ui::drawScriptEditorPanel({0, 0, 240, 200}, v, 0, 0, "");
}

KIMIA_TEST(ScriptEditor_DrawManyLines) {
  std::vector<kimia::ui::ScriptLine> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::ScriptLine l;
    l.text = "x = " + std::to_string(i) + " * 2;";
    l.highlight = (i % 3 == 0);
    v.push_back(l);
  }
  kimia::ui::drawScriptEditorPanel({0, 0, 280, 280}, v, 0, 5, "edited");
}

KIMIA_TEST(ScriptEditor_DrawAtScroll) {
  std::vector<kimia::ui::ScriptLine> v;
  for (int i = 0; i < 50; ++i) {
    kimia::ui::ScriptLine l;
    l.text = "line_" + std::to_string(i);
    v.push_back(l);
  }
  kimia::ui::drawScriptEditorPanel({0, 0, 280, 200}, v, -50, 10, "");
  kimia::ui::drawScriptEditorPanel({0, 0, 280, 200}, v, 100, 30, "");
}

KIMIA_TEST(ScriptEditor_DrawWithCursorOnDifferentLines) {
  std::vector<kimia::ui::ScriptLine> v;
  for (int i = 0; i < 5; ++i) {
    v.push_back({"line_" + std::to_string(i), false});
  }
  for (int cursor = -1; cursor <= 5; ++cursor) {
    kimia::ui::drawScriptEditorPanel({0, 0, 240, 200}, v, 0, cursor, "");
  }
}

KIMIA_TEST(ScriptEditor_DrawAtPhonePortrait) {
  std::vector<kimia::ui::ScriptLine> v;
  for (int i = 0; i < 10; ++i) {
    v.push_back({"s" + std::to_string(i), false});
  }
  kimia::ui::drawScriptEditorPanel({0, 0, 240, 320}, v, 0, 2, "");
}

KIMIA_TEST(ScriptEditor_DrawAtTabletLandscape) {
  std::vector<kimia::ui::ScriptLine> v;
  for (int i = 0; i < 25; ++i) {
    v.push_back({"var_" + std::to_string(i) + " = 0", i % 2 == 0});
  }
  kimia::ui::drawScriptEditorPanel({0, 0, 480, 320}, v, 0, 10, "main.kimia");
}

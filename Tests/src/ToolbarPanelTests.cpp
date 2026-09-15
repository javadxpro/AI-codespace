#include <kimia_test.h>
#include <kimia/ToolbarPanel.h>

KIMIA_TEST(Toolbar_DrawEmptyDoesNotCrash) {
  kimia::ui::drawToolbarPanel({0, 0, 320, 28}, {});
}

KIMIA_TEST(Toolbar_DrawOneButton) {
  std::vector<kimia::ui::ToolbarButton> v(1);
  v[0].label = "Q";
  v[0].iconGlyph = "S";
  v[0].id = 1;
  kimia::ui::drawToolbarPanel({0, 0, 320, 28}, v);
}

KIMIA_TEST(Toolbar_DrawManyButtons) {
  std::vector<kimia::ui::ToolbarButton> v;
  const char* glyphs[] = {"S", "M", "R", "X", "C", "P", "B", "V"};
  for (int i = 0; i < 8; ++i) {
    kimia::ui::ToolbarButton b;
    b.label = std::string(1, 'A' + i);
    b.iconGlyph = glyphs[i];
    b.id = i;
    b.toggle = (i < 4);
    b.toggledOn = (i == 1);
    v.push_back(b);
  }
  kimia::ui::drawToolbarPanel({0, 0, 400, 28}, v);
}

KIMIA_TEST(Toolbar_DrawOverflow) {
  // More buttons than fit in the rect — overflow must clip cleanly.
  std::vector<kimia::ui::ToolbarButton> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::ToolbarButton b;
    b.label = "B";
    b.id = i;
    v.push_back(b);
  }
  kimia::ui::drawToolbarPanel({0, 0, 200, 28}, v);
}

KIMIA_TEST(Toolbar_DrawAtPhonePortrait) {
  std::vector<kimia::ui::ToolbarButton> v;
  for (int i = 0; i < 4; ++i) {
    kimia::ui::ToolbarButton b;
    b.label = "t";
    b.id = i;
    b.toggle = true;
    b.toggledOn = (i == 0);
    v.push_back(b);
  }
  kimia::ui::drawToolbarPanel({0, 0, 240, 28}, v);
}

KIMIA_TEST(Toolbar_DrawAtTabletLandscape) {
  std::vector<kimia::ui::ToolbarButton> v;
  for (int i = 0; i < 16; ++i) {
    kimia::ui::ToolbarButton b;
    b.label = std::to_string(i);
    v.push_back(b);
  }
  kimia::ui::drawToolbarPanel({0, 0, 800, 28}, v);
}

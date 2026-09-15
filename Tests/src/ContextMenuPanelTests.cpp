// ContextMenuPanel tests — see Engine/EditorUI/include/kimia/ContextMenuPanel.h.

#include <kimia_test.h>
#include <kimia/ContextMenuPanel.h>

KIMIA_TEST(ContextMenu_ClosedIsNoOp) {
  // Closed menu — no draw, just no-op.
  kimia::ui::ContextMenuState s;
  s.open = false;
  s.items.push_back({"Delete", 1});
  kimia::ui::drawContextMenu(10.0f, 10.0f, s);
}

KIMIA_TEST(ContextMenu_DrawEmptyDoesNotCrash) {
  kimia::ui::ContextMenuState s;
  s.open = true;
  kimia::ui::drawContextMenu(10.0f, 10.0f, s);
}

KIMIA_TEST(ContextMenu_DrawOneItem) {
  kimia::ui::ContextMenuState s;
  s.open = true;
  s.items.push_back({"Delete", 1});
  kimia::ui::drawContextMenu(10.0f, 10.0f, s);
}

KIMIA_TEST(ContextMenu_DrawWithSeparator) {
  kimia::ui::ContextMenuState s;
  s.open = true;
  s.items.push_back({"Rename", 1});
  s.items.push_back({"", 0, /*separator=*/true});
  s.items.push_back({"Delete", 2});
  kimia::ui::drawContextMenu(10.0f, 10.0f, s);
}

KIMIA_TEST(ContextMenu_DrawWithDisabledItem) {
  kimia::ui::ContextMenuState s;
  s.open = true;
  s.items.push_back({"Cut", 1, false, /*disabled=*/true});
  s.items.push_back({"Copy", 2});
  s.items.push_back({"Paste", 3, false, /*disabled=*/true});
  kimia::ui::drawContextMenu(10.0f, 10.0f, s);
}

KIMIA_TEST(ContextMenu_DrawLongItem) {
  // A very long label — exercises the menu-width math.
  kimia::ui::ContextMenuState s;
  s.open = true;
  s.items.push_back(
      {"A_Very_Long_Item_Label_That_Should_Make_The_Menu_Wider_Without_Crashing",
       1});
  kimia::ui::drawContextMenu(10.0f, 10.0f, s);
}

KIMIA_TEST(ContextMenu_DrawManyItems) {
  kimia::ui::ContextMenuState s;
  s.open = true;
  for (int i = 0; i < 10; ++i) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "item_%d", i);
    s.items.push_back({buf, i});
    if (i % 3 == 0) {
      s.items.push_back({"", 0, true});
    }
  }
  kimia::ui::drawContextMenu(10.0f, 10.0f, s);
}

KIMIA_TEST(ContextMenu_DrawAtNegativePosition) {
  // Negative positions must not crash.
  kimia::ui::ContextMenuState s;
  s.open = true;
  s.items.push_back({"Delete", 1});
  kimia::ui::drawContextMenu(-10.0f, -10.0f, s);
}

KIMIA_TEST(ContextMenu_DrawAtZeroPosition) {
  // Top-left corner.
  kimia::ui::ContextMenuState s;
  s.open = true;
  s.items.push_back({"Delete", 1});
  kimia::ui::drawContextMenu(0.0f, 0.0f, s);
}

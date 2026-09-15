#include <kimia_test.h>
#include <kimia/HistoryPanel.h>

KIMIA_TEST(History_DrawEmptyDoesNotCrash) {
  kimia::ui::drawHistoryPanel({0, 0, 240, 200}, {}, 0);
}

KIMIA_TEST(History_DrawOneEntry) {
  std::vector<kimia::ui::HistoryEntry> v(1);
  v[0].description = "Moved Cube_1";
  v[0].timestamp = 1700000000;
  v[0].undone = false;
  kimia::ui::drawHistoryPanel({0, 0, 240, 200}, v, 0);
}

KIMIA_TEST(History_DrawManyEntries) {
  std::vector<kimia::ui::HistoryEntry> v;
  const char* descs[] = {
    "Create Cube", "Move Cube", "Set Color", "Add Light",
    "Move Light", "Delete Box", "Undo", "Redo", "Save",
  };
  for (int i = 0; i < 9; ++i) {
    kimia::ui::HistoryEntry e;
    e.description = descs[i];
    e.timestamp = 1700000000 + i * 60;
    e.undone = (i >= 6);
    v.push_back(e);
  }
  kimia::ui::drawHistoryPanel({0, 0, 280, 280}, v, 0);
}

KIMIA_TEST(History_DrawAtScroll) {
  std::vector<kimia::ui::HistoryEntry> v;
  for (int i = 0; i < 40; ++i) {
    kimia::ui::HistoryEntry e;
    e.description = "op_" + std::to_string(i);
    e.undone = (i > 20);
    v.push_back(e);
  }
  kimia::ui::drawHistoryPanel({0, 0, 240, 200}, v, -50);
  kimia::ui::drawHistoryPanel({0, 0, 240, 200}, v, 100);
}

KIMIA_TEST(History_DrawAtPhonePortrait) {
  std::vector<kimia::ui::HistoryEntry> v;
  for (int i = 0; i < 8; ++i) {
    kimia::ui::HistoryEntry e;
    e.description = "act_" + std::to_string(i);
    v.push_back(e);
  }
  kimia::ui::drawHistoryPanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(History_DrawAtTabletLandscape) {
  std::vector<kimia::ui::HistoryEntry> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::HistoryEntry e;
    e.description = "history_" + std::to_string(i);
    e.timestamp = 1700000000 + i;
    e.undone = (i % 3 == 0);
    v.push_back(e);
  }
  kimia::ui::drawHistoryPanel({0, 0, 480, 320}, v, 0);
}

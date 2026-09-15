#include <kimia_test.h>
#include <kimia/CommandPalettePanel.h>

KIMIA_TEST(CommandPalette_DrawEmptyDoesNotCrash) {
  kimia::ui::drawCommandPalettePanel({0, 0, 360, 240}, "", {}, -1);
}

KIMIA_TEST(CommandPalette_DrawWithCommands) {
  std::vector<kimia::ui::PaletteCommand> cmds;
  cmds.push_back({"Save Scene",      "Ctrl+S", "File"});
  cmds.push_back({"Open Scene",      "Ctrl+O", "File"});
  cmds.push_back({"New Scene",       "Ctrl+N", "File"});
  cmds.push_back({"Undo",            "Ctrl+Z", "Edit"});
  cmds.push_back({"Redo",            "Ctrl+Y", "Edit"});
  cmds.push_back({"Cut",             "Ctrl+X", "Edit"});
  cmds.push_back({"Copy",            "Ctrl+C", "Edit"});
  cmds.push_back({"Paste",           "Ctrl+V", "Edit"});
  cmds.push_back({"Duplicate",       "Ctrl+D", "Edit"});
  cmds.push_back({"Toggle Inspector","F8",     "Window"});
  cmds.push_back({"Toggle Console",  "F12",    "Window"});
  cmds.push_back({"Move Tool",       "W",      "Tools"});
  cmds.push_back({"Rotate Tool",     "E",      "Tools"});
  cmds.push_back({"Scale Tool",      "R",      "Tools"});
  kimia::ui::drawCommandPalettePanel({0, 0, 360, 280}, "", cmds, 0);
}

KIMIA_TEST(CommandPalette_DrawWithQuery) {
  std::vector<kimia::ui::PaletteCommand> cmds;
  cmds.push_back({"Save Scene", "Ctrl+S", "File"});
  cmds.push_back({"Scale Tool", "R",      "Tools"});
  cmds.push_back({"Play Scene", "F5",     "File"});
  cmds.push_back({"Open Scene", "Ctrl+O", "File"});
  cmds.push_back({"Move Tool",  "W",      "Tools"});
  kimia::ui::drawCommandPalettePanel({0, 0, 360, 280}, "save", cmds, 0);
  kimia::ui::drawCommandPalettePanel({0, 0, 360, 280}, "tool", cmds, 1);
  kimia::ui::drawCommandPalettePanel({0, 0, 360, 280}, "noMatch", cmds, -1);
}

KIMIA_TEST(CommandPalette_DrawWithSelectedIndexOutOfRange) {
  std::vector<kimia::ui::PaletteCommand> cmds;
  cmds.push_back({"A", "", ""});
  cmds.push_back({"B", "", ""});
  // Selected far above visible range.
  kimia::ui::drawCommandPalettePanel({0, 0, 360, 100}, "", cmds, 999);
  // Selected negative.
  kimia::ui::drawCommandPalettePanel({0, 0, 360, 100}, "", cmds, -5);
}

KIMIA_TEST(CommandPalette_DrawWithManyCommands) {
  std::vector<kimia::ui::PaletteCommand> cmds;
  for (int i = 0; i < 100; ++i) {
    cmds.push_back({"Cmd" + std::to_string(i),
                    "Ctrl+" + std::to_string(i),
                    "Cat" + std::to_string(i % 5)});
  }
  kimia::ui::drawCommandPalettePanel({0, 0, 360, 240}, "cmd5", cmds, 0);
}

KIMIA_TEST(CommandPalette_DrawAtPhonePortrait) {
  std::vector<kimia::ui::PaletteCommand> cmds;
  cmds.push_back({"A", "", ""});
  cmds.push_back({"B", "", ""});
  cmds.push_back({"C", "", ""});
  kimia::ui::drawCommandPalettePanel({0, 0, 240, 320}, "", cmds, 0);
}

KIMIA_TEST(CommandPalette_DrawAtTabletLandscape) {
  std::vector<kimia::ui::PaletteCommand> cmds;
  for (int i = 0; i < 30; ++i) {
    cmds.push_back({"Cmd" + std::to_string(i), "",
                    "Cat" + std::to_string(i % 4)});
  }
  kimia::ui::drawCommandPalettePanel({0, 0, 480, 320}, "cm", cmds, 0);
}

// UndoStack tests — see Engine/EditorUI/include/kimia/UndoStack.h.

#include <kimia_test.h>
#include <kimia/UndoStack.h>

namespace {

std::vector<kimia::ui::UiCommand> makeCmds(int n) {
  std::vector<kimia::ui::UiCommand> out;
  for (int i = 0; i < n; ++i) {
    kimia::ui::UiCommand c;
    c.kind = kimia::ui::UiCommandKind::SetPosition;
    c.name = "entity_" + std::to_string(i);
    c.x = static_cast<kimia::f32>(i);
    c.y = static_cast<kimia::f32>(i * 2);
    c.z = static_cast<kimia::f32>(i * 3);
    out.push_back(std::move(c));
  }
  return out;
}

}  // namespace

KIMIA_TEST(UndoStack_EmptyByDefault) {
  kimia::ui::UndoStack s;
  KIMIA_REQUIRE(s.undoDepth() == 0);
  KIMIA_REQUIRE(s.redoDepth() == 0);
}

KIMIA_TEST(UndoStack_PushAddsUndoDepth) {
  kimia::ui::UndoStack s;
  s.push(makeCmds(1));
  KIMIA_REQUIRE(s.undoDepth() == 1);
  KIMIA_REQUIRE(s.redoDepth() == 0);
}

KIMIA_TEST(UndoStack_UndoMovesToRedo) {
  kimia::ui::UndoStack s;
  s.push(makeCmds(1));
  std::vector<kimia::ui::UiCommand> out;
  KIMIA_REQUIRE(s.undo(out));
  KIMIA_REQUIRE(!out.empty());
  KIMIA_REQUIRE(s.undoDepth() == 0);
  KIMIA_REQUIRE(s.redoDepth() == 1);
}

KIMIA_TEST(UndoStack_RedoMovesToUndo) {
  kimia::ui::UndoStack s;
  s.push(makeCmds(1));
  std::vector<kimia::ui::UiCommand> tmp;
  s.undo(tmp);
  KIMIA_REQUIRE(s.redo(tmp));
  KIMIA_REQUIRE(s.undoDepth() == 1);
  KIMIA_REQUIRE(s.redoDepth() == 0);
}

KIMIA_TEST(UndoStack_PushAfterUndoClearsRedo) {
  kimia::ui::UndoStack s;
  s.push(makeCmds(1));
  std::vector<kimia::ui::UiCommand> tmp;
  s.undo(tmp);
  KIMIA_REQUIRE(s.redoDepth() == 1);
  s.push(makeCmds(2));
  KIMIA_REQUIRE(s.redoDepth() == 0);
  KIMIA_REQUIRE(s.undoDepth() == 1);  // only the new push, not the old one
}

KIMIA_TEST(UndoStack_EmptyUndoReturnsFalse) {
  kimia::ui::UndoStack s;
  std::vector<kimia::ui::UiCommand> tmp;
  KIMIA_REQUIRE(!s.undo(tmp));
  KIMIA_REQUIRE(!s.redo(tmp));
}

KIMIA_TEST(UndoStack_ClearDropsBoth) {
  kimia::ui::UndoStack s;
  s.push(makeCmds(1));
  s.push(makeCmds(2));
  std::vector<kimia::ui::UiCommand> tmp;
  s.undo(tmp);
  s.clear();
  KIMIA_REQUIRE(s.undoDepth() == 0);
  KIMIA_REQUIRE(s.redoDepth() == 0);
}

KIMIA_TEST(UndoStack_EmptyPushIsNoOp) {
  kimia::ui::UndoStack s;
  s.push({});
  KIMIA_REQUIRE(s.undoDepth() == 0);
}

KIMIA_TEST(UndoStack_MultiplePushesAllUndone) {
  kimia::ui::UndoStack s;
  s.push(makeCmds(1));
  s.push(makeCmds(1));
  s.push(makeCmds(1));
  KIMIA_REQUIRE(s.undoDepth() == 3);
  std::vector<kimia::ui::UiCommand> tmp;
  for (int i = 0; i < 3; ++i) KIMIA_REQUIRE(s.undo(tmp));
  KIMIA_REQUIRE(s.undoDepth() == 0);
  KIMIA_REQUIRE(s.redoDepth() == 3);
}

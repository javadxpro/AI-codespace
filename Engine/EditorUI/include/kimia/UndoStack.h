// UndoStack — a generic command-pattern undo/redo buffer for the editor.
//
// Phase 4+ placeholder: a fixed-capacity ring of UiCommand snapshots
// that the editor pushes onto whenever the user mutates state
// (drag-gizmo, set-property, add-entity, etc). The actual command
// *types* (MoveEntity, SetProperty, DeleteEntity) live with the
// WorldEditor and apply them via applyCommands(). This stack only
// remembers the last 256 commands and provides the standard
// Ctrl+Z / Ctrl+Shift+Z (or undo-tap / redo-tap) interface.
//
// Phase 5+ will switch the snapshots for delta-compressed diffs so
// the ring stays light even with thousands of entities.
#pragma once

#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

class UndoStack {
public:
  // The maximum number of commands we keep in memory. Anything older
  // is dropped silently (this matches what the user expects from
  // mobile editors — no "save 50 undo steps" dialog).
  static constexpr usize kCapacity = 256;

  UndoStack();

  // Push a new command onto the undo stack. The redo stack is cleared
  // (any redo path becomes invalid the moment the user does a new
  // mutation, that's the standard contract).
  void push(std::vector<UiCommand> cmds);

  // Apply and pop the top command. Returns false if the undo stack
  // is empty.
  bool undo(std::vector<UiCommand>& outCmds);

  // Apply and pop the top redo command. Returns false if the redo
  // stack is empty.
  bool redo(std::vector<UiCommand>& outCmds);

  // How many commands are pending undo / redo.
  usize undoDepth() const { return undo_.size(); }
  usize redoDepth() const { return redo_.size(); }

  // Drop everything. Used when the user opens a new world.
  void clear();

private:
  std::vector<std::vector<UiCommand>> undo_;
  std::vector<std::vector<UiCommand>> redo_;
};

}  // namespace kimia::ui

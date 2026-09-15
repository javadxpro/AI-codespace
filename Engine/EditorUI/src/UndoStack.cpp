// UndoStack implementation — see UndoStack.h.
#include <kimia/UndoStack.h>

namespace kimia::ui {

UndoStack::UndoStack() {
  undo_.reserve(kCapacity);
  redo_.reserve(kCapacity);
}

void UndoStack::push(std::vector<UiCommand> cmds) {
  if (cmds.empty()) return;
  undo_.push_back(std::move(cmds));
  if (undo_.size() > kCapacity) {
    undo_.erase(undo_.begin());
  }
  // Any new mutation invalidates the redo path — that's the contract
  // every user expects from desktop and mobile editors.
  redo_.clear();
}

bool UndoStack::undo(std::vector<UiCommand>& outCmds) {
  if (undo_.empty()) return false;
  outCmds = std::move(undo_.back());
  undo_.pop_back();
  redo_.push_back(outCmds);  // copy: outCmds is what the caller will apply
  if (redo_.size() > kCapacity) {
    redo_.erase(redo_.begin());
  }
  return true;
}

bool UndoStack::redo(std::vector<UiCommand>& outCmds) {
  if (redo_.empty()) return false;
  outCmds = std::move(redo_.back());
  redo_.pop_back();
  undo_.push_back(outCmds);
  if (undo_.size() > kCapacity) {
    undo_.erase(undo_.begin());
  }
  return true;
}

void UndoStack::clear() {
  undo_.clear();
  redo_.clear();
}

}  // namespace kimia::ui

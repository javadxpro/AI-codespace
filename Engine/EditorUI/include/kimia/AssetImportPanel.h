#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

enum class ImportStatus {
  Pending,
  Importing,
  Done,
  Failed,
};

struct ImportEntry {
  std::string sourcePath;
  std::string destName;
  ImportStatus status = ImportStatus::Pending;
  f32 progress = 0.0f;
  std::string error;
};

void drawAssetImportPanel(const Rect& rect,
                          const std::vector<ImportEntry>& entries);

}

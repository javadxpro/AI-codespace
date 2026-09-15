#include <kimia/AssetImportPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <algorithm>

namespace kimia::ui {

namespace {
const char* statusName(ImportStatus s) {
  switch (s) {
    case ImportStatus::Pending:   return "...";
    case ImportStatus::Importing: return "...";
    case ImportStatus::Done:      return "OK";
    case ImportStatus::Failed:    return "ERR";
  }
  return "?";
}
Color statusColor(ImportStatus s) {
  using namespace theme;
  switch (s) {
    case ImportStatus::Pending:   return kTextMuted;
    case ImportStatus::Importing: return kAccent;
    case ImportStatus::Done:      return kSuccess;
    case ImportStatus::Failed:    return kError;
  }
  return kText;
}
}

void drawAssetImportPanel(const Rect& rect,
                          const std::vector<ImportEntry>& entries) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Importing", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 22.0f;
  const f32 startY = rect.y + 18.0f;
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH > rect.y + rect.h) break;

    drawText(entries[i].destName.c_str(),
             rect.x + 4.0f, y + 4.0f, 1, kText);
    drawText(statusName(entries[i].status),
             rect.x + rect.w - 36.0f, y + 4.0f,
             1, statusColor(entries[i].status));

    if (entries[i].status == ImportStatus::Importing) {
      const f32 prog = std::max(0.0f, std::min(1.0f, entries[i].progress));
      const f32 barW = (rect.w - 8.0f) * prog;
      drawRect({rect.x + 4.0f, y + rowH - 4.0f, barW, 2.0f},
               kAccent, 0.0f);
    }
    if (entries[i].status == ImportStatus::Failed &&
        !entries[i].error.empty()) {
      drawText(entries[i].error.c_str(),
               rect.x + 4.0f, y + rowH - 4.0f, 1, kError);
    }
  }
  popClip();
}

}

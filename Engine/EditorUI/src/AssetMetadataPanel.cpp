#include <kimia/AssetMetadataPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 80.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
}
void formatSize(char* buf, usize n, u64 bytes) {
  if (bytes < 1024ULL) {
    std::snprintf(buf, n, "%llu B", (unsigned long long)bytes);
  } else if (bytes < 1024ULL * 1024) {
    std::snprintf(buf, n, "%.1f KB", bytes / 1024.0);
  } else if (bytes < 1024ULL * 1024 * 1024) {
    std::snprintf(buf, n, "%.1f MB", bytes / (1024.0 * 1024.0));
  } else {
    std::snprintf(buf, n, "%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
  }
}
void formatDate(char* buf, usize n, i64 sec) {
  std::snprintf(buf, n, "%lld", (long long)sec);
}
}

void drawAssetMetadataPanel(const Rect& rect, const AssetMetadata& md) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Metadata", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[64];

  drawRow({rect.x, y, rect.w, rowH}, "Name", md.name.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Type", md.type.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Path", md.path.c_str());
  y += rowH;
  formatSize(buf, sizeof(buf), md.sizeBytes);
  drawRow({rect.x, y, rect.w, rowH}, "Size", buf);
  y += rowH;
  formatDate(buf, sizeof(buf), md.modifiedSec);
  drawRow({rect.x, y, rect.w, rowH}, "Modified", buf);
  y += rowH;
  formatDate(buf, sizeof(buf), md.createdSec);
  drawRow({rect.x, y, rect.w, rowH}, "Created", buf);
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Author", md.author.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "License", md.license.c_str());
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d", md.refCount);
  drawRow({rect.x, y, rect.w, rowH}, "Ref count", buf);
}

}

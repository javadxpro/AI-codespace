#include <kimia/MemoryPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
void formatBytes(char* buf, usize n, u64 bytes) {
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
}

void drawMemoryPanel(const Rect& rect,
                     const std::vector<MemoryBucket>& buckets) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Memory", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Compute total + max for relative bar scaling.
  u64 total = 0;
  u64 maxBucket = 0;
  for (const auto& b : buckets) {
    total += b.bytes;
    if (b.bytes > maxBucket) maxBucket = b.bytes;
  }

  constexpr f32 rowH = 22.0f;
  f32 y = rect.y + 18.0f;
  char buf[32];

  for (std::size_t i = 0; i < buckets.size(); ++i) {
    if (y + rowH > rect.y + rect.h) break;

    drawText(buckets[i].name.c_str(),
             rect.x + 4.0f, y + 4.0f, 1, kText);
    formatBytes(buf, sizeof(buf), buckets[i].bytes);
    drawText(buf, rect.x + rect.w - 80.0f, y + 4.0f, 1, kTextMuted);
    std::snprintf(buf, sizeof(buf), "x%d", buckets[i].allocCount);
    drawText(buf, rect.x + rect.w - 24.0f, y + 4.0f, 1, kAccent);

    // Bar.
    const f32 barX = rect.x + 4.0f;
    const f32 barW = rect.w - 8.0f;
    const f32 ratio = maxBucket > 0
        ? static_cast<f32>(buckets[i].bytes) / static_cast<f32>(maxBucket)
        : 0.0f;
    drawRect({barX, y + rowH - 4.0f, barW, 2.0f},
             kPanelAlt, 0.0f);
    drawRect({barX, y + rowH - 4.0f, barW * ratio, 2.0f},
             kAccent, 0.0f);

    y += rowH;
  }

  // Total at the bottom.
  formatBytes(buf, sizeof(buf), total);
  drawText("Total:", rect.x + 4.0f, rect.y + rect.h - 14.0f,
           1, kTextMuted);
  drawText(buf,
           rect.x + rect.w - 80.0f, rect.y + rect.h - 14.0f,
           1, kAccentHot);
}

}

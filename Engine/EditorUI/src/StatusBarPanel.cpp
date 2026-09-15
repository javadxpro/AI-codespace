#include <kimia/StatusBarPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
Color fpsColor(f32 fps) {
  using namespace theme;
  if (fps >= 55.0f) return kSuccess;
  if (fps >= 30.0f) return kWarning;
  return kError;
}
}

void drawStatusBarPanel(const Rect& rect, const StatusInfo& info) {
  using namespace theme;
  drawRect(rect, kPanelAlt, 0.0f);
  drawRect({rect.x, rect.y, rect.w, 1.0f}, kAccent, 0.0f);

  const f32 mid = rect.y + rect.h * 0.5f - 6.0f;

  // Left: scene name.
  drawText(info.sceneName.c_str(), rect.x + 6.0f, mid, 1, kText);
  drawText(info.version.c_str(),
           rect.x + 6.0f + 120.0f, mid, 1, kTextMuted);

  // Center: FPS.
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.0f fps", static_cast<double>(info.fps));
  drawText(buf,
           rect.x + rect.w * 0.5f - 20.0f, mid, 1, fpsColor(info.fps));

  // Right: tri count, branch, build config.
  f32 rightX = rect.x + rect.w - 6.0f;
  drawText(info.buildConfig.c_str(),
           rightX, mid, 1, kTextMuted);
  // Walk left for branch + tris.
  // Approximate widths via substring instead of measuring.
  // (Label widths are fixed for simplicity.)
  std::snprintf(buf, sizeof(buf), "%llu tri",
                static_cast<unsigned long long>(info.triCount));
  drawText(buf,
           rect.x + rect.w - 100.0f, mid, 1, kAccent);
  drawText(info.branch.c_str(),
           rect.x + rect.w - 180.0f, mid, 1, kText);
}

}

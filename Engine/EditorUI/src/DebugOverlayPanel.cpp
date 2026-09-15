#include <kimia/DebugOverlayPanel.h>
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

void drawDebugOverlayPanel(const Rect& rect, const DebugReadout& r) {
  using namespace theme;
  // Semi-transparent background.
  drawRect(rect, Color{0.0f, 0.0f, 0.0f, 0.6f}, 0.0f);
  drawRect({rect.x, rect.y, rect.w, 1.0f}, kAccent, 0.0f);

  const f32 pad = 4.0f;
  const f32 lineH = 14.0f;
  f32 y = rect.y + pad;

  char buf[64];

  std::snprintf(buf, sizeof(buf), "FPS %.0f  (%.1f ms)",
                static_cast<double>(r.fps),
                static_cast<double>(r.frameMs));
  drawText(buf, rect.x + pad, y, 1, fpsColor(r.fps));
  y += lineH;

  std::snprintf(buf, sizeof(buf), "Mem %.0f MB", static_cast<double>(r.memMb));
  drawText(buf, rect.x + pad, y, 1, kText);
  y += lineH;

  std::snprintf(buf, sizeof(buf), "Draws %u   Verts %u",
                static_cast<unsigned>(r.drawCalls),
                static_cast<unsigned>(r.verts));
  drawText(buf, rect.x + pad, y, 1, kAccent);
  y += lineH;

  if (!r.scene.empty()) {
    std::snprintf(buf, sizeof(buf), "Scene: %s", r.scene.c_str());
    drawText(buf, rect.x + pad, y, 1, kText);
    y += lineH;
  }
  if (!r.camera.empty()) {
    std::snprintf(buf, sizeof(buf), "Camera: %s", r.camera.c_str());
    drawText(buf, rect.x + pad, y, 1, kText);
    y += lineH;
  }
}

}

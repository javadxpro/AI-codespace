#include <kimia/SplitterPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawSplitterPanel(const Rect& rect, SplitterAxis axis) {
  using namespace theme;
  drawRect(rect, kSplitter, 0.0f);

  // Tiny grip indicator in the centre so the user can find the
  // splitter at a glance.
  if (axis == SplitterAxis::Vertical) {
    const f32 cx = rect.x + rect.w * 0.5f;
    const f32 cy = rect.y + rect.h * 0.5f;
    drawRect({cx - 0.5f, cy - 8.0f, 1.0f, 16.0f}, kBorder, 0.0f);
  } else {
    const f32 cx = rect.x + rect.w * 0.5f;
    const f32 cy = rect.y + rect.h * 0.5f;
    drawRect({cx - 8.0f, cy - 0.5f, 16.0f, 1.0f}, kBorder, 0.0f);
  }
}

}

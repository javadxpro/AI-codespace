#include <kimia/LayerPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

void drawLayerPanel(const Rect& rect,
                    const std::vector<LayerEntry>& layers) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Layers", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  for (std::size_t i = 0; i < layers.size(); ++i) {
    if (y + rowH > rect.y + rect.h) break;

    // Visibility / lock toggles on the left.
    drawText(layers[i].visible ? "V" : " ",
             rect.x + 4.0f, y + 4.0f, 1,
             layers[i].visible ? kSuccess : kTextDim);
    drawText(layers[i].locked ? "L" : " ",
             rect.x + 18.0f, y + 4.0f, 1,
             layers[i].locked ? kWarning : kTextDim);

    // Name (dimmed if hidden or locked).
    Color nameColor = kText;
    if (!layers[i].visible) nameColor = kTextDim;
    if (layers[i].locked)   nameColor = kTextMuted;
    drawText(layers[i].name.c_str(),
             rect.x + 36.0f, y + 4.0f, 1, nameColor);
    y += rowH;
  }
}

}

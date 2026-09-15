#include <kimia/ColorPickerPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawColorPickerPanel(const Rect& rect, const Vec3& rgb,
                          Vec3& editedRgb) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Color Picker", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 swatchSize = 48.0f;
  const f32 sx = rect.x + 8.0f;
  const f32 sy = rect.y + 18.0f;

  // Live preview swatch.
  drawRect({sx, sy, swatchSize, swatchSize},
           {static_cast<float>(rgb.x),
            static_cast<float>(rgb.y),
            static_cast<float>(rgb.z), 1.0f}, 2.0f);

  // Hex readout on the right.
  char buf[16];
  std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
                (int)(rgb.x*255.0f),
                (int)(rgb.y*255.0f),
                (int)(rgb.z*255.0f));
  drawText(buf, sx + swatchSize + 12.0f, sy + 4.0f, 1, kText);

  // 3 sliders: R / G / B.
  const f32 sliderX = sx + swatchSize + 12.0f;
  const f32 sliderW = rect.x + rect.w - sliderX - 8.0f;
  constexpr f32 sliderH = 8.0f;
  f32 ry = sy + 22.0f;

  auto drawSlider = [&](const char* label, float value, Color tint) {
    drawText(label, sliderX, ry + 1.0f, 1, kText);
    drawRect({sliderX + 14.0f, ry, sliderW - 14.0f, sliderH},
             kPanelAlt, 2.0f);
    const f32 fw = (sliderW - 14.0f) * std::max(0.0f, std::min(1.0f, value));
    drawRect({sliderX + 14.0f, ry, fw, sliderH}, tint, 2.0f);
    ry += sliderH + 4.0f;
  };
  drawSlider("R", rgb.x, {1.0f, 0.3f, 0.3f, 1.0f});
  drawSlider("G", rgb.y, {0.3f, 1.0f, 0.3f, 1.0f});
  drawSlider("B", rgb.z, {0.3f, 0.3f, 1.0f, 1.0f});

  // Quick swatches row.
  static const Color quick[] = {
    {1.0f, 1.0f, 1.0f, 1.0f},
    {0.5f, 0.5f, 0.5f, 1.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 1.0f, 1.0f},
  };
  f32 qx = rect.x + 8.0f;
  const f32 qy = rect.y + rect.h - 24.0f;
  for (std::size_t i = 0; i < sizeof(quick) / sizeof(quick[0]); ++i) {
    drawRect({qx, qy, 16.0f, 16.0f}, quick[i], 2.0f);
    qx += 18.0f;
  }

  (void)editedRgb;
}

}

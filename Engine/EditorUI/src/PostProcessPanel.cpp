#include <kimia/PostProcessPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

void drawPostProcessPanel(const Rect& rect, const PostProcessProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Post FX", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 16.0f;
  f32 y = rect.y + 18.0f;
  char buf[32];

  bool b = props.bloom;
  if (checkbox("Bloom", b, {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.bloomIntensity);
  drawText(buf, rect.x + 22.0f, y, 1, b ? kText : kTextDim);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.bloomThreshold);
  drawText(buf, rect.x + 22.0f, y, 1, b ? kText : kTextDim);
  y += rowH;

  bool t = props.tonemap;
  if (checkbox("Tonemap", t, {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.exposure);
  drawText(buf, rect.x + 22.0f, y, 1, t ? kText : kTextDim);
  y += rowH;

  bool s = props.ssao;
  if (checkbox("SSAO", s, {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.ssaoRadius);
  drawText(buf, rect.x + 22.0f, y, 1, s ? kText : kTextDim);
  y += rowH;

  bool f = props.fxaa;
  if (checkbox("FXAA", f, {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;

  bool v = props.vignette;
  if (checkbox("Vignette", v, {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.vignetteIntensity);
  drawText(buf, rect.x + 22.0f, y, 1, v ? kText : kTextDim);
  y += rowH;

  bool m = props.motionBlur;
  if (checkbox("Motion blur", m, {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
}
}  // namespace kimia::ui

#include <kimia/ThemePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
const char* nameOf(ThemeName t) {
  switch (t) {
    case ThemeName::DarkPro:    return "Dark Pro";
    case ThemeName::LightPro:   return "Light Pro";
    case ThemeName::Solarized:  return "Solarized";
    case ThemeName::Monokai:    return "Monokai";
  }
  return "?";
}
Color accentOf(ThemeName t) {
  switch (t) {
    case ThemeName::DarkPro:    return {0.290f, 0.565f, 0.886f, 1.0f};
    case ThemeName::LightPro:   return {0.196f, 0.471f, 0.886f, 1.0f};
    case ThemeName::Solarized:  return {0.521f, 0.600f, 0.000f, 1.0f};
    case ThemeName::Monokai:    return {0.972f, 0.149f, 0.486f, 1.0f};
  }
  return theme::kAccent;
}
Color bgOf(ThemeName t) {
  switch (t) {
    case ThemeName::DarkPro:    return {0.219f, 0.219f, 0.219f, 1.0f};
    case ThemeName::LightPro:   return {0.929f, 0.929f, 0.929f, 1.0f};
    case ThemeName::Solarized:  return {0.988f, 0.964f, 0.882f, 1.0f};
    case ThemeName::Monokai:    return {0.156f, 0.156f, 0.156f, 1.0f};
  }
  return theme::kBg;
}
}

void drawThemePanel(const Rect& rect, ThemeName theme) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Theme", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 swatchW = 80.0f;
  constexpr f32 swatchH = 60.0f;
  const f32 padX = 8.0f;
  const f32 padY = 20.0f;
  const ThemeName all[] = {
    ThemeName::DarkPro, ThemeName::LightPro,
    ThemeName::Solarized, ThemeName::Monokai,
  };
  for (int i = 0; i < 4; ++i) {
    const f32 x = rect.x + padX + static_cast<float>(i % 2) * (swatchW + 8.0f);
    const f32 y = rect.y + padY + static_cast<float>(i / 2) * (swatchH + 8.0f);
    const bool active = all[i] == theme;
    drawRect({x - 2.0f, y - 2.0f, swatchW + 4.0f, swatchH + 4.0f},
             active ? kAccent : kBorder, 2.0f);
    drawRect({x, y, swatchW, swatchH}, bgOf(all[i]), 2.0f);
    drawRect({x + 4.0f, y + 4.0f, swatchW - 8.0f, 4.0f},
             accentOf(all[i]), 0.0f);
    drawText(nameOf(all[i]),
             x + 4.0f, y + swatchH - 14.0f, 1, kText);
  }
}

}

#include <kimia/PreferencesPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
const char* languageName(i32 lang) {
  switch (lang) {
    case 0: return "English";
    case 1: return "Persian";
    case 2: return "Japanese";
  }
  return "?";
}
const char* backendName(i32 id) {
  switch (id) {
    case 0: return "GLES3";
    case 1: return "Vulkan";
    case 2: return "Software";
  }
  return "?";
}
}

void drawPreferencesPanel(const Rect& rect, const PrefsValues& p) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Preferences", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 20.0f;
  f32 y = rect.y + 18.0f;

  bool a = p.autosave;
  if (checkbox("Auto-save scene", a,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  bool r = p.autoReloadOnChange;
  if (checkbox("Auto-reload on change", r,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  bool l = p.showLineNumbers;
  if (checkbox("Show line numbers in scripts", l,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  bool v = p.enableVibration;
  if (checkbox("Vibration feedback", v,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
  bool s = p.enableSounds;
  if (checkbox("UI sounds", s,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH + 4.0f;

  // UI scale row.
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.2fx", static_cast<double>(p.uiScale));
  drawText("UI scale", rect.x + 4.0f, y + 4.0f, 1, kText);
  drawText(buf, rect.x + rect.w - 60.0f, y + 4.0f, 1, kAccent);
  y += rowH;

  // Language row.
  drawText("Language", rect.x + 4.0f, y + 4.0f, 1, kText);
  drawText(languageName(p.language),
           rect.x + rect.w - 80.0f, y + 4.0f, 1, kAccent);
  y += rowH;

  // Backend row.
  drawText("Render backend", rect.x + 4.0f, y + 4.0f, 1, kText);
  drawText(backendName(p.renderBackend),
           rect.x + rect.w - 80.0f, y + 4.0f, 1, kAccent);
  y += rowH;
}

}

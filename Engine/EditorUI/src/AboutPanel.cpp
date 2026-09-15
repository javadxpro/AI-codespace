#include <kimia/AboutPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
void row(const Rect& rect, const char* label, const std::string& value) {
  using namespace theme;
  constexpr f32 labelW = 80.0f;
  drawText(label, rect.x + 4.0f, rect.y + 4.0f, 1, kTextMuted);
  drawText(value.c_str(), rect.x + labelW, rect.y + 4.0f, 1, kText);
}
}

void drawAboutPanel(const Rect& rect,
                    const std::string& engineName,
                    const std::string& version,
                    const std::string& buildDate,
                    const std::string& platform,
                    const std::string& gpu) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  // Centered title.
  const f32 titleY = rect.y + 18.0f;
  drawText(engineName.c_str(),
           rect.x + rect.w * 0.5f - 50.0f, titleY, 1, kAccentHot);
  drawText("Open-source game engine",
           rect.x + rect.w * 0.5f - 60.0f, titleY + 16.0f, 1, kTextMuted);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 60.0f;
  row({rect.x + 16.0f, y, rect.w - 16.0f, rowH}, "Version", version);
  y += rowH;
  row({rect.x + 16.0f, y, rect.w - 16.0f, rowH}, "Build",   buildDate);
  y += rowH;
  row({rect.x + 16.0f, y, rect.w - 16.0f, rowH}, "Platform", platform);
  y += rowH;
  row({rect.x + 16.0f, y, rect.w - 16.0f, rowH}, "GPU",     gpu);
  y += rowH + 12.0f;

  drawRect({rect.x + 16.0f, y, rect.w - 32.0f, 1.0f},
           kPanelAlt, 0.0f);
  y += 6.0f;
  drawText("MIT License",
           rect.x + 16.0f, y, 1, kText);
  y += rowH;
  drawText("(c) 2026 Kimia Engine contributors",
           rect.x + 16.0f, y, 1, kTextMuted);
}

}

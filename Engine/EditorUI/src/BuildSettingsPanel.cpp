#include <kimia/BuildSettingsPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
const char* platformName(BuildPlatform p) {
  switch (p) {
    case BuildPlatform::Android: return "Android";
    case BuildPlatform::Windows: return "Windows";
    case BuildPlatform::Linux:   return "Linux";
    case BuildPlatform::Web:     return "Web";
  }
  return "?";
}
const char* configName(BuildConfig c) {
  switch (c) {
    case BuildConfig::Debug:    return "Debug";
    case BuildConfig::Release:  return "Release";
    case BuildConfig::Profile:  return "Profile";
  }
  return "?";
}
void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 80.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
}
}

void drawBuildSettingsPanel(const Rect& rect, const BuildSettingsProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Build", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;

  drawRow({rect.x, y, rect.w, rowH}, "Platform", platformName(props.platform));
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Config",   configName(props.config));
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Embed assets",  props.embedAssets ? "yes" : "no");
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "arm64-only",    props.arm64Only ? "yes" : "no");
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Compress",      props.compressAssets ? "yes" : "no");
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Strip symbols", props.stripDebugSymbols ? "yes" : "no");
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Output name",   props.outputName.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Version",       props.version.c_str());
}

}

#include <kimia/SceneSettingsPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
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

void drawSceneSettingsPanel(const Rect& rect, const SceneSettingsProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Scene", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[64];

  drawRow({rect.x, y, rect.w, rowH}, "Name", props.sceneName.c_str());
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.gravity);
  drawRow({rect.x, y, rect.w, rowH}, "Gravity", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f, %.2f, %.2f",
                (double)props.ambientColor.x,
                (double)props.ambientColor.y,
                (double)props.ambientColor.z);
  drawRow({rect.x, y, rect.w, rowH}, "Ambient", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.2f", props.ambientIntensity);
  drawRow({rect.x, y, rect.w, rowH}, "Amb. int.", buf);
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Fog",
          props.fogEnabled ? "on" : "off");
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.1f", props.fogStart);
  drawRow({rect.x, y, rect.w, rowH}, "Fog start", buf);
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%.1f", props.fogEnd);
  drawRow({rect.x, y, rect.w, rowH}, "Fog end", buf);
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Physics",
          props.physicsEnabled ? "on" : "off");
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Auto save",
          props.autoSave ? "on" : "off");
  y += rowH;
  std::snprintf(buf, sizeof(buf), "%d s", props.autoSaveIntervalSec);
  drawRow({rect.x, y, rect.w, rowH}, "Auto int.", buf);
}

}

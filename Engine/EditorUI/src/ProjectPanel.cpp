#include <kimia/ProjectPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

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

void drawProjectPanel(const Rect& rect, const ProjectProps& props) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Project", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;

  drawRow({rect.x, y, rect.w, rowH}, "Name",    props.projectName.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Company",  props.companyName.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Version",  props.version.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Scenes",   props.scenesDir.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Assets",   props.assetsDir.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Profiles", props.profilesDir.c_str());
  y += rowH;
  drawRow({rect.x, y, rect.w, rowH}, "Build",    props.buildDir.c_str());
  y += rowH;

  bool e = props.useEditorUi;
  if (checkbox("Use native editor", e,
               {rect.x + 4.0f, y, rect.w - 8.0f, rowH})) {}
  y += rowH;
}

}

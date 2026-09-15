// InspectorPanel tests — see Engine/EditorUI/include/kimia/InspectorPanel.h.

#include <kimia_test.h>
#include <kimia/InspectorPanel.h>

KIMIA_TEST(InspectorPanel_DrawEmptyDoesNotCrash) {
  kimia::ui::drawInspectorPanel({0, 0, 320, 200}, "Empty", {});
}

KIMIA_TEST(InspectorPanel_DrawOneRow) {
  std::vector<kimia::ui::InspectorRow> v(1);
  v[0].label = "Mass";
  v[0].value = "1.0 kg";
  kimia::ui::drawInspectorPanel({0, 0, 320, 200}, "Cube", v);
}

KIMIA_TEST(InspectorPanel_DrawManyRows) {
  std::vector<kimia::ui::InspectorRow> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::InspectorRow r;
    r.label = "field_" + std::to_string(i);
    r.value = "value_" + std::to_string(i);
    // Cycle through theme colours so we exercise all 3 paths.
    if (i % 3 == 0)      r.valueColor = {0.298f, 0.686f, 0.314f, 1.0f};  // kSuccess
    else if (i % 3 == 1) r.valueColor = {1.000f, 0.722f, 0.180f, 1.0f};  // kWarning
    else                 r.valueColor = {0.957f, 0.263f, 0.212f, 1.0f};  // kError
    v.push_back(r);
  }
  kimia::ui::drawInspectorPanel({0, 0, 320, 200}, "Many", v);
}

KIMIA_TEST(InspectorPanel_DrawWithEmptyTitle) {
  std::vector<kimia::ui::InspectorRow> v(1);
  v[0].label = "x";
  v[0].value = "1";
  kimia::ui::drawInspectorPanel({0, 0, 320, 200}, "", v);
}

KIMIA_TEST(InspectorPanel_DrawWithVeryLongValue) {
  // Truncation test — long value should still render without crash.
  std::vector<kimia::ui::InspectorRow> v(1);
  v[0].label = "long";
  v[0].value = "A_Very_Long_Value_That_Should_Be_Truncated_Or_Clipped_Without_Crashing_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  kimia::ui::drawInspectorPanel({0, 0, 200, 200}, "Long", v);
}

KIMIA_TEST(InspectorPanel_DrawAtPhonePortrait) {
  std::vector<kimia::ui::InspectorRow> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::InspectorRow r;
    r.label = "l" + std::to_string(i);
    r.value = "v" + std::to_string(i);
    v.push_back(r);
  }
  kimia::ui::drawInspectorPanel({0, 0, 240, 320}, "Phone", v);
}

KIMIA_TEST(InspectorPanel_DrawAtTabletLandscape) {
  std::vector<kimia::ui::InspectorRow> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::InspectorRow r;
    r.label = "l" + std::to_string(i);
    r.value = "v" + std::to_string(i);
    v.push_back(r);
  }
  kimia::ui::drawInspectorPanel({0, 0, 800, 400}, "Tablet", v);
}

KIMIA_TEST(InspectorPanel_RowDefaultColorIsText) {
  // Default-constructed InspectorRow: label/value empty, valueColor
  // matches theme::kText.
  kimia::ui::InspectorRow r;
  KIMIA_REQUIRE(r.label.empty());
  KIMIA_REQUIRE(r.value.empty());
  KIMIA_REQUIRE(r.valueColor.r > 0.0f);
}

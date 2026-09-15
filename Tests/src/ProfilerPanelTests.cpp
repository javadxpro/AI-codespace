// ProfilerPanel tests — see Engine/EditorUI/include/kimia/ProfilerPanel.h.

#include <kimia_test.h>
#include <kimia/ProfilerPanel.h>

KIMIA_TEST(ProfilerPanel_DrawEmptyDoesNotCrash) {
  kimia::ui::drawProfilerPanel({0, 0, 240, 120}, {});
}

KIMIA_TEST(ProfilerPanel_DrawOneRow) {
  std::vector<kimia::ui::ProfilerRow> v(1);
  v[0].label = "Update";
  v[0].lastMs = 0.5f;
  v[0].maxMs = 1.0f;
  kimia::ui::drawProfilerPanel({0, 0, 240, 120}, v);
}

KIMIA_TEST(ProfilerPanel_DrawTypicalFrame) {
  std::vector<kimia::ui::ProfilerRow> v;
  const char* labels[] = {"Update", "Physics", "Render", "Editor", "GPU"};
  const float lastMs[] = {0.3f, 2.5f, 5.0f, 0.8f, 1.5f};
  const float maxMs[]  = {1.0f, 4.0f, 8.0f, 2.0f, 3.0f};
  for (int i = 0; i < 5; ++i) {
    kimia::ui::ProfilerRow r;
    r.label = labels[i];
    r.lastMs = lastMs[i];
    r.maxMs = maxMs[i];
    v.push_back(r);
  }
  kimia::ui::drawProfilerPanel({0, 0, 240, 200}, v);
}

KIMIA_TEST(ProfilerPanel_DrawWithWarningValue) {
  // > 16 ms flips the readout to the warning colour.
  std::vector<kimia::ui::ProfilerRow> v(1);
  v[0].label = "Render";
  v[0].lastMs = 25.0f;
  v[0].maxMs = 30.0f;
  kimia::ui::drawProfilerPanel({0, 0, 240, 120}, v);
}

KIMIA_TEST(ProfilerPanel_DrawWithZeroMax) {
  // maxMs == 0 → ratio is 0 (bar empty), must not crash.
  std::vector<kimia::ui::ProfilerRow> v(1);
  v[0].label = "Idle";
  v[0].lastMs = 0.0f;
  v[0].maxMs = 0.0f;
  kimia::ui::drawProfilerPanel({0, 0, 240, 120}, v);
}

KIMIA_TEST(ProfilerPanel_DrawWithOverMax) {
  // lastMs > maxMs → ratio clamps to 1.
  std::vector<kimia::ui::ProfilerRow> v(1);
  v[0].label = "Spike";
  v[0].lastMs = 100.0f;
  v[0].maxMs = 10.0f;
  kimia::ui::drawProfilerPanel({0, 0, 240, 120}, v);
}

KIMIA_TEST(ProfilerPanel_DrawAtPhonePortrait) {
  std::vector<kimia::ui::ProfilerRow> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::ProfilerRow r;
    r.label = "s" + std::to_string(i);
    r.lastMs = static_cast<float>(i) * 0.5f;
    r.maxMs = 5.0f;
    v.push_back(r);
  }
  kimia::ui::drawProfilerPanel({0, 0, 240, 200}, v);
}

KIMIA_TEST(ProfilerPanel_DrawAtTabletLandscape) {
  std::vector<kimia::ui::ProfilerRow> v;
  for (int i = 0; i < 8; ++i) {
    kimia::ui::ProfilerRow r;
    r.label = "s" + std::to_string(i);
    r.lastMs = static_cast<float>(i);
    r.maxMs = 10.0f;
    v.push_back(r);
  }
  kimia::ui::drawProfilerPanel({0, 0, 480, 200}, v);
}

KIMIA_TEST(ProfilerPanel_RowDefaultIsZero) {
  kimia::ui::ProfilerRow r;
  KIMIA_REQUIRE(r.lastMs == 0.0f);
  KIMIA_REQUIRE(r.maxMs == 0.0f);
  KIMIA_REQUIRE(r.label.empty());
}

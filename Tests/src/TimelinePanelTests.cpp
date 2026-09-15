#include <kimia_test.h>
#include <kimia/TimelinePanel.h>

KIMIA_TEST(Timeline_DrawEmptyDoesNotCrash) {
  kimia::ui::drawTimelinePanel({0, 0, 360, 200}, {}, 0.0f, 10.0f, 1.0f);
}

KIMIA_TEST(Timeline_DrawOneBlock) {
  std::vector<kimia::ui::TimelineBlock> v(1);
  v[0] = {"Scene1", 0.5f, 2.0f, 80, 200, 240, false};
  kimia::ui::drawTimelinePanel({0, 0, 360, 200}, v, 0.0f, 10.0f, 1.0f);
}

KIMIA_TEST(Timeline_DrawManyBlocks) {
  std::vector<kimia::ui::TimelineBlock> v;
  v.push_back({"Intro",       0.0f, 2.0f, 200, 80,  80,  false});
  v.push_back({"Gameplay",    2.0f, 5.0f, 80,  200, 80,  false});
  v.push_back({"Cutscene",    7.0f, 1.5f, 80,  80,  200, true});
  v.push_back({"Score",       8.5f, 0.5f, 200, 200, 80,  false});
  v.push_back({"Outro",       9.0f, 3.0f, 200, 80,  200, false});
  kimia::ui::drawTimelinePanel({0, 0, 480, 320}, v, 0.0f, 12.0f, 5.0f);
}

KIMIA_TEST(Timeline_DrawWithPlayheadOutOfView) {
  std::vector<kimia::ui::TimelineBlock> v(1);
  v[0] = {"B", 2.0f, 2.0f, 200, 100, 100, false};
  kimia::ui::drawTimelinePanel({0, 0, 360, 200}, v, 0.0f, 10.0f, -2.0f);
  kimia::ui::drawTimelinePanel({0, 0, 360, 200}, v, 0.0f, 10.0f, 15.0f);
}

KIMIA_TEST(Timeline_DrawWithZeroView) {
  std::vector<kimia::ui::TimelineBlock> v(1);
  v[0] = {"B", 0.0f, 1.0f, 200, 100, 100, false};
  kimia::ui::drawTimelinePanel({0, 0, 360, 200}, v, 0.0f, 0.0f, 0.0f);
}

KIMIA_TEST(Timeline_DrawWithBlockOutsideView) {
  std::vector<kimia::ui::TimelineBlock> v;
  v.push_back({"Before",  -5.0f, 2.0f, 100, 100, 100, false});
  v.push_back({"During",   4.0f, 2.0f, 200, 200, 100, false});
  v.push_back({"After",   20.0f, 2.0f, 100, 200, 200, false});
  kimia::ui::drawTimelinePanel({0, 0, 360, 200}, v, 0.0f, 10.0f, 5.0f);
}

KIMIA_TEST(Timeline_DrawAtPhonePortrait) {
  std::vector<kimia::ui::TimelineBlock> v;
  v.push_back({"A", 0.0f, 2.0f, 200, 80, 80, false});
  v.push_back({"B", 2.0f, 1.0f, 80, 200, 80, false});
  v.push_back({"C", 3.0f, 3.0f, 80, 80, 200, false});
  kimia::ui::drawTimelinePanel({0, 0, 240, 320}, v, 0.0f, 6.0f, 2.5f);
}

KIMIA_TEST(Timeline_DrawAtTabletLandscape) {
  std::vector<kimia::ui::TimelineBlock> v;
  for (int i = 0; i < 10; ++i) {
    v.push_back({"Block" + std::to_string(i),
                 static_cast<kimia::f32>(i),
                 static_cast<kimia::f32>(i % 3 + 1),
                 static_cast<kimia::u8>((i * 30) & 255),
                 static_cast<kimia::u8>((i * 60) & 255),
                 static_cast<kimia::u8>((i * 90) & 255),
                 i == 4});
  }
  kimia::ui::drawTimelinePanel({0, 0, 480, 320}, v, 0.0f, 15.0f, 8.0f);
}

#include <kimia_test.h>
#include <kimia/AnimationTrackPanel.h>

KIMIA_TEST(AnimTrack_DrawEmptyDoesNotCrash) {
  kimia::ui::drawAnimationTrackPanel({0, 0, 360, 200}, {}, 0.0f, 1.0f, 0.5f, -1);
}

KIMIA_TEST(AnimTrack_DrawOneKeyframe) {
  std::vector<kimia::ui::AnimTrack> t(1);
  t[0].name = "x";
  t[0].keys.push_back({0.5f, 0.0f, false});
  kimia::ui::drawAnimationTrackPanel({0, 0, 360, 100}, t, 0.0f, 1.0f, 0.3f, 0);
}

KIMIA_TEST(AnimTrack_DrawManyTracks) {
  std::vector<kimia::ui::AnimTrack> t;
  t.push_back({"Pos.x"});
  t.push_back({"Pos.y"});
  t.push_back({"Pos.z"});
  t.push_back({"Rot.y"});
  t.push_back({"Scale"});
  for (size_t i = 0; i < t.size(); ++i) {
    for (int k = 0; k < 8; ++k) {
      t[i].keys.push_back({static_cast<kimia::f32>(k) * 0.1f + 0.1f,
                           static_cast<kimia::f32>(k % 3 - 1) * 0.5f,
                           k == 3});
    }
  }
  kimia::ui::drawAnimationTrackPanel({0, 0, 480, 200}, t, 0.0f, 1.0f, 0.5f, 1);
}

KIMIA_TEST(AnimTrack_DrawWithPlayheadAtStart) {
  std::vector<kimia::ui::AnimTrack> t(1);
  t[0].name = "rot";
  t[0].keys.push_back({0.0f, 0.0f, false});
  t[0].keys.push_back({1.0f, 1.0f, false});
  kimia::ui::drawAnimationTrackPanel({0, 0, 360, 80}, t, 0.0f, 1.0f, 0.0f, 0);
}

KIMIA_TEST(AnimTrack_DrawWithPlayheadOutOfView) {
  std::vector<kimia::ui::AnimTrack> t(1);
  t[0].name = "x";
  t[0].keys.push_back({0.5f, 0.5f, false});
  kimia::ui::drawAnimationTrackPanel({0, 0, 360, 80}, t, 0.0f, 1.0f, 2.0f, -1);
  kimia::ui::drawAnimationTrackPanel({0, 0, 360, 80}, t, 0.0f, 1.0f, -1.0f, -1);
}

KIMIA_TEST(AnimTrack_DrawWithZeroView) {
  std::vector<kimia::ui::AnimTrack> t(1);
  t[0].name = "x";
  t[0].keys.push_back({0.5f, 0.5f, false});
  kimia::ui::drawAnimationTrackPanel({0, 0, 360, 80}, t, 0.0f, 0.0f, 0.0f, 0);
}

KIMIA_TEST(AnimTrack_DrawAtPhonePortrait) {
  std::vector<kimia::ui::AnimTrack> t(3);
  t[0].name = "x";
  t[1].name = "y";
  t[2].name = "z";
  for (int i = 0; i < 3; ++i) {
    for (int k = 0; k < 5; ++k) {
      t[i].keys.push_back({static_cast<kimia::f32>(k) * 0.2f, 0.0f, false});
    }
  }
  kimia::ui::drawAnimationTrackPanel({0, 0, 240, 320}, t, 0.0f, 1.0f, 0.5f, 0);
}

KIMIA_TEST(AnimTrack_DrawAtTabletLandscape) {
  std::vector<kimia::ui::AnimTrack> t(8);
  for (int i = 0; i < 8; ++i) {
    t[i].name = "T" + std::to_string(i);
    for (int k = 0; k < 6; ++k) {
      t[i].keys.push_back({static_cast<kimia::f32>(k) * 0.16f,
                           static_cast<kimia::f32>(k % 4) * 0.25f,
                           k == 2});
    }
  }
  kimia::ui::drawAnimationTrackPanel({0, 0, 480, 320}, t, 0.0f, 1.0f, 0.7f, 4);
}

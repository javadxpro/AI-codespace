#include <kimia_test.h>
#include <kimia/AnimationPanel.h>

KIMIA_TEST(AnimationPanel_DrawDefaultDoesNotCrash) {
  kimia::ui::AnimationProps p;
  kimia::ui::drawAnimationPanel({0, 0, 240, 220}, p);
}

KIMIA_TEST(AnimationPanel_DrawPlayingLoop) {
  kimia::ui::AnimationProps p;
  p.clipName = "walk";
  p.durationSec = 2.0f;
  p.currentTime = 1.0f;
  p.playbackRate = 1.0f;
  p.loop = kimia::ui::AnimationLoop::Loop;
  p.playing = true;
  kimia::ui::drawAnimationPanel({0, 0, 240, 220}, p);
}

KIMIA_TEST(AnimationPanel_DrawPausedOnce) {
  kimia::ui::AnimationProps p;
  p.clipName = "death";
  p.durationSec = 0.5f;
  p.currentTime = 0.5f;
  p.playbackRate = 0.5f;
  p.loop = kimia::ui::AnimationLoop::Once;
  p.playing = true;
  p.paused = true;
  kimia::ui::drawAnimationPanel({0, 0, 240, 220}, p);
}

KIMIA_TEST(AnimationPanel_DrawPingPong) {
  kimia::ui::AnimationProps p;
  p.clipName = "bounce";
  p.loop = kimia::ui::AnimationLoop::PingPong;
  p.currentTime = 0.3f;
  p.durationSec = 1.0f;
  kimia::ui::drawAnimationPanel({0, 0, 240, 220}, p);
}

KIMIA_TEST(AnimationPanel_DrawZeroDuration) {
  kimia::ui::AnimationProps p;
  p.durationSec = 0.0f;
  p.currentTime = 0.0f;
  kimia::ui::drawAnimationPanel({0, 0, 240, 220}, p);
}

KIMIA_TEST(AnimationPanel_DrawAtPhonePortrait) {
  kimia::ui::AnimationProps p;
  p.clipName = "run";
  kimia::ui::drawAnimationPanel({0, 0, 240, 320}, p);
}

KIMIA_TEST(AnimationPanel_DrawAtTabletLandscape) {
  kimia::ui::AnimationProps p;
  p.clipName = "idle";
  kimia::ui::drawAnimationPanel({0, 0, 480, 240}, p);
}

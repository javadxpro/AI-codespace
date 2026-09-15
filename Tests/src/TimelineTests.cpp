// Timeline tests — see Engine/EditorUI/include/kimia/Timeline.h.

#include <kimia_test.h>
#include <kimia/Timeline.h>

KIMIA_TEST(Timeline_DrawIdleDoesNotCrash) {
  kimia::ui::TimelineState s;
  s.playing = false;
  s.paused = false;
  s.playhead = 0.0f;
  s.totalTime = 0.0f;
  kimia::ui::drawTimeline({0, 0, 320, 28}, s);
}

KIMIA_TEST(Timeline_DrawPlayingDoesNotCrash) {
  kimia::ui::TimelineState s;
  s.playing = true;
  s.playhead = 0.42f;
  s.totalTime = 73.5f;
  kimia::ui::drawTimeline({0, 0, 320, 28}, s);
}

KIMIA_TEST(Timeline_DrawPausedDoesNotCrash) {
  kimia::ui::TimelineState s;
  s.playing = true;
  s.paused = true;
  s.playhead = 1.0f;
  s.totalTime = 600.0f;  // 10:00
  kimia::ui::drawTimeline({0, 0, 320, 28}, s);
}

KIMIA_TEST(Timeline_DrawPlayheadClampedAtZeroAndOne) {
  // playhead outside [0,1] must clamp, not crash.
  kimia::ui::TimelineState s;
  s.playhead = -0.5f;
  kimia::ui::drawTimeline({0, 0, 200, 24}, s);
  s.playhead = 1.5f;
  kimia::ui::drawTimeline({0, 0, 200, 24}, s);
}

KIMIA_TEST(Timeline_DrawAtDifferentSizes) {
  // Phone-portrait, phone-landscape, tablet.
  kimia::ui::TimelineState s;
  s.playing = true;
  s.playhead = 0.3f;
  s.totalTime = 12.3f;
  kimia::ui::drawTimeline({0, 0, 240, 24}, s);
  kimia::ui::drawTimeline({0, 0, 480, 24}, s);
  kimia::ui::drawTimeline({0, 0, 800, 32}, s);
}

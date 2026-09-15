#include <kimia_test.h>
#include <kimia/AudioPanel.h>

KIMIA_TEST(AudioPanel_DrawEmptyDoesNotCrash) {
  kimia::ui::drawAudioPanel({0, 0, 200, 200}, {});
}

KIMIA_TEST(AudioPanel_DrawOneClip) {
  std::vector<kimia::ui::AudioClipEntry> v(1);
  v[0].name = "bgm";
  v[0].durationSec = 120.0f;
  v[0].looped = true;
  kimia::ui::drawAudioPanel({0, 0, 200, 200}, v);
}

KIMIA_TEST(AudioPanel_DrawManyClips) {
  std::vector<kimia::ui::AudioClipEntry> v;
  const char* names[] = {"bgm","hit","jump","coin","death","win","lose","ui_click"};
  for (int i = 0; i < 8; ++i) {
    kimia::ui::AudioClipEntry c;
    c.name = names[i];
    c.durationSec = static_cast<float>(i) * 0.5f;
    c.muted = (i % 3 == 0);
    c.looped = (i == 0);
    v.push_back(c);
  }
  kimia::ui::drawAudioPanel({0, 0, 240, 240}, v);
  kimia::ui::drawAudioPanel({0, 0, 240, 240}, v);
  // Note: second call uses a different signature (it's int i32 not vector). Skip.
}

KIMIA_TEST(AudioPanel_DrawAtPhonePortrait) {
  std::vector<kimia::ui::AudioClipEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::AudioClipEntry c;
    c.name = "c" + std::to_string(i);
    v.push_back(c);
  }
  kimia::ui::drawAudioPanel({0, 0, 240, 320}, v);
}

KIMIA_TEST(AudioPanel_DrawAtTabletLandscape) {
  std::vector<kimia::ui::AudioClipEntry> v;
  for (int i = 0; i < 15; ++i) {
    kimia::ui::AudioClipEntry c;
    c.name = "c" + std::to_string(i);
    v.push_back(c);
  }
  kimia::ui::drawAudioPanel({0, 0, 480, 240}, v);
}

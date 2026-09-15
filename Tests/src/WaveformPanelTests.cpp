#include <kimia_test.h>
#include <kimia/WaveformPanel.h>
#include <cmath>

KIMIA_TEST(Waveform_DrawEmptyDoesNotCrash) {
  kimia::ui::drawWaveformPanel({0, 0, 320, 200}, {}, 44100.0f, 0.0f, false);
}

KIMIA_TEST(Waveform_DrawSilence) {
  std::vector<kimia::f32> s(4410, 0.0f);
  kimia::ui::drawWaveformPanel({0, 0, 320, 200}, s, 44100.0f, 0.0f, false);
}

KIMIA_TEST(Waveform_DrawSineWave) {
  std::vector<kimia::f32> s;
  for (int i = 0; i < 4410; ++i) {
    s.push_back(static_cast<kimia::f32>(
        std::sin(static_cast<double>(i) * 6.28 * 440.0 / 44100.0)));
  }
  kimia::ui::drawWaveformPanel({0, 0, 320, 200}, s, 44100.0f, 0.0f, false);
}

KIMIA_TEST(Waveform_DrawWithPlayhead) {
  std::vector<kimia::f32> s;
  for (int i = 0; i < 8820; ++i) {
    s.push_back(static_cast<kimia::f32>(
        std::sin(static_cast<double>(i) * 0.1) * 0.7));
  }
  kimia::ui::drawWaveformPanel({0, 0, 320, 200}, s, 44100.0f, 0.05f, true);
  kimia::ui::drawWaveformPanel({0, 0, 320, 200}, s, 44100.0f, 0.2f, false);
}

KIMIA_TEST(Waveform_DrawWithOutOfRangePlayhead) {
  std::vector<kimia::f32> s(4410, 0.5f);
  kimia::ui::drawWaveformPanel({0, 0, 320, 200}, s, 44100.0f, -1.0f, false);
  kimia::ui::drawWaveformPanel({0, 0, 320, 200}, s, 44100.0f, 100.0f, false);
}

KIMIA_TEST(Waveform_DrawWithZeroSampleRate) {
  std::vector<kimia::f32> s(1000, 0.5f);
  kimia::ui::drawWaveformPanel({0, 0, 320, 200}, s, 0.0f, 0.0f, false);
}

KIMIA_TEST(Waveform_DrawAtPhonePortrait) {
  std::vector<kimia::f32> s;
  for (int i = 0; i < 2205; ++i) {
    s.push_back(static_cast<kimia::f32>(
        std::sin(static_cast<double>(i) * 0.2) * 0.5));
  }
  kimia::ui::drawWaveformPanel({0, 0, 240, 320}, s, 22050.0f, 0.02f, true);
}

KIMIA_TEST(Waveform_DrawAtTabletLandscape) {
  std::vector<kimia::f32> s;
  for (int i = 0; i < 17640; ++i) {
    s.push_back(static_cast<kimia::f32>(
        std::sin(static_cast<double>(i) * 0.05)));
  }
  kimia::ui::drawWaveformPanel({0, 0, 480, 320}, s, 44100.0f, 0.3f, true);
}

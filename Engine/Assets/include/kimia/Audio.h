#pragma once

#include <kimia/Types.h>

#include <optional>
#include <string>
#include <vector>

namespace kimia {

// Decoded PCM audio. Samples are interleaved f32 frames in [-1, 1].
struct AudioBuffer {
  i32 channels = 0;
  i32 sampleRate = 0;
  u64 frameCount = 0;
  std::vector<f32> samples;

  bool isEmpty() const { return channels <= 0 || sampleRate <= 0 || samples.empty(); }
  f64 durationSeconds() const;
  std::vector<f32> downmixMono() const;

  // Detects WAV/MP3/OGG/FLAC by extension.
  static std::optional<AudioBuffer> load(const std::string& path, std::string& error);
  static std::optional<AudioBuffer> loadWAV(const std::string& path, std::string& error);
  static std::optional<AudioBuffer> loadMP3(const std::string& path, std::string& error);
  static std::optional<AudioBuffer> loadOGG(const std::string& path, std::string& error);
  static std::optional<AudioBuffer> loadFLAC(const std::string& path, std::string& error);

  // Writes 16-bit PCM WAV.
  bool writeWAV(const std::string& path) const;
  // The same 16-bit PCM WAV as bytes (44-byte RIFF header + data); empty if
  // the buffer is empty. What the web page streams as /sfx/<name>.
  std::vector<u8> encodeWAV() const;

  // Procedural cues (no asset files needed): a short mono tone with an
  // exponential decay — `frequency` Hz for `seconds`, optional pitch slide to
  // `endFrequency` (0 = none). Deterministic: same arguments, same samples.
  static AudioBuffer tone(f64 frequency, f64 seconds, f64 amplitude = 0.6, f64 endFrequency = 0.0,
                          i32 sampleRateHz = 22050);
  // A soft noise burst (the "thock" of a club/foot on a ball): white noise
  // through a one-pole low-pass at `cutoffHz`, decaying over `seconds`.
  static AudioBuffer thock(f64 seconds, f64 cutoffHz = 900.0, f64 amplitude = 0.7, i32 sampleRateHz = 22050);
  // Two buffers played one after the other (same channel count/sample rate).
  static AudioBuffer concat(const AudioBuffer& first, const AudioBuffer& second);
};

}  // namespace kimia

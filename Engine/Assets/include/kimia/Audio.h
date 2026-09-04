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

  // Detects WAV/MP3 by extension.
  static std::optional<AudioBuffer> load(const std::string& path, std::string& error);
  static std::optional<AudioBuffer> loadWAV(const std::string& path, std::string& error);
  static std::optional<AudioBuffer> loadMP3(const std::string& path, std::string& error);

  // Writes 16-bit PCM WAV.
  bool writeWAV(const std::string& path) const;
};

}  // namespace kimia

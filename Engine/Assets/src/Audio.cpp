#include <kimia/Audio.h>

// Vendored third-party code: keep strict warnings scoped to our own code.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define DR_WAV_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>
#include <dr_wav.h>
#pragma GCC diagnostic pop

#include <limits>

namespace kimia {

namespace {

bool endsWithIgnoreCase(const std::string& text, const std::string& suffix) {
  if (text.size() < suffix.size()) return false;
  usize i = text.size() - suffix.size();
  for (usize j = 0; j < suffix.size(); ++j) {
    char a = text[i + j];
    char b = suffix[j];
    if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
    if (a != b) return false;
  }
  return true;
}

std::optional<AudioBuffer> fromWAV(const std::string& path, std::string& error) {
  drwav wav;
  if (drwav_init_file(&wav, path.c_str(), nullptr) == DRWAV_FALSE) {
    error = "cannot open WAV file: " + path;
    return std::nullopt;
  }
  AudioBuffer buffer;
  if (wav.channels <= 0 || wav.channels > 64) {
    drwav_uninit(&wav);
    error = "unsupported channel count in WAV: " + path;
    return std::nullopt;
  }
  buffer.channels = static_cast<i32>(wav.channels);
  buffer.sampleRate = static_cast<i32>(wav.sampleRate);
  const u64 totalFrames = static_cast<u64>(wav.totalPCMFrameCount);
  const u64 channelCount = static_cast<u64>(buffer.channels);
  if (totalFrames > (std::numeric_limits<usize>::max() / sizeof(f32)) / channelCount) {
    drwav_uninit(&wav);
    error = "WAV too large: " + path;
    return std::nullopt;
  }
  buffer.samples.resize(static_cast<usize>(totalFrames * channelCount));
  const drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, totalFrames, buffer.samples.data());
  drwav_uninit(&wav);
  if (framesRead != totalFrames) {
    error = "WAV truncated (read " + std::to_string(framesRead) + " of " + std::to_string(totalFrames) + " frames): " + path;
    return std::nullopt;
  }
  buffer.frameCount = totalFrames;
  return buffer;
}

std::optional<AudioBuffer> fromMP3(const std::string& path, std::string& error) {
  drmp3 mp3;
  if (drmp3_init_file(&mp3, path.c_str(), nullptr) == DRMP3_FALSE) {
    error = "cannot open MP3 file: " + path;
    return std::nullopt;
  }
  AudioBuffer buffer;
  if (mp3.channels <= 0 || mp3.channels > 64) {
    drmp3_uninit(&mp3);
    error = "unsupported channel count in MP3: " + path;
    return std::nullopt;
  }
  buffer.channels = static_cast<i32>(mp3.channels);
  buffer.sampleRate = static_cast<i32>(mp3.sampleRate);
  const usize channelCount = static_cast<usize>(buffer.channels);
  constexpr usize kChunkFrames = 4096U;
  std::vector<f32> chunk(kChunkFrames * channelCount);
  for (;;) {
    const drmp3_uint64 framesRead = drmp3_read_pcm_frames_f32(&mp3, kChunkFrames, chunk.data());
    if (framesRead == 0) break;
    buffer.samples.insert(buffer.samples.end(), chunk.data(), chunk.data() + static_cast<usize>(framesRead) * channelCount);
  }
  drmp3_uninit(&mp3);
  if (buffer.samples.empty()) {
    error = "MP3 decoded to zero frames: " + path;
    return std::nullopt;
  }
  buffer.frameCount = static_cast<u64>(buffer.samples.size() / channelCount);
  return buffer;
}

}  // namespace

f64 AudioBuffer::durationSeconds() const {
  if (sampleRate <= 0) return 0.0;
  return static_cast<f64>(frameCount) / static_cast<f64>(sampleRate);
}

std::vector<f32> AudioBuffer::downmixMono() const {
  std::vector<f32> result;
  if (samples.empty() || channels <= 0) return result;
  const usize channelCount = static_cast<usize>(channels);
  const usize frameCountLocal = samples.size() / channelCount;
  result.reserve(frameCountLocal);
  for (usize frame = 0; frame < frameCountLocal; ++frame) {
    f64 sum = 0.0;
    for (usize ch = 0; ch < channelCount; ++ch) {
      sum += static_cast<f64>(samples[frame * channelCount + ch]);
    }
    result.push_back(static_cast<f32>(sum / static_cast<f64>(channelCount)));
  }
  return result;
}

std::optional<AudioBuffer> AudioBuffer::load(const std::string& path, std::string& error) {
  if (endsWithIgnoreCase(path, ".wav")) return loadWAV(path, error);
  if (endsWithIgnoreCase(path, ".mp3")) return loadMP3(path, error);
  error = "unsupported audio format (expected .wav or .mp3): " + path;
  return std::nullopt;
}

std::optional<AudioBuffer> AudioBuffer::loadWAV(const std::string& path, std::string& error) { return fromWAV(path, error); }
std::optional<AudioBuffer> AudioBuffer::loadMP3(const std::string& path, std::string& error) { return fromMP3(path, error); }

bool AudioBuffer::writeWAV(const std::string& path) const {
  if (isEmpty()) return false;
  drwav_data_format format{};
  format.container = drwav_container_riff;
  format.format = DR_WAVE_FORMAT_PCM;
  format.channels = static_cast<drwav_uint32>(channels);
  format.sampleRate = static_cast<drwav_uint32>(sampleRate);
  format.bitsPerSample = 16;
  drwav wav;
  if (drwav_init_file_write(&wav, path.c_str(), &format, nullptr) == DRWAV_FALSE) return false;
  std::vector<drwav_int16> pcm(samples.size());
  drwav_f32_to_s16(pcm.data(), samples.data(), samples.size());
  const drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, frameCount, pcm.data());
  drwav_uninit(&wav);
  return framesWritten == frameCount;
}

}  // namespace kimia

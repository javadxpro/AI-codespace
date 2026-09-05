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
#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>
#include <dr_mp3.h>
#include <dr_wav.h>
#pragma GCC diagnostic pop

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

// stb_vorbis is compiled as C (ThirdParty/stb/stb_vorbis_impl.c); bridge the
// one-shot whole-file decode API here. Returns the total interleaved sample
// count into a malloc()ed short buffer, or -1 on failure.
extern "C" int stb_vorbis_decode_filename(const char* filename, int* channels, int* sample_rate, short** output);

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

std::optional<AudioBuffer> fromOGG(const std::string& path, std::string& error) {
  int channels = 0;
  int sampleRate = 0;
  short* decoded = nullptr;
  const int totalSamples = stb_vorbis_decode_filename(path.c_str(), &channels, &sampleRate, &decoded);
  if (totalSamples <= 0 || decoded == nullptr || channels <= 0 || channels > 64 || sampleRate <= 0) {
    std::free(decoded);
    error = "cannot decode OGG/Vorbis file: " + path;
    return std::nullopt;
  }
  AudioBuffer buffer;
  buffer.channels = static_cast<i32>(channels);
  buffer.sampleRate = static_cast<i32>(sampleRate);
  const usize sampleCount = static_cast<usize>(totalSamples);
  buffer.samples.resize(sampleCount);
  for (usize i = 0; i < sampleCount; ++i) {
    buffer.samples[i] = static_cast<f32>(decoded[i]) / 32768.0f;
  }
  std::free(decoded);
  buffer.frameCount = static_cast<u64>(sampleCount / static_cast<usize>(channels));
  return buffer;
}

std::optional<AudioBuffer> fromFLAC(const std::string& path, std::string& error) {
  drflac_uint32 channels = 0;
  drflac_uint32 sampleRate = 0;
  drflac_uint64 totalFrames = 0;
  f32* decoded = drflac_open_file_and_read_pcm_frames_f32(path.c_str(), &channels, &sampleRate, &totalFrames, nullptr);
  if (decoded == nullptr || channels == 0 || channels > 64 || sampleRate == 0 || totalFrames == 0) {
    if (decoded != nullptr) drflac_free(decoded, nullptr);
    error = "cannot decode FLAC file: " + path;
    return std::nullopt;
  }
  AudioBuffer buffer;
  buffer.channels = static_cast<i32>(channels);
  buffer.sampleRate = static_cast<i32>(sampleRate);
  const usize sampleCount = static_cast<usize>(totalFrames) * static_cast<usize>(channels);
  buffer.samples.assign(decoded, decoded + sampleCount);
  drflac_free(decoded, nullptr);
  buffer.frameCount = totalFrames;
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
  if (endsWithIgnoreCase(path, ".ogg")) return loadOGG(path, error);
  if (endsWithIgnoreCase(path, ".flac")) return loadFLAC(path, error);
  error = "unsupported audio format (expected .wav, .mp3, .ogg or .flac): " + path;
  return std::nullopt;
}

std::optional<AudioBuffer> AudioBuffer::loadWAV(const std::string& path, std::string& error) { return fromWAV(path, error); }
std::optional<AudioBuffer> AudioBuffer::loadMP3(const std::string& path, std::string& error) { return fromMP3(path, error); }
std::optional<AudioBuffer> AudioBuffer::loadOGG(const std::string& path, std::string& error) { return fromOGG(path, error); }
std::optional<AudioBuffer> AudioBuffer::loadFLAC(const std::string& path, std::string& error) { return fromFLAC(path, error); }

std::vector<u8> AudioBuffer::encodeWAV() const {
  std::vector<u8> out;
  if (isEmpty()) return out;
  const u32 dataBytes = static_cast<u32>(samples.size() * 2U);
  const u32 byteRate = static_cast<u32>(sampleRate) * static_cast<u32>(channels) * 2U;
  const u16 blockAlign = static_cast<u16>(channels * 2);
  auto put16 = [&out](u16 value) {
    out.push_back(static_cast<u8>(value & 0xFFU));
    out.push_back(static_cast<u8>((value >> 8U) & 0xFFU));
  };
  auto put32 = [&out](u32 value) {
    out.push_back(static_cast<u8>(value & 0xFFU));
    out.push_back(static_cast<u8>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<u8>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<u8>((value >> 24U) & 0xFFU));
  };
  auto putTag = [&out](const char* tag) {
    for (usize i = 0; i < 4U; ++i) out.push_back(static_cast<u8>(tag[i]));
  };
  out.reserve(44U + dataBytes);
  putTag("RIFF");
  put32(36U + dataBytes);
  putTag("WAVE");
  putTag("fmt ");
  put32(16U);
  put16(1U);  // PCM
  put16(static_cast<u16>(channels));
  put32(static_cast<u32>(sampleRate));
  put32(byteRate);
  put16(blockAlign);
  put16(16U);
  putTag("data");
  put32(dataBytes);
  // The same quantization as dr_wav's f32 -> s16 (so the bytes equal writeWAV's).
  for (const f32 sample : samples) {
    i16 pcm = 0;
    if (sample != sample) {
      pcm = 0;  // NaN
    } else if (sample <= -1.0f) {
      pcm = -32768;
    } else if (sample >= 1.0f) {
      pcm = 32767;
    } else {
      pcm = static_cast<i16>(sample * 32768.0f);
    }
    put16(static_cast<u16>(pcm));
  }
  return out;
}

AudioBuffer AudioBuffer::tone(f64 frequency, f64 seconds, f64 amplitude, f64 endFrequency, i32 sampleRateHz) {
  AudioBuffer out;
  if (frequency <= 0.0 || seconds <= 0.0 || sampleRateHz <= 0) return out;
  out.channels = 1;
  out.sampleRate = sampleRateHz;
  out.frameCount = static_cast<u64>(seconds * static_cast<f64>(sampleRateHz));
  out.samples.resize(static_cast<usize>(out.frameCount));
  const f64 target = endFrequency > 0.0 ? endFrequency : frequency;
  const f64 twoPi = 6.283185307179586;
  f64 phase = 0.0;
  for (usize i = 0; i < out.samples.size(); ++i) {
    const f64 t = static_cast<f64>(i) / static_cast<f64>(sampleRateHz);
    const f64 progress = t / seconds;
    const f64 hz = frequency + (target - frequency) * progress;
    phase += twoPi * hz / static_cast<f64>(sampleRateHz);
    const f64 envelope = std::exp(-5.0 * progress);  // -43 dB by the end
    out.samples[i] = static_cast<f32>(std::sin(phase) * amplitude * envelope);
  }
  return out;
}

AudioBuffer AudioBuffer::thock(f64 seconds, f64 cutoffHz, f64 amplitude, i32 sampleRateHz) {
  AudioBuffer out;
  if (seconds <= 0.0 || sampleRateHz <= 0) return out;
  out.channels = 1;
  out.sampleRate = sampleRateHz;
  out.frameCount = static_cast<u64>(seconds * static_cast<f64>(sampleRateHz));
  out.samples.resize(static_cast<usize>(out.frameCount));
  // Deterministic noise (LCG) so the cue is the same on every machine.
  u32 state = 0x9E3779B9U;
  const f64 rc = 1.0 / (6.283185307179586 * std::max(1.0, cutoffHz));
  const f64 dt = 1.0 / static_cast<f64>(sampleRateHz);
  const f64 alpha = dt / (rc + dt);
  f64 filtered = 0.0;
  for (usize i = 0; i < out.samples.size(); ++i) {
    state = state * 1664525U + 1013904223U;
    const f64 noise = static_cast<f64>(state >> 8U) / 8388608.0 - 1.0;  // [-1, 1)
    filtered += alpha * (noise - filtered);
    const f64 progress = static_cast<f64>(i) / static_cast<f64>(out.samples.size());
    const f64 envelope = std::exp(-9.0 * progress);
    out.samples[i] = static_cast<f32>(filtered * amplitude * 3.0 * envelope);
  }
  return out;
}

AudioBuffer AudioBuffer::concat(const AudioBuffer& first, const AudioBuffer& second) {
  if (first.isEmpty()) return second;
  if (second.isEmpty() || second.channels != first.channels || second.sampleRate != first.sampleRate) return first;
  AudioBuffer out = first;
  out.samples.insert(out.samples.end(), second.samples.begin(), second.samples.end());
  out.frameCount = first.frameCount + second.frameCount;
  return out;
}

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

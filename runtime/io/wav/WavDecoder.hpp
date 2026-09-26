// WavDecoder.hpp — reference AudioDecoder for RIFF/WAVE (ADR-0050).
// Reads PCM 8/16/24/32-bit integer and 32-bit IEEE float, any channel count,
// and downmixes to mono. A backend, not SDK API: it lives under runtime/io and
// is injected by the application (SoundSystem::setAudioDecoder).
#ifndef X3D_RUNTIME_IO_WAV_DECODER_HPP
#define X3D_RUNTIME_IO_WAV_DECODER_HPP

#include "AudioDecoder.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace x3d::runtime::io::wav {

namespace detail {
inline std::uint32_t le32(const std::uint8_t *p) {
  return std::uint32_t(p[0]) | std::uint32_t(p[1]) << 8 | std::uint32_t(p[2]) << 16 |
         std::uint32_t(p[3]) << 24;
}
inline std::uint16_t le16(const std::uint8_t *p) {
  return static_cast<std::uint16_t>(p[0] | p[1] << 8);
}
} // namespace detail

inline DecodedAudio decodeWav(const std::vector<std::uint8_t> &b) {
  using namespace detail;
  DecodedAudio out;
  if (b.size() < 12 || std::memcmp(b.data(), "RIFF", 4) != 0 ||
      std::memcmp(b.data() + 8, "WAVE", 4) != 0)
    return out;
  std::uint16_t format = 0, channels = 0, bits = 0;
  std::uint32_t rate = 0;
  const std::uint8_t *data = nullptr;
  std::size_t dataSize = 0;
  for (std::size_t pos = 12; pos + 8 <= b.size();) {
    const std::uint32_t size = le32(b.data() + pos + 4);
    const std::size_t body = pos + 8;
    if (size > b.size() - body) break; // truncated chunk
    if (std::memcmp(b.data() + pos, "fmt ", 4) == 0 && size >= 16) {
      format = le16(b.data() + body);
      channels = le16(b.data() + body + 2);
      rate = le32(b.data() + body + 4);
      bits = le16(b.data() + body + 14);
      if (format == 0xFFFE && size >= 26) format = le16(b.data() + body + 24); // extensible
    } else if (std::memcmp(b.data() + pos, "data", 4) == 0) {
      data = b.data() + body;
      dataSize = size;
    }
    pos = body + size + (size & 1u); // chunks are word-aligned
  }
  const bool pcm = format == 1 && (bits == 8 || bits == 16 || bits == 24 || bits == 32);
  const bool flt = format == 3 && bits == 32;
  if (!data || channels == 0 || rate == 0 || !(pcm || flt)) return out;
  const std::size_t bytesPer = bits / 8u;
  const std::size_t frames = dataSize / (bytesPer * channels);
  out.samples.resize(frames);
  for (std::size_t f = 0; f < frames; ++f) {
    double sum = 0.0;
    for (std::size_t c = 0; c < channels; ++c) {
      const std::uint8_t *s = data + (f * channels + c) * bytesPer;
      double v = 0.0;
      if (flt) {
        float x;
        std::memcpy(&x, s, 4);
        v = x;
      } else if (bits == 8) {
        v = (double(s[0]) - 128.0) / 128.0; // 8-bit WAV is unsigned
      } else if (bits == 16) {
        v = double(static_cast<std::int16_t>(le16(s))) / 32768.0;
      } else if (bits == 24) {
        std::int32_t x = std::int32_t(s[0]) | std::int32_t(s[1]) << 8 | std::int32_t(s[2]) << 16;
        if (x & 0x800000) x -= 0x1000000;
        v = double(x) / 8388608.0;
      } else {
        v = double(static_cast<std::int32_t>(le32(s))) / 2147483648.0;
      }
      sum += v;
    }
    out.samples[f] = static_cast<float>(sum / channels);
  }
  out.sampleRate = static_cast<float>(rate);
  out.ok = true;
  return out;
}

/// The reference decoder as an AudioDecoder for SoundSystem::setAudioDecoder.
inline AudioDecoder makeWavDecoder() { return [](const std::vector<std::uint8_t> &b) { return decodeWav(b); }; }

} // namespace x3d::runtime::io::wav

#endif // X3D_RUNTIME_IO_WAV_DECODER_HPP

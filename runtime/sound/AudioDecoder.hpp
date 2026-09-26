// AudioDecoder.hpp — the audio decode seam (ADR-0050).
// An AudioClip's bytes (fetched through the AssetResolver seam) become mono PCM
// here, app-side, before crossing the AudioBackend seam once as a Buffer node.
// IO-free: the SDK ships only the null default; a concrete decoder (e.g. the
// reference WAV reader in runtime/io/wav, or a full codec library) is injected
// from the application layer, like the texture and movie decoders.
#ifndef X3D_RUNTIME_AUDIO_DECODER_HPP
#define X3D_RUNTIME_AUDIO_DECODER_HPP

#include <cstdint>
#include <functional>
#include <vector>

namespace x3d::runtime {

/// Decoded audio: mono samples in [-1, 1] at `sampleRate`. `ok` false on
/// unsupported or malformed input.
struct DecodedAudio {
  bool ok = false;
  std::vector<float> samples;
  float sampleRate = 0.0f;
  double duration() const {
    return ok && sampleRate > 0.0f ? static_cast<double>(samples.size()) / sampleRate : -1.0;
  }
};

using AudioDecoder = std::function<DecodedAudio(const std::vector<std::uint8_t> &bytes)>;

/// The IO-free default: decodes nothing (every AudioClip stays silent).
inline AudioDecoder makeNullAudioDecoder() {
  return [](const std::vector<std::uint8_t> &) { return DecodedAudio{}; };
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_AUDIO_DECODER_HPP

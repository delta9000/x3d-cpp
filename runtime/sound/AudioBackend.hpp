// ISO/IEC 19775-1 §16 audio graph interface. See ADR-0026.
// DSP implementation and engine types remain private to each backend.
// Panners receive positions rather than channel gains so each backend chooses
// its attenuation model. Decoded PCM is supplied at Buffer creation; playback
// state and rate are updated separately. See ADR-0050.
#ifndef X3D_RUNTIME_AUDIO_BACKEND_HPP
#define X3D_RUNTIME_AUDIO_BACKEND_HPP

#include <cstdint>
#include <vector>

namespace x3d::runtime {

/**
 * @brief Opaque handle a backend returns from createNode() to identify one §16
 *        audio-graph node. Zero is the invalid handle.
 * @details The runtime never interprets the bits; it only passes a handle back
 *          to the same backend that minted it (to connect()/setParam()/render()).
 */
using NodeHandle = std::uint64_t;

inline constexpr NodeHandle kInvalidNodeHandle = 0;

enum class NodeKind { Oscillator, Biquad, Gain, Destination, Panner, Buffer,
                      Delay,
                      Compressor,
                      WaveShaper };

/**
 * @brief Distance attenuation model for Panner nodes.
 * @details Mirrors the X3D 4.0 §16 DistanceModel choices. The backend applies
 *          the model using the positions from NodeParams; no precomputed gain
 *          may cross the seam. See the seam-purity note at the top of this file.
 *   - Linear:      gain = 1 - rolloffFactor * (d - refDist) / (maxDist - refDist)
 *   - Inverse:     gain = refDist / (refDist + rolloffFactor * (d - refDist))
 *   - Exponential: gain = (d / refDist)^(-rolloffFactor)
 *   - Ellipsoid:   the §16.4.17 Sound node model. Two ellipsoids with one focus
 *                  at the source, oriented along `direction`: full level inside
 *                  the inner (minFront/minBack), falling linearly in dB to
 *                  -20 dB at the outer (maxFront/maxBack), silent outside;
 *                  scaled by `intensity`; panned only when `spatialize`.
 */
enum class DistanceModel { Linear, Inverse, Exponential, Ellipsoid };

/**
 * @brief Oscillator waveform, in the runtime's own terms (mirrors X3D's
 *        PeriodicWaveTypeChoices). Custom = the harmonic terms carried in
 *        NodeParams::optionsReal/optionsImag (a §16.4.18 PeriodicWave whose
 *        type is CUSTOM). The seam never names a DSP-engine waveform.
 */
enum class Waveform { Sine, Square, Sawtooth, Triangle, Custom };

/**
 * @brief Biquad filter algorithm, in the runtime's own terms (mirrors X3D's
 *        BiquadTypeFilterChoices). The seam never names a DSP-engine filter type.
 */
enum class FilterType { Lowpass, Highpass, Bandpass, Lowshelf, Highshelf,
                        Peaking, Notch, Allpass };

/**
 * @brief A §16 node's initial parameters, described in the runtime's own terms.
 * @details A small tagged struct so the seam never names an engine param type.
 *          SoundSystem fills the fields relevant to the NodeKind from the §16
 *          node's accessors:
 *            - Oscillator:  frequency, detune (cents), gain, waveform
 *                           (optionsReal/optionsImag harmonic terms for
 *                           Waveform::Custom).
 *            - Biquad:      frequency (cutoff), q (qualityFactor), detune, gain,
 *                           filterType.
 *            - Gain:        gain.
 *            - Destination: maxChannelCount.
 *            - Buffer:      samples, sampleRate (PCM crosses once, ADR-0050).
 *            - Delay:       delayTime, maxDelayTime, enabled (a pure delay).
 *            - Compressor:  threshold, knee (dB), ratio, attack, release
 *                           (seconds), gain, enabled.
 *            - WaveShaper:  curve (the transfer curve — like Buffer's samples,
 *                           it crosses once, at createNode), gain, enabled.
 *            - Panner:      sourcePosition, listenerPosition, listenerForward,
 *                           listenerUp, distanceModel, referenceDistance,
 *                           maxDistance, rolloffFactor. positions cross the seam
 *                           — the backend computes pan/attenuation from geometry.
 *                           A precomputed gainL/gainR/coefficient must not cross
 *                           the seam (see seam-purity note at top of file).
 *          Unused fields keep their defaults. Only SF* scalars + bounded enums
 *          cross the seam — never any DSP state.
 */
struct NodeParams {
  /** @brief Oscillator/biquad frequency in hertz. */
  float frequency = 440.0f;
  /** @brief Detune in cents (frequency *= 2^(detune/1200)). */
  float detune = 0.0f;
  /** @brief Quality factor (Q) for the biquad. */
  float q = 1.0f;
  /** @brief Linear gain multiplier (Gain node, or a source/processing gain). */
  float gain = 1.0f;
  /** @brief Whether this source or processing node is enabled. */
  bool enabled = true;
  /** @brief Oscillator waveform (Oscillator nodes). */
  Waveform waveform = Waveform::Sine;

  // Custom-waveform fields (Waveform::Custom, the §16.4.18 PeriodicWave)
  //
  // The real/imag harmonic terms cross once, here, like a Buffer's samples —
  // they are waveform data, not DSP state. Index 0 is the DC term (ignored);
  // harmonic k sums real[k]*cos(k*phase) + imag[k]*sin(k*phase), normalized so
  // the peak absolute value is 1 (Web Audio's default; X3D 4.0's PeriodicWave
  // exposes no disableNormalization toggle). Extra elements of the longer
  // array are ignored; arrays with no harmonics at all are the sine default.
  /** @brief Real (cosine) harmonic terms (Oscillator nodes). */
  std::vector<float> optionsReal;
  /** @brief Imaginary (sine) harmonic terms (Oscillator nodes). */
  std::vector<float> optionsImag;
  /** @brief Filter algorithm (Biquad nodes). */
  FilterType filterType = FilterType::Lowpass;
  /** @brief Max channel count (Destination nodes). */
  int maxChannelCount = 2;
  /** @brief Delay duration in seconds (Delay nodes). Clamped to [0, maxDelayTime]. */
  float delayTime = 0.0f;
  /** @brief Maximum delay in seconds (Delay nodes) — bounds the ring buffer. */
  float maxDelayTime = 1.0f;

  // WaveShaper field (like Buffer's samples, the curve crosses once, here) ─
  /** @brief Transfer curve (WaveShaper nodes): input x in [-1,1] maps to index
   *         (N-1)/2*(x+1) with linear interpolation; out-of-range x clamps to
   *         the first/last value; empty = pass-through. */
  std::vector<float> curve;

  // Panner-only fields (unused / defaulted for all other node kinds)
  //
  // positions cross the seam; the backend derives pan/attenuation from geometry.
  // never pass a precomputed gainL/gainR/coefficient here — that would violate
  // the seam-purity contract and couple the runtime to a specific pan law.

  /** @brief Sound source position in world space (Panner nodes). */
  float sourcePosition[3]   = {0.0f, 0.0f, 0.0f};
  /** @brief Listener position in world space (Panner nodes). */
  float listenerPosition[3] = {0.0f, 0.0f, 0.0f};
  /** @brief Listener forward unit vector (Panner nodes). Default: -Z. */
  float listenerForward[3]  = {0.0f, 0.0f, -1.0f};
  /** @brief Listener up unit vector (Panner nodes). Default: +Y. */
  float listenerUp[3]       = {0.0f, 1.0f, 0.0f};
  /** @brief Distance attenuation model (Panner nodes). */
  DistanceModel distanceModel  = DistanceModel::Inverse;
  /** @brief Reference distance at which gain = 1.0 (Panner nodes). */
  float referenceDistance      = 1.0f;
  /** @brief Maximum distance beyond which attenuation stops (Panner nodes). */
  float maxDistance            = 10000.0f;
  /** @brief Rolloff rate multiplier (Panner nodes). */
  float rolloffFactor          = 1.0f;

  // Ellipsoid Panner fields (DistanceModel::Ellipsoid, the §16.4.17 Sound
  // node). Geometry only: the backend computes the attenuation.
  /** @brief Sound direction (unit-less; the ellipsoids' axis), world space. */
  float direction[3] = {0.0f, 0.0f, 1.0f};
  float minFront = 1.0f, minBack = 1.0f, maxFront = 10.0f, maxBack = 10.0f;
  /** @brief Sound.intensity: a linear level scale in [0, 1]. */
  float intensity = 1.0f;
  /** @brief Sound.spatialize: pan by direction to the listener, else centred. */
  bool spatialize = true;

  // Buffer source fields (NodeKind::Buffer). The samples cross once, here. ─
  /** @brief Decoded mono PCM in [-1, 1]. */
  std::vector<float> samples;
  /** @brief The samples' own rate in hertz. */
  float sampleRate = 44100.0f;

  // DynamicsCompressor fields (NodeKind::Compressor, §16.4.9)
  /** @brief Compression threshold in dB (DynamicsCompressor). */
  float threshold = -24.0f;
  /** @brief Soft-knee width in dB (DynamicsCompressor). */
  float knee = 30.0f;
  /** @brief Compression ratio applied above the threshold. */
  float ratio = 12.0f;
  /** @brief Attack time in seconds (DynamicsCompressor). */
  float attack = 0.003f;
  /** @brief Release time in seconds (DynamicsCompressor). */
  float release = 0.25f;
};

/**
 * @brief Which scalar a setParam() call updates (for route-animated fields).
 * @details SoundSystem reads a node's (possibly cascade-animated) field each
 *          tick and pushes it as one of these. The set mirrors NodeParams'
 *          animatable scalars.
 *          PositionX/Y/Z are Panner-specific: each tick the source position is
 *          pushed as three separate setParam calls so route-animated motion works
 *          without expanding the seam to SFVec3f.
 */
enum class Param {
  Frequency, Detune, Q, Gain, PositionX, PositionY, PositionZ,
  // Ellipsoid Panner (per tick): the listener is the moving viewer, and the
  // Sound's direction / intensity may be route-animated.
  ListenerPositionX, ListenerPositionY, ListenerPositionZ,
  ListenerForwardX, ListenerForwardY, ListenerForwardZ,
  ListenerUpX, ListenerUpY, ListenerUpZ,
  DirectionX, DirectionY, DirectionZ, Intensity,
  // Buffer source: 0 = stopped (rewinds), 1 = playing, 2 = paused (holds the
  // position); rate = playback speed (AudioClip.pitch). The source loops when
  // it runs past its end — the time lifecycle stops a non-looping clip.
  PlaybackState, PlaybackRate, Enabled,
  // Delay node: the delay duration and its maximum (the ring-buffer bound).
  DelayTime, MaxDelayTime,
  // DynamicsCompressor (§16.4.9): threshold/knee in dB, ratio, attack/release
  // in seconds.
  Threshold, Knee, Ratio, Attack, Release
};

/**
 * @brief Abstract audio backend: owns an audio-processing graph, renders it.
 * @details One backend instance owns many nodes, each addressed by a NodeHandle,
 *          wired by connect() into a directed graph that flows inputs -> output.
 *          The lifecycle mirrors the §16 graph SoundSystem reads:
 *            - createNode(): one per §16 node (OscillatorSource / BiquadFilter /
 *              Gain / AudioDestination), with its initial NodeParams.
 *            - connect(dst, src): wire `src` as an input feeding into `dst`
 *              (matching §16 `children` = inputs; the graph flows children ->
 *              parent up to the destination).
 *            - setParam(): update one animatable scalar (route-animation each
 *              tick).
 *            - render(): pull `frames` mono samples from `destination` at
 *              `sampleRate` into `out`. The output target is the backend's
 *              concern (a buffer here; a device / network in a real backend).
 *
 *          The backend owns all engine/DSP state internally and tears it down in
 *          its destructor. The runtime owns the backend's lifetime via a
 *          shared_ptr held by SoundSystem.
 */
class AudioBackend {
public:
  virtual ~AudioBackend() = default;

  /**
   * @brief Create one audio-graph node of the given kind with initial params.
   * @return A non-zero NodeHandle on success, kInvalidNodeHandle on failure.
   */
  virtual NodeHandle createNode(NodeKind kind, const NodeParams &params) = 0;

  /**
   * @brief Wire `src` as an input feeding into `dst` (§16 children = inputs).
   * @details Both handles must already have been created. A node may have many
   *          inputs (they sum). The destination's inputs are its `children`.
   */
  virtual void connect(NodeHandle dst, NodeHandle src) = 0;

  /**
   * @brief Update one animatable scalar on a node (route-animation each tick).
   */
  virtual void setParam(NodeHandle node, Param param, float value) = 0;

  /**
   * @brief Render `frames` mono samples pulled through the graph from
   *        `destination`, at `sampleRate` Hz, into `out` (resized to `frames`).
   * @details A fixed graph + fixed params + fixed sampleRate must be
   *          deterministic so a rendered buffer is byte-reproducible.
   */
  virtual void render(NodeHandle destination, int frames, float sampleRate,
                      std::vector<float> &out) = 0;

  /**
   * @brief Render `frames` stereo sample-pairs (interleaved L,R,L,R,…) from
   *        `destination` into `outLR` (resized to `2*frames`).
   * @details Backends without spatial panning duplicate mono into both channels.
   *
   *          outLR[2*i]   = left  sample for frame i
   *          outLR[2*i+1] = right sample for frame i
   */
  virtual void renderStereo(NodeHandle destination, int frames, float sampleRate,
                             std::vector<float> &outLR) = 0;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_AUDIO_BACKEND_HPP

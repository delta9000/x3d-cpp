// AudioBackend.hpp  [STABLE]
// The engine-agnostic audio seam (ISO/IEC 19775-1 §16 Sound: the X3D 4.0 sound
// component models a Web-Audio-style directed processing graph but mandates no
// specific DSP engine). A backend (a built-in DSP, miniaudio, LabSound, or a
// browser's Web Audio) implements this interface; SoundSystem drives it by
// reading the §16 `children` audio graph.
//
// STATUS: [STABLE] — proven generic by two independent backends:
//   Backend A: BuiltinDspBackend (runtime/sound/dsp/BuiltinDspBackend.{hpp,cpp},
//              always-built, dependency-free)
//   Backend B: MiniaudioBackend (runtime/sound/miniaudio/MiniaudioBackend.{hpp,cpp},
//              -DX3D_CPP_BUILD_MINIAUDIO=ON, vendored single-header,
//              Unlicense OR MIT-0)
// The CI-gated headless swap-test `x3d_sound_swaptest` asserts both backends
// agree on the synthesis chain (RMS ±2%, Goertzel ±5%) and on spatial
// invariants (ear-sign, symmetry, monotonic distance falloff). See ADR-0026.
//
// ENGINE-AGNOSTIC CONTRACT (mirrors the PhysicsBackend seam, §37, and the
// ScriptEngine seam, §29.1): this seam carries NO DSP-engine types and NO DSP
// implementation. Every value crossing it is a node KIND, a small set of SF*
// scalar parameters + bounded waveform/filter enums, or a plain sample buffer
// (std::vector<float>). There is no oscillator math, no biquad coefficient, no
// phase accumulator, no filter state in this header — all DSP lives ONLY in a
// backend's .cpp (dsp/BuiltinDspBackend.cpp). A seam-purity check greps this
// header (and SoundSystem.hpp) for DSP leakage: there must be none.
//
// Spatial-audio purity extension: POSITIONS (source, listener orientation) may
// cross the seam for Panner nodes — the backend computes pan/attenuation from
// the geometry itself. What MUST NEVER cross the seam: gainL, gainR, any
// precomputed pan coefficient, biquad DSP coefficients, or PCM buffer data.
// Crossing a precomputed gain instead of a position would couple the runtime to
// the backend's distance model and break backend-swappability.
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

/** @brief The invalid / unset node handle. */
inline constexpr NodeHandle kInvalidNodeHandle = 0;

/**
 * @brief Which §16 audio-graph node a backend should instantiate.
 * @details The seam speaks node KINDS, never an engine's node class. v1 covers
 *          the synthesis chain (OscillatorSource -> BiquadFilter -> Gain ->
 *          AudioDestination); extensible (add a Kind + its NodeParams) without
 *          engine-type leakage.
 */
enum class NodeKind { Oscillator, Biquad, Gain, Destination, Panner };

/**
 * @brief Distance attenuation model for Panner nodes.
 * @details Mirrors the X3D 4.0 §16 DistanceModel choices. The backend applies
 *          the model using the positions from NodeParams; no precomputed gain
 *          may cross the seam. See the seam-purity note at the top of this file.
 *   - Linear:      gain = 1 - rolloffFactor * (d - refDist) / (maxDist - refDist)
 *   - Inverse:     gain = refDist / (refDist + rolloffFactor * (d - refDist))
 *   - Exponential: gain = (d / refDist)^(-rolloffFactor)
 */
enum class DistanceModel { Linear, Inverse, Exponential };

/**
 * @brief Oscillator waveform, in the runtime's own terms (mirrors X3D's
 *        PeriodicWaveTypeChoices). The seam never names a DSP-engine waveform.
 */
enum class Waveform { Sine, Square, Sawtooth, Triangle };

/**
 * @brief Biquad filter algorithm, in the runtime's own terms (mirrors X3D's
 *        BiquadTypeFilterChoices). The seam never names a DSP-engine filter type.
 *        v1 implements at least Lowpass + Highpass; the rest are extension points.
 */
enum class FilterType { Lowpass, Highpass, Bandpass, Lowshelf, Highshelf,
                        Peaking, Notch, Allpass };

/**
 * @brief A §16 node's initial parameters, described in the runtime's own terms.
 * @details A small tagged struct so the seam never names an engine param type.
 *          SoundSystem fills the fields relevant to the NodeKind from the §16
 *          node's accessors:
 *            - Oscillator:  frequency, detune (cents), gain, waveform.
 *            - Biquad:      frequency (cutoff), q (qualityFactor), detune, gain,
 *                           filterType.
 *            - Gain:        gain.
 *            - Destination: maxChannelCount.
 *            - Panner:      sourcePosition, listenerPosition, listenerForward,
 *                           listenerUp, distanceModel, referenceDistance,
 *                           maxDistance, rolloffFactor. POSITIONS cross the seam
 *                           — the backend computes pan/attenuation from geometry.
 *                           A precomputed gainL/gainR/coefficient MUST NOT cross
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
  /** @brief Oscillator waveform (Oscillator nodes). */
  Waveform waveform = Waveform::Sine;
  /** @brief Filter algorithm (Biquad nodes). */
  FilterType filterType = FilterType::Lowpass;
  /** @brief Max channel count (Destination nodes). v1 is mono. */
  int maxChannelCount = 2;

  // ── Panner-only fields (unused / defaulted for all other node kinds) ──────
  //
  // POSITIONS cross the seam; the backend derives pan/attenuation from geometry.
  // NEVER pass a precomputed gainL/gainR/coefficient here — that would violate
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
enum class Param { Frequency, Detune, Q, Gain, PositionX, PositionY, PositionZ };

/**
 * @brief Abstract audio backend: owns an audio-processing graph, renders it.
 * @details One backend instance owns many nodes, each addressed by a NodeHandle,
 *          wired by connect() into a directed graph that flows inputs -> output.
 *          The lifecycle mirrors the §16 graph SoundSystem reads:
 *            - createNode(): one per §16 node (OscillatorSource / BiquadFilter /
 *              Gain / AudioDestination), with its initial NodeParams.
 *            - connect(dst, src): wire `src` as an INPUT feeding INTO `dst`
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
   * @brief Wire `src` as an input feeding INTO `dst` (§16 children = inputs).
   * @details Both handles MUST already have been created. A node may have many
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
   * @details Backends that do not yet implement spatial panning (Task 3+) MUST
   *          still provide this: duplicate the mono signal into both channels.
   *          Equal-power panning for Panner nodes is added in Task 3
   *          (BuiltinDspBackend) and Task 4 (MiniaudioBackend).
   *
   *          outLR[2*i]   = left  sample for frame i
   *          outLR[2*i+1] = right sample for frame i
   */
  virtual void renderStereo(NodeHandle destination, int frames, float sampleRate,
                             std::vector<float> &outLR) = 0;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_AUDIO_BACKEND_HPP

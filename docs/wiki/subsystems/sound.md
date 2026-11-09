---
title: "Sound (§16 audio graph)"
summary: X3D Sound through the AudioBackend interface, SoundSystem, and dependency-free BuiltinDspBackend.
tags: [subsystem, sound, seam, audio, genericity, swap-test]
updated: 2026-09-29
related:
  - ../architecture.md
  - ../decisions/0020-sound-seam.md
  - ../decisions/0026-audiobackend-second-backend-swap-test.md
  - ../decisions/0050-audio-pcm-and-ellipsoid-seam.md
  - ../seam-status.md
  - ../decisions/0019-physics-seam.md
  - ../subsystems/physics.md
  - ../subsystems/event-cascade.md
  - ../subsystems/execution-context.md
---

# Sound (§16 audio graph)

## Purpose

Animates the X3D §16 Sound component nodes (`OscillatorSource`, `BiquadFilter`, `Gain`, `Delay`, `DynamicsCompressor`, `WaveShaper`, `AudioDestination`, `SpatialSound`, `ListenerPointSource`, the classic `Sound` + `AudioClip` pair of the Immersive profile, and `BufferAudioSource`) by building and driving an audio-processing graph each tick. It owns the boundary between the X3D §16 declarative node graph and a DSP engine, using the same seam pattern as [Physics](physics.md) (`PhysicsBackend`→Jolt) and [Script/SAI](system-script-sai.md) (`ScriptEngine`→Duktape): the runtime defines an engine-agnostic contract; a backend implements DSP. Unlike those two seams the built-in DSP backend carries no external dependency, so it ships in the default build with no flag-gate.

`BuiltinDspBackend` supplies the DSP implementation. The host supplies
decoded PCM and chooses the output path.

The seam carries positions (source + listener) for `Panner` nodes — the backend computes pan and distance attenuation from the geometry itself. Precomputed gains never cross the seam (seam-purity contract in `AudioBackend.hpp`). `SoundSystem` does only SDK-side plumbing: it reads the X3D `SFRotation` orientation, applies Rodrigues' rotation to derive `listenerForward` and `listenerUp` vectors, and pushes positions into `NodeParams`. All spatial DSP (azimuth, distance model, pan law) runs exclusively in the backend.

The classic `Sound` node and its `AudioClip` source ride two seam additions ([ADR-0050](../decisions/0050-audio-pcm-and-ellipsoid-seam.md)). Decoded PCM crosses once, as a `Buffer` node. The `Sound` ellipsoid crosses as geometry (`DistanceModel::Ellipsoid`), and each backend evaluates the §16.4.17 falloff itself.

## Key files

| File / directory | Role |
|---|---|
| `runtime/sound/AudioBackend.hpp` | abstract seam. Pure-virtual; opaque `NodeHandle` (`uint64_t`); `NodeKind` enum (`Oscillator`/`Biquad`/`Gain`/`Destination`/`Panner`/`Buffer`/`Delay`/`Compressor`/`WaveShaper`); `NodeParams` (small tagged struct of `SF*` scalars + `Waveform`/`FilterType` enums + Panner position/orientation/distance fields + `delayTime`/`maxDelayTime` + compressor `threshold`/`knee`/`ratio`/`attack`/`release` + the WaveShaper `curve`, which crosses once like a Buffer's samples); `Param` enum (`Frequency`/`Detune`/`Q`/`Gain`/`PositionX`/`PositionY`/`PositionZ`/`DelayTime`/`MaxDelayTime`/`Threshold`/`Knee`/`Ratio`/`Attack`/`Release`); `DistanceModel` enum (`Linear`/`Inverse`/`Exponential`); `createNode`/`connect`/`setParam`/`render`/`renderStereo()`. Contains zero DSP-engine types — every value is the runtime's own SF* scalar or a bounded enum. Seam carries POSITIONS only for Panner nodes — precomputed gains must not cross it. |
| `runtime/sound/SoundSystem.hpp` | `System`. Reads the §16 `children` graph bottom-up into backend nodes and connections. Constructed with a `shared_ptr<AudioBackend>` — inert if none (exactly like `PhysicsSystem` without a backend). `attach()` accepts `AudioDestination` and `SpatialSound`; `setListener()` registers the `ListenerPointSource`; `setPeriodicWave()` registers a §16.4.18 `PeriodicWave` for one `OscillatorSource` (the 4.0 bindings cannot read the authored field —); `buildPannerParams()` resolves SFRotation→forward/up (SDK plumbing — no spatial DSP). `renderStereo()` delegates to the backend's stereo path. |
| `runtime/sound/dsp/BuiltinDspBackend.hpp` | (no flag needed). Backend A — default `AudioBackend` implementation — header is DSP-free (pImpl); all DSP math lives in `BuiltinDspBackend.cpp`. |
| `runtime/sound/dsp/BuiltinDspBackend.cpp` | All audio math: phase-accumulator oscillators (all four built-in waveforms + §16.4.18 `PeriodicWave` harmonic sums — real/imag terms, DC ignored, peak-normalized to 1), RBJ "Audio EQ Cookbook" biquad coefficients (Lowpass/Highpass/Bandpass/Notch/Allpass full, Lowshelf/Highshelf/Peaking pass-through pending a dB-gain param), per-sample Direct Form I difference equation, gain multiply, the §16.4.9 feed-forward compressor (dB-domain soft-knee gain computer, one-pole attack/release smoothing), destination summing, ring-buffer delay (Delay nodes: `delayTime` clamped to `[0, maxDelayTime]`, `enabled=false` passes through), wave-shaping (WaveShaper nodes: input mapped through the transfer curve with linear interpolation; out-of-range input clamps to the end values; empty curve / `enabled=false` pass through; `oversample` is NONE-semantics only). Topological pull render. Spatial path: Panner nodes compute distance `d = |src−listener|`, distance gain via the three `DistanceModel` closed forms, azimuth from `dot(normalize(src−listener), normalize(cross(forward,up)))`, equal-power pan `θ=(az_norm·0.5+0.5)·(π/2)`, `gL=cos θ`, `gR=sin θ`. `renderStereo()` does a stereo-aware sum at the destination (Panner blocks are interleaved L/R; other inputs are mono-duplicated). |
| `runtime/sound/RecordingBackend.hpp` | Test backend that records every `createNode`/`connect`/`setParam` call (no DSP, `render` returns silence). Proves graph construction is correct + deterministic in isolation. |
| `runtime/sound/tests/sound_system_test.cpp` | Five-tier test: (a) recording-backend graph construction, (b) DSP sample assertions, (c) param animation, (d) render determinism, (f) spatial DSP — hard-left/hard-right/centered equal-power pan + distance falloff. |
| `runtime/sound/tests/sound_dynamics_compressor_test.cpp` | §16.4.9 DynamicsCompressor: recording-tier graph wiring through `SoundSystem` + DSP tier on the built-in backend — below-threshold unity, settled above-threshold ratio law, `enabled=false` pass-through, lifecycle-activated end-to-end sine. |
| `runtime/sound/AudioDecoder.hpp` | function seam: `AudioDecoder` maps fetched bytes to `DecodedAudio` (mono `float` samples, sample rate). The public default `makeNullAudioDecoder()` decodes nothing (IO-free SDK). |
| `runtime/io/wav/WavDecoder.hpp` | Reference decoder (header-only, not installed): RIFF/WAVE PCM 8/16/24/32-bit, IEEE float32, `WAVE_FORMAT_EXTENSIBLE`; multi-channel is downmixed to mono. `makeWavDecoder()` for `SoundSystem::setAudioDecoder`. |
| `runtime/sound/tests/immersive_fixtures.hpp` | Backend-level ellipsoid and Buffer fixtures, run on the built-in backend by `sound_immersive_test`. |
| `runtime/sound/tests/sound_immersive_test.cpp` | Built-in backend: the shared fixtures, the WAV decoder, a `Sound{AudioClip}` scene through `SoundSystem` (url fallback, isActive/isPaused playback, `Sound.location` through the ellipsoid), and `BufferAudioSource` scenes (authored PCM → Buffer; MediaTimeSystem lifecycle, loop, gain, stereo downmix, activation rate capture). |
| `runtime/sound/tests/sound_delay_test.cpp` | Built-in backend `Delay` node: recording-tier wiring (`delayTime`/`maxDelayTime`/`enabled` read + pushed) and DSP-tier impulse assertions — impulse in arrives one impulse `delayTime` later, `delayTime > maxDelayTime` clamps, `enabled=false` passes through. |
| `runtime/sound/tests/sound_waveshaper_test.cpp` | Built-in backend `WaveShaper` node: recording-tier wiring (`curve`/`gain`/`enabled` read + pushed) and DSP-tier curve assertions — identity curve [-1,1] unchanged, hard-clip curve clamps, out-of-range input clamps to the end values, empty curve and `enabled=false` pass through, gain scales the shaped signal. |
| `runtime/sound/tests/dsp_metrics.hpp` | Shared RMS and Goertzel helpers for sound_system_test. |

## Interfaces and seams

### Exposed interface

```cpp
// runtime/sound/AudioBackend.hpp (engine-agnostic)
using NodeHandle = std::uint64_t;
inline constexpr NodeHandle kInvalidNodeHandle = 0;

enum class NodeKind  { Oscillator, Biquad, Gain, Destination, Panner, Buffer,
                       Delay, Compressor };
enum class Waveform  { Sine, Square, Sawtooth, Triangle, Custom };
enum class FilterType { Lowpass, Highpass, Bandpass, Lowshelf, Highshelf,
                        Peaking, Notch, Allpass };
enum class DistanceModel { Linear, Inverse, Exponential, Ellipsoid };
enum class Param     { Frequency, Detune, Q, Gain, PositionX, PositionY, PositionZ,
                       ListenerPositionX, ListenerPositionY, ListenerPositionZ,
                       ListenerForwardX, ListenerForwardY, ListenerForwardZ,
                       ListenerUpX, ListenerUpY, ListenerUpZ,
                       DirectionX, DirectionY, DirectionZ, Intensity,
                       PlaybackState /* 0 stop+rewind, 1 play, 2 pause */, PlaybackRate };

struct NodeParams {
  float frequency = 440.0f;   // Oscillator/Biquad: Hz
  float detune    = 0.0f;     // cents  (f *= 2^(detune/1200))
  float q         = 1.0f;     // Biquad quality factor
  float gain      = 1.0f;     // linear multiplier
  Waveform   waveform    = Waveform::Sine;
  // Custom waveform (Waveform::Custom, the §16.4.18 PeriodicWave) — terms
  // cross once, like Buffer PCM; k=0 is DC (ignored), harmonic k sums
  // real[k]cos(k·phase) + imag[k]sin(k·phase), peak-normalized to 1:
  std::vector<float> optionsReal;  // real (cosine) terms
  std::vector<float> optionsImag;  // imaginary (sine) terms
  FilterType filterType  = FilterType::Lowpass;
  int  maxChannelCount   = 2; // Destination

  // Panner-only (POSITIONS only — no precomputed gain/coefficient):
  float sourcePosition[3]   = {0,0,0};
  float listenerPosition[3] = {0,0,0};
  float listenerForward[3]  = {0,0,-1};  // default: -Z
  float listenerUp[3]       = {0,1,0};   // default: +Y
  DistanceModel distanceModel  = DistanceModel::Inverse;
  float referenceDistance      = 1.0f;
  float maxDistance            = 10000.0f;
  float rolloffFactor          = 1.0f;

  // Ellipsoid (the §16.4.17 Sound node, ADR-0050) — geometry, not gain:
  float direction[3] = {0,0,1};
  float minFront = 1, minBack = 1, maxFront = 10, maxBack = 10;
  float intensity = 1;
  bool  spatialize = true;

  // Buffer (decoded mono PCM, sent once at createNode, ADR-0050):
  std::vector<float> samples;
  float sampleRate = 44100.0f;
};

class AudioBackend {
public:
  virtual NodeHandle createNode(NodeKind kind, const NodeParams &params) = 0;
  virtual void       connect(NodeHandle dst, NodeHandle src) = 0;
  virtual void       setParam(NodeHandle node, Param param, float value) = 0;
  virtual void       render(NodeHandle destination, int frames,
                            float sampleRate, std::vector<float> &out) = 0;
  virtual void       renderStereo(NodeHandle destination, int frames,
                                  float sampleRate, std::vector<float> &outLR) = 0;
};
```

`SoundSystem::attach(node, ctx)` accepts `Sound`, `AudioDestination` and `SpatialSound` nodes; for any other node type it is a no-op. Without a backend it is inert (the null-backend path). On attach it recurses the root node's `children` bottom-up — sources first, then processing — creating one backend node per §16 node and wiring each child into its parent via `connect(parent, child)`, matching the §16 model where `children` = inputs feeding INTO the parent. A `SpatialSound` root synthesizes a `Destination` + `Panner` carrying the resolved positions; `setListener(ListenerPointSource*)` must be called beforehand if a listener is in the scene. `SoundSystem::update(now, ctx)` re-reads each mapped node's (possibly route-animated) scalar fields and pushes them as `setParam` calls each tick.

`attachStandardRuntime` also registers `SoundTimeSystem` for `OscillatorSource`, `Gain`, `BiquadFilter`, `Delay`, and `DynamicsCompressor`. It reuses `X3DTimeDependentSystem` to emit lifecycle outputs, and `SoundSystem` gates these nodes from `isActive`/`isPaused`. `BuiltinDspBackend` silences a disabled oscillator and passes disabled processors through. Because playback now follows `isActive`, an embedder that drives `SoundSystem` without `attachStandardRuntime` must register `SoundTimeSystem` (`runtime/events/SoundTimeSystem.hpp`) itself, or the sources stay silent. `AudioDestination.gain` is applied at render; `maxChannelCount` remains unused by the mono renderer, and `StreamAudioDestination` is not built.

DynamicsCompressor (§16.4.9). `SoundSystem` builds it as a `NodeKind::Compressor` node — threshold/knee/ratio/attack/release plus the processing `gain`/`enabled` are pushed at build and re-read each tick. `BuiltinDspBackend` implements a feed-forward compressor (dB-domain gain computer with a soft knee, one-pole attack/release smoothing); `enabled=false` passes the input through. `reduction` is outputOnly and is not posted: the seam has no per-node metering path (`createNode`/`connect`/`setParam`/`render` only).

Sound + AudioClip (Immersive). A `Sound` root builds `Destination ← Panner(Ellipsoid) ← source`. Each tick `SoundSystem` pushes the `Sound`'s world location and direction, the ellipsoid lengths scaled by its world transform, `intensity`, and the bound viewpoint's pose as the listener. An `AudioClip` source (under a `Sound` or anywhere in a v4 graph) becomes a `Buffer` node once its bytes load. `SoundSystem` tries each `url` in order through the injected `setAssetResolver` (`AssetKind::Audio`). `Pending` retries on the next `update`. The bytes go through the injected `setAudioDecoder`. On success it posts `duration_changed`. Playback then follows the time lifecycle that `MediaTimeSystem` runs (`isActive`/`isPaused` → `PlaybackState`), with the pitch captured at activation → `PlaybackRate` and `gain` → `Gain`. Without a resolver and decoder a clip stays silent. No app in this repo constructs a `SoundSystem`, so an embedder injects both (for example `io::file::makeFileResolver()` and `io::wav::makeWavDecoder()`). Not read: `Sound.priority`, `AudioClip.description`; HRTF and Doppler are deferred.

BufferAudioSource (§16.4.5,). Its authored `buffer` MFFloat is PCM, so `SoundSystem` feeds the same `Buffer` node an AudioClip decodes into — no decoder, resolver or fetch involved. `MediaTimeSystem` runs its lifecycle too (`enabled`/`loop`/cycle reads), so `startTime`/`stopTime`/`pauseTime`/`resumeTime` drive `isActive`/`isPaused`/`elapsedTime` exactly as for an AudioClip; `gain` → `Gain`, and `playbackRate` × `detune` (cents) is captured at activation → `PlaybackRate`. Mono is the seam contract: a `numberOfChannels` > 1 buffer is read as interleaved frames and averaged down (the WAV decoder's policy); `bufferDuration` truncates the seconds used; an authored `sampleRate` of 0 (spec: "the context rate") falls back to 44100 Hz, and without an authored `sampleRate` the duration is unknowable so the lifecycle plays until `stopTime`. Not wired: `loopStart`/`loopEnd` (the Buffer node wraps the whole buffer — a loop region would need a seam extension), the `X3DUrlObject` `url`/`description`, the `length` output, reversed (negative-rate) playback.

Sound + MovieTexture. `SoundSystem::setMovieAudioDecoder` accepts an `AudioDecoder` callback for the movie's encoded bytes. It resolves `MovieTexture.url` with `AssetKind::Movie`, creates one Buffer from the decoded mono PCM, and retries Pending assets. `load=FALSE` defers reading. The Buffer follows MovieTexture `isActive`/`isPaused` and the speed captured at activation; the consumer separately calls `reportMovieDuration` for the movie's duration. `io::plmpeg::makePlMpegMovieAudioDecoder()` handles MP2 tracks in MPEG program streams. Raw `.m1v` video has no audio track. See ADR-0052.

The §16 `OscillatorSource` binding has no authored waveform field — `periodicWave` was added to the node in X3D 4.1 and the generated bindings model 4.0, so a parsed scene's authored link cannot be read back (a `<PeriodicWave containerField="periodicWave"/>` child parks in the parser's `IS` fallback slot; `findField(osc, "periodicWave")` is null — pinned by `x3d_sound_periodicwave`). What ships instead: `SoundSystem::setPeriodicWave(osc, wave)` registers a `PeriodicWave` node for one oscillator before `attach()` — the four standard types map onto the seam's `Waveform` enum, and `CUSTOM` carries the `optionsReal`/`optionsImag` harmonic terms in the node's params (a `CUSTOM` wave with no terms falls back to sine). `BuiltinDspBackend` synthesizes the wave: harmonic k sums `real[k]·cos(kφ) + imag[k]·sin(kφ)` (index 0 is DC, ignored), normalized so the peak absolute value is 1 (Web Audio's default; X3D 4.0 exposes no `disableNormalization` toggle).

Seam-purity rule for Panner: `buildPannerParams()` in `SoundSystem` resolves the `ListenerPointSource`'s `SFRotation` orientation via Rodrigues' formula into `listenerForward` and `listenerUp` unit vectors, then writes source and listener positions into `NodeParams`. It computes no gain and no pan angle — that computation happens exclusively in the backend.

### Seam points

- `AudioBackend` defines the engine interface. `BuiltinDspBackend` implements it without an external audio library. Hosts choose the backend at construction.
- Param animation via the inbound cascade (no new mechanism) — An author routes an interpolator into `Gain.gain`; the existing [event cascade](event-cascade.md) writes the field; `SoundSystem::update` reads the current field each tick and pushes it via `setParam`. Identical to how `PhysicsSystem` reads node state — no new event mechanism.
- `render` output is backend policy — a buffer here (headless testing); a real backend outputs to a device, a network stream, or another buffer. The seam makes the output target the backend's concern.

## Related

- [ADR-0050: Decoded PCM and the Sound Ellipsoid Cross the AudioBackend Seam](../decisions/0050-audio-pcm-and-ellipsoid-seam.md)
- [ADR-0026: Second AudioBackend (miniaudio) + Headless Swap-Test as Genericity Proof](../decisions/0026-audiobackend-second-backend-swap-test.md)
- [ADR-0020: Sound via an Engine-Agnostic AudioBackend Seam](../decisions/0020-sound-seam.md)
- [Seam-Status Matrix](../seam-status.md)
- [ADR-0019: Physics via a Flag-Gated Engine Backend](../decisions/0019-physics-seam.md)
- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md)
- [Physics subsystem](physics.md)
- [Script / SAI Runtime](system-script-sai.md)
- [Event Cascade](event-cascade.md)
- [Execution Context](execution-context.md)

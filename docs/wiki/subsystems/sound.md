---
title: "Sound (§16 audio graph)"
summary: "X3D §16 Sound component via a proven-generic [STABLE] AudioBackend seam; BuiltinDspBackend (always-built) and MiniaudioBackend (-DX3D_CPP_BUILD_MINIAUDIO=ON) as two independent backends proven by the headless x3d_sound_swaptest CI gate (synthesis + spatial structural invariants)."
tags: [subsystem, sound, seam, audio, genericity, swap-test]
updated: 2026-06-24
related:
  - ../architecture.md
  - ../decisions/0020-sound-seam.md
  - ../decisions/0026-audiobackend-second-backend-swap-test.md
  - ../seam-status.md
  - ../decisions/0019-physics-seam.md
  - ../subsystems/physics.md
  - ../subsystems/event-cascade.md
  - ../subsystems/execution-context.md
---

# Sound (§16 audio graph)

## Purpose

Animates the X3D §16 Sound component nodes (`OscillatorSource`, `BiquadFilter`, `Gain`, `AudioDestination`, `SpatialSound`, `ListenerPointSource`) — previously modeled but behaviorally inert — by building and driving an audio-processing graph each tick. It owns the boundary between the X3D §16 declarative node graph and a DSP engine, using the same **seam pattern** as [Physics](physics.md) (`PhysicsBackend`→Jolt) and [Script/SAI](system-script-sai.md) (`ScriptEngine`→Duktape): the runtime defines an engine-agnostic contract; a backend implements DSP. Unlike those two seams the built-in DSP backend carries **no external dependency**, so it ships in the default build with no flag-gate.

The `AudioBackend` seam is **proven generic** ([ADR-0026](../decisions/0026-audiobackend-second-backend-swap-test.md)): two independent backends — `BuiltinDspBackend` (always-built, RBJ synthesis, equal-power spatial) and `MiniaudioBackend` (miniaudio v0.11.x, flag-gated, `ma_spatializer`) — are exercised by the CI-gated headless swap-test `x3d_sound_swaptest`. The synthesis tier proves numeric spectral agreement (RMS ±2%, Goertzel ±5%); the spatial tier proves structural physical invariants (ear-sign, symmetry, monotonic distance falloff over all three `DistanceModel` modes). The seam is frozen `[STABLE]`.

The seam carries **positions** (source + listener) for `Panner` nodes — the backend computes pan and distance attenuation from the geometry itself. Precomputed gains never cross the seam (seam-purity contract in `AudioBackend.hpp`). `SoundSystem` does only SDK-side plumbing: it reads the X3D `SFRotation` orientation, applies Rodrigues' rotation to derive `listenerForward` and `listenerUp` vectors, and pushes positions into `NodeParams`. All spatial DSP (azimuth, distance model, pan law) runs exclusively in the backend.

## Key files

| File / directory | Role |
|---|---|
| `runtime/sound/AudioBackend.hpp` | **CORE** abstract seam. Pure-virtual; opaque `NodeHandle` (`uint64_t`); `NodeKind` enum (`Oscillator`/`Biquad`/`Gain`/`Destination`/`Panner`); `NodeParams` (small tagged struct of `SF*` scalars + `Waveform`/`FilterType` enums + Panner position/orientation/distance fields); `Param` enum (`Frequency`/`Detune`/`Q`/`Gain`/`PositionX`/`PositionY`/`PositionZ`); `DistanceModel` enum (`Linear`/`Inverse`/`Exponential`); `createNode`/`connect`/`setParam`/`render`/`renderStereo`. Contains **zero DSP-engine types** — every value is the runtime's own SF* scalar or a bounded enum. Seam carries POSITIONS only for Panner nodes — precomputed gains must not cross it. |
| `runtime/sound/SoundSystem.hpp` | **CORE** `System`. Reads the §16 `children` graph bottom-up into backend nodes and connections. Constructed with a `shared_ptr<AudioBackend>` — inert if none (exactly like `PhysicsSystem` without a backend). `attach()` accepts `AudioDestination` and `SpatialSound`; `setListener()` registers the `ListenerPointSource`; `buildPannerParams()` resolves SFRotation→forward/up (SDK plumbing — no spatial DSP). `renderStereo()` delegates to the backend's stereo path. |
| `runtime/sound/dsp/BuiltinDspBackend.hpp` | **CORE** (no flag needed). Backend A — default `AudioBackend` implementation — header is DSP-free (pImpl); all DSP math lives in `BuiltinDspBackend.cpp`. |
| `runtime/sound/dsp/BuiltinDspBackend.cpp` | All audio math: phase-accumulator oscillators (all four waveforms), RBJ "Audio EQ Cookbook" biquad coefficients (Lowpass/Highpass/Bandpass/Notch/Allpass full, Lowshelf/Highshelf/Peaking pass-through pending a dB-gain param), per-sample Direct Form I difference equation, gain multiply, destination summing. Topological pull render. **Spatial path:** Panner nodes compute distance `d = |src−listener|`, distance gain via the three `DistanceModel` closed forms, azimuth from `dot(normalize(src−listener), normalize(cross(forward,up)))`, equal-power pan `θ=(az_norm·0.5+0.5)·(π/2)`, `gL=cos θ`, `gR=sin θ`. `renderStereo` does a stereo-aware sum at the destination (Panner blocks are interleaved L/R; other inputs are mono-duplicated). |
| `runtime/sound/miniaudio/MiniaudioBackend.hpp` | Backend B — `AudioBackend` backed by miniaudio v0.11.x (`-DX3D_CPP_BUILD_MINIAUDIO=ON`). Header is seam-pure; all miniaudio types are PRIVATE to the TU. |
| `runtime/sound/miniaudio/MiniaudioBackend.cpp` | miniaudio synthesis (`ma_waveform_node`) + biquad (`ma_biquad_node`) + spatial (`ma_spatializer`) path. Uses `ma_node_graph_read_pcm_frames` for headless render — no audio device required. `MINIAUDIO_IMPLEMENTATION` defined only in this TU. |
| miniaudio v0.11.25 (fetched via CMake FetchContent from [github.com/mackron/miniaudio](https://github.com/mackron/miniaudio)) | Single-file amalgamation. Unlicense OR MIT-0. Included only in `MiniaudioBackend.cpp`; fetched only when `-DX3D_CPP_BUILD_MINIAUDIO=ON`. |
| `runtime/sound/RecordingBackend.hpp` | Test backend that records every `createNode`/`connect`/`setParam` call (no DSP, `render` returns silence). Proves graph construction is correct + deterministic in isolation. |
| `runtime/sound/tests/sound_system_test.cpp` | Five-tier test: (a) recording-backend graph construction, (b) DSP sample assertions, (c) param animation, (d) render determinism, (f) spatial DSP — hard-left/hard-right/centered equal-power pan + distance falloff. |
| `runtime/sound/tests/sound_swap_test.cpp` | **Genericity proof** — headless swap-test. Synthesis tier: same `OscillatorSource → BiquadFilter → Gain → AudioDestination` graph through both backends; RMS ±2%, Goertzel ±5%, F2 in-band ±15%. Spatial tier: same `OscillatorSource → Panner → Destination` graph at multiple positions and all three `DistanceModel` modes; ear-sign, centered symmetry, monotonic distance falloff structural invariants. CI-gated (`x3d_sound_swaptest`). |
| `runtime/sound/tests/dsp_metrics.hpp` | Shared RMS and Goertzel helpers for the swap-test and sound_system_test. |

## Interfaces and seams

### Exposed interface

```cpp
// runtime/sound/AudioBackend.hpp (CORE, engine-agnostic)
using NodeHandle = std::uint64_t;
inline constexpr NodeHandle kInvalidNodeHandle = 0;

enum class NodeKind  { Oscillator, Biquad, Gain, Destination, Panner };
enum class Waveform  { Sine, Square, Sawtooth, Triangle };
enum class FilterType { Lowpass, Highpass, Bandpass, Lowshelf, Highshelf,
                        Peaking, Notch, Allpass };
enum class DistanceModel { Linear, Inverse, Exponential };
enum class Param     { Frequency, Detune, Q, Gain, PositionX, PositionY, PositionZ };

struct NodeParams {
  float frequency = 440.0f;   // Oscillator/Biquad: Hz
  float detune    = 0.0f;     // cents  (f *= 2^(detune/1200))
  float q         = 1.0f;     // Biquad quality factor
  float gain      = 1.0f;     // linear multiplier
  Waveform   waveform    = Waveform::Sine;
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

`SoundSystem::attach(node, ctx)` accepts `AudioDestination` and `SpatialSound` nodes; for any other node type it is a no-op. Without a backend it is inert (the null-backend path). On attach it recurses the root node's `children` **bottom-up** — sources first, then processing — creating one backend node per §16 node and wiring each child into its parent via `connect(parent, child)`, matching the §16 model where `children` = inputs feeding INTO the parent. A `SpatialSound` root synthesizes a `Destination` + `Panner` carrying the resolved positions; `setListener(ListenerPointSource*)` must be called beforehand if a listener is in the scene. `SoundSystem::update(now, ctx)` re-reads each mapped node's (possibly route-animated) scalar fields and pushes them as `setParam` calls each tick.

The §16 `OscillatorSource` binding has **no authored waveform field** — `SoundSystem` unconditionally passes `Waveform::Sine`. The seam carries all four waveforms so a future authored field or production backend can use them without a seam change.

**Seam-purity rule for Panner:** `buildPannerParams()` in `SoundSystem` resolves the `ListenerPointSource`'s `SFRotation` orientation via Rodrigues' formula into `listenerForward` and `listenerUp` unit vectors, then writes source and listener **positions** into `NodeParams`. It computes **no gain and no pan angle** — that computation happens exclusively in the backend.

### Seam points

- **`AudioBackend`** — the DSP engine seam `[STABLE]`. `BuiltinDspBackend` is the shipped default (no flag); `MiniaudioBackend` is the second backend (flag-gated, proven in swap-test). LabSound is the documented flag-gated production follow-on (full filter/convolver/spatializer/device set, deferred). Swapping backends touches only the construction site — the seam carries no engine types.
- **Param animation via the inbound cascade (no new mechanism)** — An author routes an interpolator into `Gain.gain`; the existing [event cascade](event-cascade.md) writes the field; `SoundSystem::update` reads the current field each tick and pushes it via `setParam`. Identical to how `PhysicsSystem` reads node state — no new event mechanism.
- **`render` output is backend policy** — a buffer here (headless testing); a real backend outputs to a device, a network stream, or another buffer. The seam makes the output target the backend's concern.

## How it is tested

The test (`runtime/sound/tests/sound_system_test.cpp`) runs in five independent tiers, all headless with no audio hardware:

**(a) Recording tier — graph construction:** a §16 `OscillatorSource → BiquadFilter → Gain → AudioDestination` scene (440 Hz oscillator, 8 kHz lowpass cutoff, gain = 0.5) → `SoundSystem.attach(RecordingBackend)` → asserts:
- 4 nodes created (one per kind), 3 connections (child→parent, `osc→biquad→gain→dest`).
- `SoundSystem::nodeCount() == 4`, `destinationCount() == 1`.
- Initial params read off the §16 nodes: `osc.frequency == 440 Hz`, `biquad.frequency == 8000 Hz`, `biquad.filterType == Lowpass`, `gain == 0.5`.
- The op sequence (7 total: 4 creates + 3 connects) is **deterministic** — two runs record identical op-kind/handle/direction sequences.

**(b) DSP tier — real sample correctness:** same chain → `BuiltinDspBackend` → `render(4096 frames, 48 kHz)`:
- RMS of the rendered buffer ≈ **0.354** (expected: full-amplitude sine at gain 0.5 → `0.5/√2 ≈ 0.354`; test window: `> 0.30` and `< 0.40`).
- Goertzel magnitude at 440 Hz ≈ **0.346** (test: `> 0.30`); Goertzel at 5 kHz ≈ **0.00007** (test: `< g440 × 0.1`), confirming the 440 Hz tone dominates a non-harmonic bin by over **4000×**.
- Low-frequency pass vs high-frequency stop: 200 Hz through a 1 kHz lowpass → near-unity RMS (> 0.5); 15 kHz through the same lowpass → strongly attenuated (RMS `< rLo × 0.1`, ≥ **10×** attenuation; in practice the test log shows ~**377×**).

**(c) Param animation:** 440 Hz oscillator, gain animated from 0.0 → 0.25 → 0.5 → 0.75 → 1.0 via `ctx.postEvent(gain, "gain", SFFloat{g})` + `ctx.tick(t)` (the same inbound mechanism a `ScalarInterpolator` ROUTE uses) → per-block RMS ramps **monotonically** with gain; at gain 1.0 the block RMS > 0.30.

**(d) Determinism:** the same graph (`440 Hz, 6 kHz lowpass, gain 0.7, 4096 frames`) rendered twice from fresh backends → buffers are **byte-identical** (every `float` compares equal).

**(f) Spatial DSP (Task 3):** direct `NodeParams` Panner graph (`OscillatorSource → Panner → Destination`), positions set directly (no full SpatialSound scene path). `renderStereo` at 48 kHz / 4096 frames. Observed values:
- **Hard-left** (`sourcePosition = (-5,0,0)`, listener at origin facing -Z): `rmsL=0.1413, rmsR=0.0000` → `L > 4×R` ✓ (ratio effectively ∞).
- **Hard-right** (`sourcePosition = (+5,0,0)`): `rmsL=0.0000, rmsR=0.1413` → `R > 4×L` ✓.
- **Centered** (`sourcePosition = (0,0,-5)`, directly ahead): `rmsL=rmsR=0.0999`, `|L−R|=0.00000` — equal-power (`gL=gR=1/√2`), `L²+R²≈0.02` ✓.
- **Distance falloff (Linear model, rolloff=1):** `rmsRef=0.4997` at distance 1.0; `rmsMax=0.00000` at distance 10.0 (maxDistance) → `< 5%` of ref energy ✓.

Run: `ctest --preset dev -R x3d_sound_system` (always enabled — no flag needed).

### Swap-test (genericity proof)

`runtime/sound/tests/sound_swap_test.cpp` — runs headless with no audio device; requires `-DX3D_CPP_BUILD_MINIAUDIO=ON`. Run via:

```bash
cmake -S . -B build-audio -G Ninja -DX3D_CPP_BUILD_MINIAUDIO=ON -DX3D_CPP_BUILD_TESTS=ON \
      -DX3D_CPP_PER_HEADER_CHECKS=OFF
cmake --build build-audio --target x3d_sound_swaptest x3d_sound_system
ctest --test-dir build-audio -R x3d_sound
```

**Synthesis tier** (both backends, same graph):
- Same `OscillatorSource → BiquadFilter → Gain → AudioDestination` fixture (440 Hz, 8 kHz lowpass, gain 0.5, 4096 frames, 48 kHz).
- RMS ratio A/B within ±2%.
- Goertzel at 440 Hz ratio within ±5%.
- F2 (2nd harmonic, in-band) ratio within ±15% — intentionally wide to accommodate the RBJ-vs-independent-derivation filter-law delta at the filter knee.

**Spatial tier** (structural invariants, both backends):
- Hard-left, hard-right, and centered positions over `OscillatorSource → Panner → Destination`.
- Ear-sign: source left of listener → left channel louder (and vice versa).
- Centered symmetry: source directly ahead → `|rmsL − rmsR| < threshold` (muted-channel guard).
- Monotonic distance falloff at 5 distances per model (Linear, Inverse, Exponential).
- Energy allowed to differ by up to ±40% between backends — equal-power vs. amplitude+1/d laws agree on physical invariants, not on exact numbers.

CI gate: `.github/workflows/ci.yml` `sound-swaptest` job (`-DX3D_CPP_BUILD_MINIAUDIO=ON`).

## Related specs and ADRs

- [ADR-0026: Second AudioBackend (miniaudio) + Headless Swap-Test as Genericity Proof](../decisions/0026-audiobackend-second-backend-swap-test.md) — the proof ADR; tolerance thresholds + scope honesty.
- [ADR-0020: Sound via an Engine-Agnostic AudioBackend Seam](../decisions/0020-sound-seam.md) — the binding decision: why a seam, why the built-in DSP ships in core, deferred follow-ons.
- [Seam-Status Matrix](../seam-status.md) — Audio row is now GREEN.
- [ADR-0019: Physics via a Flag-Gated Engine Backend](../decisions/0019-physics-seam.md) — the sibling seam this one mirrors; the key distinction is in ADR-0020.
- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md) — the one-way isolation pattern (LabSound, when added, will follow it).
- [Physics subsystem](physics.md) — the seam-pattern sibling (`PhysicsBackend`→`PhysicsSystem`).
- [Script / SAI Runtime](system-script-sai.md) — the other seam sibling (`ScriptEngine`→`SoundSystem` mirror pattern).
- [Event Cascade](event-cascade.md) + [Execution Context](execution-context.md) — the cascade the param animation rides.
- Design spec: `docs/superpowers/specs/2026-06-24-audio-seam-genericity-design.md`

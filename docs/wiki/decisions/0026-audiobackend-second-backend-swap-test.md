---
title: "ADR-0026: Second AudioBackend (miniaudio) + Headless Swap-Test as the Genericity Proof"
summary: An interface is *proven generic* only when a second, independent backend runs identical fixtures to identical observable behavior, gated in CI — applied to the AudioBackend seam by adding a miniaudio backend alongside BuiltinDsp and a CI-gated headless swap-test asserting synthesis-chain agreement (RMS ±2%, Goertzel ±5%) and spatial structural invariants (ear-sign, symmetry, monotonic distance falloff over three distance models). Scope is honest — synthesis is a tight numeric proof; the spatial path is a real-but-coarse structural proof given two different panning laws (equal-power vs. amplitude+1/d). HRTF, Doppler, and the §16 Sound ellipsoid remain deferred (SND-3 partial).
tags: [adr, seam, genericity, audio, audiobackend, miniaudio, swap-test, ci-gate, thesis]
updated: 2026-06-24
related:
  - ../architecture.md
  - ../seam-status.md
  - ../subsystems/sound.md
  - 0024-textureresolver-second-backend-swap-test.md
  - 0022-scriptengine-second-backend-swap-test.md
  - 0020-sound-seam.md
---

# ADR-0026: Second AudioBackend (miniaudio) + Headless Swap-Test as the Genericity Proof

## Status

Accepted (2026-06-24). Implemented on `feat/audio-seam-proof`: Backend A
(BuiltinDspBackend, `runtime/sound/dsp/BuiltinDspBackend.{hpp,cpp}`, always-built,
dependency-free) and Backend B (MiniaudioBackend, `runtime/sound/miniaudio/MiniaudioBackend.{hpp,cpp}`,
`-DX3D_CPP_BUILD_MINIAUDIO=ON`, vendored single-header, Unlicense OR MIT-0) agree on the
synthesis chain within calibrated tolerance and on structural spatial invariants under the
`x3d_sound_swaptest` headless swap-test, gated in CI. The seam is frozen `[STABLE]` in
`runtime/sound/AudioBackend.hpp`.

## Context

The product thesis is that x3d-cpp is **unopinionated and pluggable**: every place a
renderer/engine plugs in is an abstract **seam**, and the runtime core stays spec-correct and
backend-free. The `AudioBackend` seam (`runtime/sound/AudioBackend.hpp`) is the **DSP engine
boundary**: how a §16 audio graph (OscillatorSource → BiquadFilter → Gain → AudioDestination,
optionally via a Panner for spatial sound) is rendered to sample buffers. The seam carries
node KINDs, bounded waveform/filter enums, scalar parameters, and — for Panner nodes — raw
**positions** (source, listener orientation). It carries **no DSP state and no precomputed
gain**.

[ADR-0020](0020-sound-seam.md) established the seam and shipped BuiltinDspBackend — a
dependency-free, RBJ-Cookbook-derived synthesizer. One implementation is not a genericity
proof. This ADR applies the ADR-0022/0023/0024 rule:

> **An interface is proven generic only when a second independent backend runs identical
> fixtures to identical observable behavior, gated in CI.**

This is the fourth seam to take the proof, after [ADR-0022](0022-scriptengine-second-backend-swap-test.md)
(ScriptEngine: Duktape + QuickJS), [ADR-0023](0023-assetresolver-second-backend-swap-test.md)
(AssetResolver: libcurl + S3), and [ADR-0024](0024-textureresolver-second-backend-swap-test.md)
(TextureResolver: stb_image + wuffs).

### The second backend: miniaudio

**miniaudio** (`github.com/mackron/miniaudio`, Unlicense OR MIT-0) is a well-maintained,
production-tested, single-header C audio library with a full node-graph API (`ma_node_graph`,
`ma_waveform_node`, `ma_biquad_node`, `ma_splitter_node`, `ma_spatializer`) and a documented
**headless** render path (`ma_node_graph_read_pcm_frames`) that requires no audio device.
That headless path is the key: it lets the swap-test run in CI with no audio hardware and no
mocking.

The two backends share **zero implementation** — BuiltinDsp uses a hand-rolled phase
accumulator and RBJ biquad difference equations; miniaudio uses its own independent waveform
synthesis and biquad pipeline. A parity failure therefore surfaces a real interface or
DSP-contract leak, not a coincidence.

The backend fetches miniaudio v0.11.x via CMake FetchContent from `mackron/miniaudio`.
The backend is behind the `X3D_CPP_BUILD_MINIAUDIO` build option (OFF default), with
**no core `#ifdef`** — miniaudio meets the seam in a single isolated TU
(`x3d_miniaudio` static lib, PRIVATE, `MINIAUDIO_IMPLEMENTATION` in that TU only). The
default build is byte-identical and behavior-unchanged.

### The equality boundary — why tolerance, not byte-equal

For the **synthesis chain** (OscillatorSource → BiquadFilter → Gain → AudioDestination),
two independently correct synthesizers apply different math:

- **Oscillator**: BuiltinDsp uses a phase-accumulator with per-sample increment; miniaudio
  `ma_waveform_node` uses a similar LUT-free accumulator. Both render the same waveform shape
  but the exact per-sample float values diverge by floating-point rounding.
- **Biquad filter**: BuiltinDsp derives RBJ "Audio EQ Cookbook" coefficients; miniaudio uses
  its own independent coefficient derivation for the same filter types. Both implement the
  correct Butterworth/Linkwitz-Riley response but the two sets of coefficients are
  algebraically distinct — they agree on the transfer function, not on the sample stream.

Byte-equal PCM output is therefore **not** a valid goal. The swap-test instead asserts
**perceptual equivalence** via calibrated spectral metrics:

| Metric | Threshold | Rationale |
|---|---|---|
| RMS level | ±2% | Same signal energy — both backends deliver the expected amplitude |
| Goertzel magnitude at signal frequency | ±5% | Same dominant spectral content |
| F2 (passband Goertzel @ 440 Hz) in-band | ±15% | Honest RBJ-vs-independent-derivation delta at the filter knee |

The ±15% F2 bound is deliberately wide and explicitly documented: it reflects the
RBJ-vs-Butterworth filter-law difference at the filter knee (both implementations
pass the 440 Hz tone through a 1 kHz lowpass; the two coefficient derivations agree
on the transfer function but diverge in-band at 440/1000 Hz with default Q), not
test slop.

### The spatial path — structural, not numeric

For the **spatial path** (Panner nodes), the two backends implement fundamentally different
panning laws:

- **BuiltinDspBackend**: equal-power pan (`gL = cos θ`, `gR = sin θ` where `θ ∈ [0, π/2]`)
  + RBJ-derived distance attenuation per the three `DistanceModel` closed forms.
- **MiniaudioBackend**: `ma_spatializer` — miniaudio's native spatializer, which uses an
  amplitude-based pan (proportional to the stereo angle) + a `1/d`-style distance falloff.

These two laws **agree on structural physical invariants** but not on gain numbers. The
swap-test asserts invariants both must satisfy:

| Invariant | Check |
|---|---|
| **Ear sign**: source left of listener → left channel louder | `rmsL > rmsR` (resp. right) |
| **Centered symmetry**: source directly ahead → `\|rmsL − rmsR\| / max < 1%` (muted-channel guard) | `\|L−R\|/max < 0.01` |
| **Monotonic distance falloff**: over Linear, Inverse, and Exponential models, increasing `d` → decreasing energy | Checked at 3 distances per model (d=1, 3, 6) |
| **Energy range at refDist (centered)**: both backends produce output within ±40% of the equal-power reference (`monoRms/√2`) | `rmsRef ∈ [monoRms/√2 × 0.60, monoRms/√2 × 1.40]` |

The ±40% energy bound (`kSpatialEnergy = 0.40`) spans the equal-power-vs-amplitude-law
difference: at the centered position BuiltinDsp's equal-power pan lands at exactly
`monoRms/√2` (1.00× the reference) while miniaudio's amplitude law lands at
`monoRms/2 ≈ 0.71×(monoRms/√2)`. The ±40% window covers both with ~10% headroom.
The muted-channel failure mode (one channel silent) is caught by the symmetry check
above (`|L−R|/max < 1%`), not by this energy bound.

### Anti-tautology design: positions cross the seam

The seam was explicitly designed so that **source and listener positions** — not precomputed
gains — cross the seam boundary. Each backend independently computes its own pan angle and
distance attenuation from the geometry. This means:

1. The swap-test cannot be trivially gamed by making Backend B reproduce Backend A's pan
   coefficients — Backend B uses a structurally different algorithm.
2. A future third backend (LabSound's HRTF spatializer, Web Audio's `PannerNode`) slots in
   by implementing the same seam, with its own spatial law.

The seam carries: `sourcePosition[3]`, `listenerPosition[3]`, `listenerForward[3]`,
`listenerUp[3]`, `distanceModel`, `referenceDistance`, `maxDistance`, `rolloffFactor`.
What it explicitly forbids crossing: `gainL`, `gainR`, any pan angle, any precomputed
attenuation coefficient. Violation would make the runtime the spatial DSP engine, not the
seam.

## Decision

**An interface is _proven generic_ only when a second independent backend runs identical
fixtures to identical observable behavior, gated in CI** (the ADR-0022/0023/0024 rule,
restated for this seam).

Concretely, for the `AudioBackend` seam:

- **Backend A — BuiltinDspBackend** (`runtime/sound/dsp/BuiltinDspBackend.{hpp,cpp}`):
  the originally shipped backend — always-built, dependency-free, RBJ-Cookbook synthesis,
  equal-power spatial path. No flag required.
- **Backend B — MiniaudioBackend** (`runtime/sound/miniaudio/MiniaudioBackend.{hpp,cpp}`):
  a second, independent backend behind `X3D_CPP_BUILD_MINIAUDIO` (OFF default), with
  **no core `#ifdef`** — miniaudio meets the seam in a single isolated TU (`x3d_miniaudio`
  static lib, `MINIAUDIO_IMPLEMENTATION` in that TU only, PRIVATE). The default build is
  unaffected. Both the synthesis path and the spatial path (`ma_spatializer`) are
  exercised. The headless render path (`ma_node_graph_read_pcm_frames`) needs no device.
- **Headless swap-test — `x3d_sound_swaptest`** (`runtime/sound/tests/sound_swap_test.cpp`):
  drives the *same* graph fixtures through both backends and asserts the calibrated
  thresholds above. All in-process, headless, no audio device, no service container.
  Synthesis tier: RMS ±2%, Goertzel ±5%, F2 ±15%. Spatial tier: structural invariants
  (ear-sign, symmetry guard, monotonic distance falloff over Linear/Inverse/Exponential).
- **Permanent CI merge gate**: an `audio-swap` job in `.github/workflows/ci.yml`,
  flag-gated `-DX3D_CPP_BUILD_MINIAUDIO=ON`, scoped build of `x3d_sound_swaptest`
  only, `ctest -R x3d_sound_swaptest`. No vcpkg, no docker — miniaudio is a vendored
  single file, so the gate runs sub-minute.
- The interface carried two backends with **no signature change** (one seam extension: the
  `Panner` `NodeKind` + positional `NodeParams` fields + `renderStereo()` were added in a
  prior task, not forced by the second backend). The seam is promoted `[EXPERIMENTAL]` →
  `[STABLE]` in `runtime/sound/AudioBackend.hpp` (a runtime seam, not re-exported in
  `include/x3d/sdk.hpp`).
- **Attribution:** `NOTICE` lists miniaudio (Unlicense OR MIT-0) with the build flag.
  miniaudio's dual-license means the default SDK stays MIT-clean; the flag-gated backend
  adds a Unlicense/MIT-0 option.

## Scope honesty (what this ADR explicitly does NOT prove)

The synthesis proof is **tight** (numeric spectral agreement). The spatial proof is
**real but coarse**: two structurally different panning laws agree on physical invariants,
not on sample values. This is intentional and documented.

The following items are **explicitly OUT** of this proof and remain deferred under SND-3:

- **HRTF spatialization**: no second reference HRTF implementation (LabSound's HRTF
  renderer, the §16 SpatialSound ellipsoid). Deferred.
- **Doppler shift**: no second Doppler implementation. Deferred.
- **§16 Sound ellipsoid** (`minFront`/`maxFront`/`minBack`/`maxBack` attenuation zones):
  the seam carries no ellipsoid params; SpatialSound does not fully map to `Panner`.
  Deferred.
- **Multi-channel rendering**: `renderStereo()` covers stereo; the `ChannelMerger` /
  `ChannelSplitter` / `ChannelSelector` path is deferred (SND-6).

The `x3d_sound_swaptest` CI gate failing would mean one of the proven invariants broke for
one of the two backends — that is a real regression, not test noise.

## Consequences

**Positive:**

- The genericity claim for `AudioBackend` is now **empirical**, not aspirational — the
  permanent CI gate keeps it true.
- The swap-test is headless and fully in-process: no audio device, no mock driver, no
  service container, sub-minute gate.
- The anti-tautology design (positions, not gains, cross the seam) means a third backend
  with a completely different spatial law (HRTF, Web Audio, ambisonics) slots in without
  a seam change.
- The Panner node extension (`NodeKind::Panner`, positional params, `renderStereo`) landed
  without any interface breakage, which is independent evidence the seam is generic.

**Trade-offs / costs:**

- The spatial proof uses **structural invariants** (not sample-level numeric equality)
  because two different panning laws legitimately produce different gain numbers. This is
  the right approach for a spatial seam, but it is weaker than the TextureResolver
  byte-equal gate or the synthesis-chain RMS gate. Documented explicitly.
- One more vendored single-file library in the build tree; the Unlicense/MIT-0 dual license
  means no license compatibility concern for any downstream.
- The `±15%` F2 bound on the filter-knee metric is wide. It reflects the honest RBJ-vs-
  Butterworth delta. If a future backend uses a tighter filter derivation, the bound can
  be narrowed.
- SND-3 (spatialization) moves to PARTIAL: the synthesis chain and distance models are
  proven; the §16 Sound ellipsoid, HRTF, and Doppler remain deferred.

## Related

- [Seam-Status Matrix](../seam-status.md) — the live tracker this ADR turns the Audio row green in.
- [Sound subsystem](../subsystems/sound.md) — the subsystem page with full interface and test detail.
- [ADR-0024: Second TextureResolver Backend (wuffs) + Decode Swap-Test](0024-textureresolver-second-backend-swap-test.md)
  — the prior seam to take the proof; this ADR mirrors its structure.
- [ADR-0022: Second ScriptEngine Backend (QuickJS) + Swap-Test](0022-scriptengine-second-backend-swap-test.md)
  — the original pattern ADR.
- [ADR-0020: Sound via an Engine-Agnostic AudioBackend Seam](0020-sound-seam.md)
  — established the seam and BuiltinDspBackend; this ADR completes the genericity proof.
- Design spec: `docs/superpowers/specs/2026-06-24-audio-seam-genericity-design.md`.

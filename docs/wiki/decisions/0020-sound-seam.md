---
title: "ADR-0020: Sound via an Engine-Agnostic AudioBackend Seam with a Dependency-Free Default Backend"
summary: "§16 audio is a core engine-agnostic AudioBackend seam + a dependency-free BuiltinDspBackend that ships in the default build — unlike physics, no flag-gate is needed. Task 3 added equal-power spatial DSP and SoundSystem SpatialSound/Listener wiring."
tags: [adr, sound, seam, audio]
updated: 2026-06-24
related:
  - ../architecture.md
  - ../subsystems/sound.md
  - ../decisions/0019-physics-seam.md
  - ../decisions/0001-ext-firewall.md
---

# ADR-0020: Sound via an Engine-Agnostic AudioBackend Seam with a Dependency-Free Default Backend

## Status

Accepted

## Context

The X3D §16 Sound component nodes (`OscillatorSource`, `BiquadFilter`, `Gain`, `AudioDestination`) were modeled in the generated bindings but behaviorally inert — no System read their `children` graph or produced audio output. Closing this gap requires real DSP (oscillator math, biquad filter coefficients, gain multiplication) and eventually an output device.

The forces resemble those that produced [ADR-0019 (Physics)](0019-physics-seam.md) but with one important structural difference: the simplest DSP for the v1 node set (oscillators, a biquad filter, gain) is a few hundred lines of pure math with **no external dependency**. The project's no-external-libs discipline (see [ADR-0012](0012-no-external-math-lib.md)) calls this out explicitly: when a capability can be dependency-free, it should be, and the build gate that blocks it from the default path (`X3D_CPP_BUILD_PHYSICS`, `X3D_CPP_BUILD_EXT`) is not needed.

The SDK already has a proven seam pattern for exactly this — `ScriptEngine`→Duktape and `PhysicsBackend`→Jolt — where the runtime defines an engine-agnostic contract and a backend (possibly flag-gated) implements the external dependency. Audio fits the shape: the runtime reads a declarative §16 graph; a backend does the DSP; the output target (buffer/device/network) is the backend's policy, not the runtime's.

The full-featured production backend (LabSound — full C++ Web Audio: convolver, spatializer, device output) carries a real external dependency and is deferred as a flag-gated follow-on, directly analogous to Jolt/Duktape.

## Decision

§16 audio is implemented as a **core engine-agnostic seam** (`runtime/sound/AudioBackend.hpp` — pure-virtual, opaque `NodeHandle`, only `SF*` scalars and bounded enums) plus a **core `SoundSystem`** that reads the §16 `children` graph and drives the seam, plus a **dependency-free default backend** (`runtime/sound/dsp/BuiltinDspBackend`) that lives in `CORE` with **no CMake flag** (unlike Jolt). The key distinction from ADR-0019: the built-in DSP always builds — it is pure math (`<cmath>` only) and requires no `FetchContent`, no external library, and no `-m`-flag arch constraints. LabSound (the production follow-on) will be flag-gated when added, exactly as Jolt is. The `BuiltinDspBackend` is pImpl so its DSP internals (phase accumulators, biquad state, RBJ coefficients) never leak into headers.

## Consequences

**Positive:**
- The third seam-pattern domain ships (after Script and Physics), reinforcing the architecture rather than adding a new shape. `AudioBackend` is a clean extension point: LabSound / miniaudio / a browser's Web Audio / a game-engine audio system each slot in without touching `AudioBackend.hpp` or `SoundSystem.hpp`.
- The default build (`mise run build`, `ctest --preset dev`) is completely unaffected — no new dependency, no new flag, no fetch. The built-in DSP always tests.
- Headless, deterministic, hardware-free audio testing: the `BuiltinDspBackend` renders byte-reproducible sample buffers, so the DSP tier asserts real correctness (440 Hz RMS ≈ 0.354; Goertzel@440 Hz >> @5 kHz; lowpass attenuation; param-animation monotonic RMS ramp) without any audio device.
- Param animation is free: it rides the existing event cascade inbound (an author routes a `ScalarInterpolator` into `Gain.gain`; the cascade writes the field; `SoundSystem::update` reads it each tick via `setParam`) — no new mechanism.
- The seam is purity-checked: `AudioBackend.hpp` and `SoundSystem.hpp` are grepped for DSP leakage (oscillator math / biquad / coefficients / phase / filter state) — there must be none; all DSP lives only in `BuiltinDspBackend.cpp`.

**Trade-offs / costs:**
- The built-in DSP is band-unlimited (naive waveforms, no PolyBLEP anti-aliasing). It is adequate for the v1 validation asserts (tone presence, filtering, gain). Anti-aliasing is a clean later refinement that does not change the seam.
- `Lowshelf`/`Highshelf`/`Peaking` filter types fall back to pass-through in v1 (they need a dB-gain param the current `NodeParams` does not carry). These are named extension points — adding the param and completing those formulas changes only `BuiltinDspBackend.cpp` and `NodeParams`.
- Spatialization (`SpatialSound`/`ListenerPointSource`) **shipped in Task 3**: `NodeKind::Panner`, positional `NodeParams` fields (source/listener positions + orientation vectors + distance model fields), `renderStereo`, and `DistanceModel` are part of the seam. `BuiltinDspBackend` implements equal-power pan (`gL=cos θ`, `gR=sin θ`) and three distance models (Linear/Inverse/Exponential) entirely backend-side from POSITIONS — no precomputed gain crosses the seam. `SoundSystem` resolves `SpatialSound`+`ListenerPointSource` pose into a `Panner` node (SDK-side plumbing: Rodrigues' rotation to derive `listenerForward`/`listenerUp`; no spatial computation). The miniaudio backend's `ma_spatializer` path shipped in ADR-0026 (Task 4); HRTF, Doppler, and the §16 Sound ellipsoid remain future work.
- File/asset audio sources (`AudioClip`/`BufferAudioSource`/`StreamAudioSource`/`MicrophoneSource`) need an audio-decode asset seam (analogous to `AssetResolver`) — deferred. They are skipped by `SoundSystem::buildChild` today (the unknown-kind path).
- v1 is mono. Channel mixing (`channelCount`/`channelCountMode`/`channelInterpretation`) is deferred.

## Related

- [Architecture](../architecture.md) — where the Sound System sits in the behavior System set.
- [Sound subsystem](../subsystems/sound.md) — the implementation detail.
- [ADR-0019: Physics via a Flag-Gated Engine Backend](0019-physics-seam.md) — the seam sibling; the key distinction from this ADR is that physics requires a flag-gated external dependency (Jolt) while sound's default backend is dependency-free.
- [ADR-0001: Ext Firewall](0001-ext-firewall.md) — the one-way isolation pattern that LabSound (when added) will follow.
- Spec: `docs/superpowers/specs/2026-06-20-sound-seam-design.md`

# Sound seam — X3D §16 audio graph (v1: synthesis chain, built-in DSP)

**Date:** 2026-06-20
**Status:** Approved design; build next.
**Goal:** Wire the X3D §16 Sound component as an engine-agnostic **audio seam** + a `SoundSystem` that reads the §16 audio graph, validated headless by a dependency-free DSP backend. v1 ships a real synthesis chain (`OscillatorSource → BiquadFilter → Gain → AudioDestination`); it extends the SDK's proven seam pattern (ScriptEngine→Duktape, PhysicsBackend→Jolt) to a third heavyweight domain — audio.

## Why
The X3D 4.0 Sound component (§16, "new sound component") is modeled on the **Web Audio API** — its nodes are a directed audio-processing graph. ~13 of these nodes are currently inert (no System). The seam fit is excellent: the runtime reads the declarative §16 graph; a backend does the DSP; the core stays audio-engine-agnostic (LabSound / miniaudio / a custom DSP / a browser's Web Audio could each fulfill it). And it's tractable headless: audio is a side effect (samples), so the seam's output is embedder policy (device / buffer / network), and validation renders to a buffer with no audio hardware.

## §16 grounding (from the ISO prose + the generated bindings)
- **The audio graph wires via `children`, not routes.** A processing/destination node's `MFNode [in,out] children [X3DSoundChannelNode, X3DSoundProcessingNode, X3DSoundSourceNode]` are its **inputs** (sources feeding *into* it). The graph flows children→parent (inputs up to the destination). `OscillatorSource : X3DSoundSourceNode`; `BiquadFilter`/`Gain : X3DSoundProcessingNode`; `AudioDestination : X3DSoundDestinationNode`.
- **v1 node fields** (confirmed against `generated_cpp_bindings/` and `runtime/sound/SoundSystem.hpp`): `OscillatorSource` — `frequency`, `detune`, + the `X3DSoundSourceNode` base (`gain`, `startTime`/`stopTime`/`isActive` time-dependency, `pitch`); **NOTE: the `OscillatorSource` binding has NO authored waveform (`type`) field** — `SoundSystem` unconditionally passes `Waveform::Sine` (see `SoundSystem.hpp` line ~150: `p.waveform = Waveform::Sine; // §16 OscillatorSource has no authored type`). The seam carries all four waveforms (`Sine`/`Square`/`Sawtooth`/`Triangle`) for when the binding gains a `type` field or a production backend needs them. `BiquadFilter` — `children`, `frequency`, `qualityFactor` (Q), `detune`, `type` (`BiquadTypeFilterChoices`: LOWPASS/HIGHPASS/…); `Gain` — `children` + the processing-base `gain`; `AudioDestination` — `children`, `maxChannelCount`. (The implementer verifies the exact `gain`/`channelCount` fields on the `X3DSoundProcessingNode`/`X3DSoundSourceNode` bases.)
- **Channel semantics** (`channelCount`/`channelCountMode`/`channelInterpretation`) govern up/down-mixing — **v1 assumes mono** and defers mixing.

## Architecture (mirrors PhysicsBackend→Jolt)
- **`runtime/sound/AudioBackend.hpp`** — CORE abstract seam, **engine-agnostic** (the seam-purity rule: zero DSP-engine types). Opaque `NodeHandle`. Methods: `NodeHandle createNode(NodeKind, const NodeParams&)` (NodeKind ∈ {Oscillator, Biquad, Gain, Destination}; `NodeParams` = a small tagged struct of `SF*` scalars + an enum for waveform/filter-type), `connect(NodeHandle dst, NodeHandle src)` (src feeds into dst — matching §16 `children`=inputs), `setParam(NodeHandle, Param, float)`, `render(NodeHandle destination, int frames, float sampleRate, std::vector<float>& out)`. The output target is the backend's concern (buffer here; a device/network in a real backend).
- **`runtime/sound/SoundSystem.hpp`** — CORE `System` (constructed with a `shared_ptr<AudioBackend>`; inert if none). `attach`: find each `AudioDestination` (and, later, `SpatialSound`), then **recurse `children` bottom-up** — create the backend node for each §16 node (mapping kind + initial params), `connect` each child→parent, and remember the §16-node↔`NodeHandle` map. `update(now)`: for each mapped node, read its (possibly route-animated) param fields (`frequency`/`qualityFactor`/`gain`/…) → `setParam`. Param **animation rides the existing cascade inbound** — an author routes an interpolator into `Gain.gain`; the cascade writes the field; the SoundSystem reads it each tick (same as physics reads node state — no new mechanism). ZERO DSP-engine types.
- **`runtime/sound/dsp/BuiltinDspBackend.{hpp,cpp}`** — a **dependency-free** DSP backend (the default; fits the no-external-libs discipline). Implements `AudioBackend`: oscillator (sine/square/saw/triangle), biquad (RBJ cookbook coefficients for the §16 filter types), gain (per-sample multiply), destination (sum inputs → output buffer). Pure math, ~a few hundred lines. CORE (no flag needed — no dependency).
- **`runtime/sound/RecordingBackend.hpp`** — a test backend that records `createNode`/`connect`/`setParam` ops (for deterministic graph-construction assertions).
- **LabSound** (full C++ Web Audio) is the documented **flag-gated production backend follow-on** — the Jolt/Duktape analog (full filter/convolver/spatializer/device set), deferred.

## Validation (two-tier, headless, deterministic — the "prove it")
1. **Graph tier (recording backend):** a §16 `Oscillator→Biquad→Gain→Destination` scene → `SoundSystem.attach` → assert the recorded backend graph (nodes created, connections, initial params) is exactly correct + the op sequence is deterministic.
2. **DSP tier (built-in backend):** render the graph to a buffer → assert on the **samples**: a 440 Hz oscillator → lowpass `BiquadFilter` → `Gain 0.5` → the buffer has a ~440 Hz tone at the expected amplitude (an FFT peak at 440 Hz within tolerance + RMS ≈ expected), and a high-frequency oscillator is attenuated by the lowpass. **Param animation:** route a `ScalarInterpolator` (driven by a `TimeSensor`) into `Gain.gain` → render across ticks → assert the buffer's per-block RMS ramps as the gain animates. Real DSP correctness, no audio hardware.

## Scope (v1)
- Nodes: `OscillatorSource`, `BiquadFilter`, `Gain`, `AudioDestination`; the `children` graph; param animation; **mono**.
- **Deferred** (each behind a clean extension point): file/asset sources (`AudioClip`/`BufferAudioSource`/`StreamAudioSource`/`MicrophoneSource` — need an **audio-decode asset seam**); the remaining processing nodes (`Convolver`/`Delay`/`DynamicsCompressor`/`WaveShaper`/`Analyser`); **3D spatialization** (`SpatialSound`/`Sound`/`ListenerPointSource` — couples to the head-pose seam, the distinctive X3D piece → the clear v2); channel split/merge + full channel up/down-mixing; the LabSound production backend; output to a real device/`AudioDestination.url`.

## Testing / gates
- Recording-backend unit: the §16 graph → assert nodes/connections/params + determinism.
- DSP-backend unit: render → FFT/RMS sample asserts (tone present, filter attenuates, gain scales); a hand-built graph round-trips.
- Param-animation unit: interpolator→`Gain.gain` → assert the amplitude ramp.
- Determinism: same graph + ticks → byte-identical buffer.
- (Optional) a `sim` sound fixture tracing the animated param fields.
- **No external dependency** in the core path → no flag-gate needed (unlike physics); the built-in DSP always builds + tests. `mise run build` green, `mise run golden` zero-drift, generated + default runtime untouched.

## Success criteria
1. An `AudioBackend` seam (CORE, engine-agnostic — zero DSP types) + `SoundSystem` (reads the §16 `children` graph) + a dependency-free `BuiltinDspBackend` + a `RecordingBackend` exist, following the `runtime/physics/` pattern.
2. The recording tier proves the §16 graph is read correctly + deterministically; the DSP tier proves real sample correctness (440 Hz tone through filter+gain) + param animation.
3. Gates: `mise run build` green, golden zero-drift, generated + default runtime untouched.
4. The seam stays engine-agnostic (LabSound / a browser / miniaudio could fulfill it); spatialization + file sources are clean, named v2 extension points.

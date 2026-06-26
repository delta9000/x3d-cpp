# Audio (AudioBackend) seam — second backend + spatial swap-test (design spec)

**Date:** 2026-06-24
**Seam:** `AudioBackend` (`runtime/sound/AudioBackend.hpp`) — the §16 Sound DSP-graph seam.
**Decision:** [ADR-0026](../../wiki/decisions/0026-audiobackend-second-backend-swap-test.md) (to be authored)
**Pattern:** mirrors TextureResolver (ADR-0024) and FontMetrics (ADR-0025); isolation mirrors
ScriptEngine/QuickJS (ADR-0022). Equality model is the **physics-class "real-but-tolerance"** family.
**Status:** proposed plan (design approved 2026-06-24).

---

## 0. Is this testing something real? (anti-tautology check — the load-bearing section)

A genericity proof is meaningful **only if each backend independently computes the compared output**
from raw inputs that cross the seam. If a value is computed SDK-side and both backends merely *apply*
it, the agreement is tautological and proves nothing (the lesson from the rejected Consumer/RenderDelta
seam — see [[seam-genericity-pattern]]). This spec is built around that rule.

**Real-computation inventory — the synthesis chain (in-contract today):**

| Crosses the seam (raw input) | BuiltinDsp computes independently | miniaudio computes independently |
|---|---|---|
| `Oscillator` + `frequency`,`detune`,`waveform`,`gain` | phase accumulator, `f=freq·2^(detune/1200)`, waveform shaping, gain (`BuiltinDspBackend.cpp:57-72,170-181`) | `ma_waveform` in a `ma_data_source_node` — its own phase/oscillator math |
| `Biquad` + `frequency`,`q`,`filterType` | RBJ-cookbook coeffs + Direct-Form-I difference eq with persistent state (`BuiltinDspBackend.cpp:82-150,182-200`) | `ma_lpf/hpf/bpf_node` or `ma_biquad_node` (backend-computed coeffs) |
| `Gain` + `gain` | multiply summed inputs | `ma_node_set_output_bus_volume` |
| `connect(dst,src)`; `Destination` | recursive fan-in sum | `ma_node_graph` endpoint sum |

`SoundSystem` pushes only raw §16 scalars + graph edges; it computes **no** PCM, gain product, phase, or
coefficient (`SoundSystem.hpp`; header asserts "this layer names NO DSP-engine type and contains NO DSP").
Two independent DSP implementations genuinely synthesize each compared sample. **Confirmed real.** (The
oscillator "wrap" — miniaudio has `ma_waveform` as a data source, not an oscillator *node* — is a wrap,
not a fake: `ma_waveform` runs independent math. The only forbidden move is hand-feeding BuiltinDsp's
samples into miniaudio.)

**The spatial extension — where the trap bites.** Spatialization is **not in the seam today**. To make it
a first-class part of the proof we extend the seam, and the extension is honest **only if positions cross
the seam, not gains**:

- **CORRECT (real):** the seam carries `sourcePosition`, `listenerPosition`, `listenerForward`,
  `listenerUp`, `distanceModel`, `referenceDistance`, `maxDistance`, `rolloffFactor`. **Each backend
  independently computes** distance, the distance-gain closed form, azimuth, and the pan pair.
- **TAUTOLOGICAL (forbidden):** `SoundSystem` computes the pan/attenuation gain and pushes a scalar
  `gain`/`gainL`/`gainR`. Both backends then multiply by the same number — proves nothing.

**Enforcement:** extend the existing mandated seam-purity grep (`AudioBackend.hpp:13-15`) to also reject
`gainL`/`gainR`/`coeff`/pcm-buffer params crossing the seam. Every swap-test assertion must be one that
would **fail if a backend's DSP loop were stubbed to copy its input** — that is the guardrail.

**Verdict:** the proof is real for the synthesis chain (two independent DSP engines) and for the spatial
path *provided positions cross the seam*. This spec enforces both.

---

## 1. Goal

Flip the Audio seam to **GREEN** under the U1–U4 pattern with two **independent** DSP backends —
**BuiltinDsp** (the shipped, dependency-free backend) and a flag-gated, vendored **miniaudio** backend —
covering the synthesis chain **and** a **spatial** fixture, with **≥1 headless rendered PCM-to-buffer
output and no audio device**, then freeze the (spatially-extended) interface `[EXPERIMENTAL]` → `[STABLE]`.

**Success criteria (GREEN):**
1. A CI-gated swap-test renders a fixed fixture set through both backends, **headless** (in-memory
   `std::vector<float>`, no `ma_device`), asserting agreement within calibrated tolerance.
2. The fixture set includes **≥1 spatial fixture** comparing independently-computed L/R energy.
3. `X3D_CPP_BUILD_MINIAUDIO` OFF by default (core unchanged), ON in the gate job; `miniaudio.h` vendored,
   `MINIAUDIO_IMPLEMENTATION` in exactly one TU, linked PRIVATE, no core `#ifdef`.
4. The seam interface (with the spatial extension) frozen `[EXPERIMENTAL]` → `[STABLE]`; seam-status row
   flipped; ADR-0026 + subsystem doc + NOTICE row land in the same change.

**Why tolerance is honest, not a dodge.** Exact byte-equality across two *independent* implementations is
impossible (BuiltinDsp accumulates in `double` then casts to `float`; miniaudio is `float32`; `std::sin`,
biquad coefficient rounding, and `ma_lpf` order-N vs a single RBJ biquad all diverge). Exact `==` would
*force one backend to copy the other* — i.e. break independence. A tolerance test over output **both
backends computed from scratch** is a real cross-implementation comparison. This is the physics-class
proof tier, weaker than FontMetrics' bit-exact (which only worked because both libs read the same on-disk
integer tables), and that is the honest ceiling for a DSP seam.

---

## 2. The seam contract + the spatial extension

**Current v1 surface (`AudioBackend.hpp`, unchanged below):** `NodeHandle`; `NodeKind{Oscillator,Biquad,
Gain,Destination}`; `Waveform`; `FilterType`; `NodeParams{frequency,detune,q,gain,waveform,filterType,
maxChannelCount}`; `Param{Frequency,Detune,Q,Gain}`; four virtuals `createNode`/`connect(dst,src)`/
`setParam`/`render(dest,frames,sampleRate,out)`. **`render()` is mono** — the headless comparison artifact.

**The spatial extension (the minimal, honest delta):**
1. **`NodeKind::Panner`** (extends the enum; the header already advertises extensibility).
2. **Positional params** added to `NodeParams` (or a spatial sub-struct): `sourcePosition[3]`,
   `listenerPosition[3]`, `listenerForward[3]`, `listenerUp[3]`, `distanceModel` enum
   (LINEAR/INVERSE/EXPONENTIAL — 1:1 with generated `X3Denums::DistanceModelChoices`, default INVERSE),
   `referenceDistance`, `maxDistance`, `rolloffFactor`. Cone fields deferred.
3. **`Param::PositionX/Y/Z`** for route-animated source motion.
4. **A stereo render path** — `render()` is mono; equal-power pan produces an L/R pair unobservable in
   mono. Add an `int channels` arg or a sibling `renderStereo()` returning interleaved 2-channel output.
   **This couples in SND-6 (multi-channel) as real interface surface — not test-only.**

**Model:** standardize on the Web Audio `PannerNode` (`panningModel=equalpower`,
`distanceModel∈{linear,inverse,exponential}`), targeting the `SpatialSound` node (generated fields already
1:1). Closed forms each backend computes itself:
- distance gain — LINEAR `1−rolloff·(clamp(d,ref,max)−ref)/(max−ref)`; INVERSE `ref/(ref+rolloff·(max(d,ref)−ref))`;
  EXPONENTIAL `(max(d,ref)/ref)^(−rolloff)`.
- equal-power pan — `gL=cos(θ)`, `gR=sin(θ)`, `θ=azimuth_norm·π/2`, azimuth from source position in the
  listener's forward/up frame.

**Out of the GREEN matrix** (lock to what both backends support — the wuffs/Netpbm lesson): HRTF
(`enableHRTF`), Doppler, and the §16 Sound **ellipsoid** model (min/maxFront/Back) — no second reference
implementation; stay deferred under SND-3.

**SoundSystem wiring:** add a `SpatialSound` branch in `buildChild()` (the documented extension site);
resolve `ListenerPointSource` pose into listener vectors; extend `attach()` to walk Sound/SpatialSound
roots. **SoundSystem still computes no gain** — it only resolves SFRotation→forward/up (plumbing) and
pushes positions.

---

## 3. The headless swap-test

**Mechanism:** both backends render a fixed graph to an in-memory `std::vector<float>` with **no device**.
BuiltinDsp already does (`sound_system_test.cpp:188`). miniaudio: build a `ma_node_graph` and call
`ma_node_graph_read_pcm_frames(&graph, out, frameCount, NULL)` — **no `ma_device_init`, no `ma_engine`,
no ALSA/Pulse/PipeWire.** Both driven from the **same** `SoundSystem`-built graph so only the DSP differs.

**Fixtures (spatial prioritized):**

| # | Fixture | Each backend independently computes | Bug the swap-test catches |
|---|---|---|---|
| 1 | Mono tone: Oscillator(440,sine)→Destination | sine via phase accumulator | broken oscillator (freq/amplitude/DC) |
| 2 | Mono filter: Osc(440)→Biquad(LP,1k)→Dest (+ a 15k/8k variant) | biquad coeffs + difference eq | wrong coeffs / unstable filter |
| 3 | **SPATIAL:** Osc→Panner(sourcePos)→StereoDest; listener at origin facing −Z; render source hard-left/center/hard-right/behind. **Positions cross the seam.** | distance gain + equal-power pan (BuiltinDsp) vs `ma_spatializer`/independent pan (miniaudio) | a broken / SDK-shortcut pan; **proves the seam generic for the distinctive 3D-audio piece** |
| 4 | (optional) Two sources summed | synthesize both + sum | summing/mixing bug |

Fixtures 1/2/4 are expressible on the seam as-is; **Fixture 3 requires the §2 extension** and is the
priority.

---

## 4. The equality model (tolerance, calibrated)

Never per-sample exact-`==` across backends. A multi-part tolerance metric over independently-rendered
buffers (thresholds are **estimates — calibrate empirically and record the measured numbers in
ADR-0026**, as the existing test records its ~377× attenuation):

1. **Goertzel bin magnitude** at each expected frequency (idiom at `sound_system_test.cpp:57-74`):
   `|magA−magB|/max < 0.05`; plus tone-present check (`gOff < g440·0.1`).
2. **Total RMS** (idiom at `:48-53`): `|rmsA−rmsB|/max < 0.05`.
3. **Filter (Fixture 2):** passband/stopband RMS-ratio trend agrees (e.g. both ≥10× stopband
   attenuation) — trend, not exact slope (`ma_lpf` order differs from a single RBJ biquad).
4. **Spatial (Fixture 3):** **L/R energy ratio** within ~10%, **PLUS sign agreement** (which ear is louder
   must match *exactly* — coarse but tautology-proof), **PLUS** equal-power energy preservation
   `gL²+gR²≈1` and total `(L²+R²)` agreement within tolerance.

The structural spatial checks (correct ear louder; monotonic distance falloff; energy preserved) are
**tautology-proof even when the numeric tolerance is loose** — they hold only if each backend actually
computed the pan from positions. Guardrail: if any assertion would still pass when one backend's DSP loop
is stubbed to copy its input, that fixture is tautological and must be redesigned.

---

## 5. Work breakdown (calibration-spike-first)

### Task 1 — Calibration spike (de-risks the one real unknown; lands as throwaway/probe, not shipped gate)

Before committing the spatial swap-test design: vendor `miniaudio.h`, stand up a minimal miniaudio backend
that renders the **synthesis chain** headless via `ma_node_graph_read_pcm_frames`, and a **spatial probe**
that feeds raw positions through `ma_spatializer`. Measure: (a) does the synthesis chain agree with
BuiltinDsp within the §4 metric? (b) what panning/attenuation model does `ma_spatializer` actually produce
— can it be coerced to equal-power, or must the spatial comparison be **structural-only** (ear/falloff/energy)
vs a **hand-rolled independent pan**? **Report the findings before finalizing the spatial tasks.** The spike
outcome locks the spatial equality model and resolves open decision D1.

### U0 — Seam extension (lands first, after the spike confirms the approach)
Add `NodeKind::Panner`, positional `NodeParams`, `Param::PositionX/Y/Z`, the stereo render path; wire
`SpatialSound`+`ListenerPointSource` in `SoundSystem` (positions only); extend the seam-purity grep to
reject `gainL/gainR/coeff/pcm` params; implement the equal-power + distance-model spatial path in
`BuiltinDspBackend.cpp` (pure `std::math`).

### U1 — Isolated, flag-gated miniaudio backend
`X3D_CPP_BUILD_MINIAUDIO` (OFF). Vendor `miniaudio.h`; `MINIAUDIO_IMPLEMENTATION` in one TU
(`MiniaudioBackend.cpp`); isolated static lib linked PRIVATE; no core `#ifdef` (mirror ADR-0022). Node
mapping: Oscillator→`ma_waveform` data source; Biquad→`ma_lpf/hpf/bpf_node` (+`ma_biquad_node` w/
backend-computed RBJ coeffs for Notch/Allpass); Gain→`ma_node_set_output_bus_volume`;
Destination→`ma_node_graph` endpoint; `connect(dst,src)`→`ma_node_attach_output_bus(src,0,dst,0)`
(swap arg order); Panner→`ma_spatializer` (or hand-rolled pan per the spike) fed raw positions.

### U2 — Swap-test target
New ctest target (e.g. `x3d_sound_swaptest`): builds the §3 fixtures via SoundSystem, renders both backends
headless, asserts the §4 metrics. miniaudio TU compiled only into this target.

### U3 — CI job
Gate job builds `-DX3D_CPP_BUILD_MINIAUDIO=ON` and runs the swap-test; default builds stay OFF; vendored
single header + no device → sub-minute. Fold into `mise run ci`'s gate set.

### U4 — Freeze + flip + docs
Freeze the (extended) `AudioBackend` `[EXPERIMENTAL]` → `[STABLE]`; flip the seam-status Audio row to GREEN;
**ADR-0026** (miniaudio chosen; tolerance model + *calibrated* thresholds; spatial-via-positions decision;
equalpower/3-distance-model GREEN matrix; HRTF/Doppler/ellipsoid explicitly out); update
`docs/wiki/subsystems/sound.md`; **NOTICE** row for miniaudio (single-header, public-domain (Unlicense) OR
MIT-0, David Reid/mackron); update `findings.yaml` (SND-3 spatialization → partial; SND-6 partially
unblocked by the stereo path). Run `mise run docs-build` (strict), `docs-drift`, then `code-ingest`/`docs-ingest`.

---

## 6. Resolved decisions
- **Spatial backend / equality model (D1):** resolved by the **Task 1 spike** — lean `ma_spatializer` for
  genuine independence + structural assertions; hand-rolled independent pan only as fallback (noting it
  weakens independence). Do not finalize the spatial tasks until the spike reports.
- **Listener pose (D2):** per-Panner-node param — keeps the seam stateless/deterministic, consistent with
  `createNode`.
- **SFRotation orientation (D3):** pre-derive forward/up SDK-side — rotation→vector is plumbing, not the
  spatial computation; the pan/attenuation still happens per-backend (anti-tautology holds).
- **Tolerance thresholds (D4):** calibrate empirically; record in ADR-0026.
- **§16 ellipsoid (D5):** out of the GREEN matrix (no second reference implementation); deferred under SND-3.

---

## 7. Risks & prior-lesson traps
1. **Tautology trap (highest priority):** positions, never gains, cross the seam; seam-purity grep extended;
   every assertion must fail if a DSP loop is stubbed to copy input.
2. **Headless-device pitfalls:** use `ma_node_graph_read_pcm_frames` with no `ma_device`/`ma_engine`; never
   `ma_device_init` (would touch ALSA/Pulse/PipeWire and break CI).
3. **Spatial divergence widening tolerance:** force miniaudio onto equalpower + closed-form distance, or
   fall back to a hand-rolled independent pan; the structural checks stay valid under loose tolerance.
4. **miniaudio nodes needing a wrap:** Oscillator→`ma_waveform` (wrap, not fake); `ma_lpf` order-N ≠ single
   RBJ biquad (compare trend); verify per-block `setParam` setters (`ma_waveform_set_frequency`,
   `ma_biquad_reinit`) exist and are deterministic in the vendored version.
5. **Verify-don't-trust:** confirm exact miniaudio symbols against the vendored header (the FontMetrics lesson).
6. **Coupled scope creep:** the spatial fixture drags in a stereo render path (SND-6) — real surface; size
   the card accordingly.
7. **Determinism semantics:** BuiltinDsp persists phase/biquad state across `render()`; compare a single
   render-from-fresh so both match single-call semantics.

---

## 8. Definition of Done
- [ ] Calibration spike run; spatial equality model locked + recorded.
- [ ] Seam extended (Panner NodeKind + positions + stereo render); seam-purity grep extended; spatial path
      in BuiltinDsp.
- [ ] `x3d_miniaudio` backend lib (flag-gated OFF, `miniaudio.h` PRIVATE in one TU, no core `#ifdef`).
- [ ] `x3d_sound_swaptest` asserts the §4 metrics (incl. ≥1 headless spatial fixture); green.
- [ ] Dedicated CI job gates it (device-free, sub-minute).
- [ ] `AudioBackend` frozen `[EXPERIMENTAL]` → `[STABLE]`; seam-status GREEN; ADR-0026 Accepted; `sound.md`
      updated; NOTICE row for miniaudio; findings.yaml SND-3/SND-6 updated.
- [ ] `mise run docs-build` (strict) green; `docs-drift` reviewed; RAG stores refreshed.

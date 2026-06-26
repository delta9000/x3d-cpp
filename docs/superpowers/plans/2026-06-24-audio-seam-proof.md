# Audio Seam Genericity Proof — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the `AudioBackend` seam generic with two independent DSP backends (BuiltinDsp + miniaudio) over a headless PCM swap-test that covers the synthesis chain **and** a spatial fixture, then freeze the (spatially-extended) interface `[EXPERIMENTAL]` → `[STABLE]`.

**Architecture:** Mirror the proven pattern (ADR-0024/0025): a flag-gated isolated backend lib (`X3D_CPP_BUILD_MINIAUDIO`, OFF default) whose heavy dep (`miniaudio.h`) is vendored and lives in ONE TU linked PRIVATE; a CI-gated swap-test renders the same `SoundSystem`-built graphs through both backends headless and asserts agreement within a calibrated **tolerance** metric (this is the physics-class proof tier — exact `==` across two independent DSP impls is impossible and would force one to copy the other). The proof is REAL because each backend independently synthesizes PCM and independently computes spatial pan/attenuation from **positions** that cross the seam — never SDK-precomputed gains.

**Tech Stack:** C++17, CMake, miniaudio (vendored single header, public-domain/MIT-0), the existing `BuiltinDspBackend` + `SoundSystem`, `rms`/`goertzel` test helpers, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-06-24-audio-seam-genericity-design.md`

## Global Constraints

- **Anti-tautology (load-bearing):** positions cross the seam for spatial, never gains. Every swap-test assertion must be one that would **fail if a backend's DSP loop were stubbed to copy its input**. Extend the mandated seam-purity grep (`AudioBackend.hpp:13-15`) to also reject `gainL`/`gainR`/`coeff`/pcm-buffer params.
- **No core `#ifdef`; heavy dep PRIVATE in one TU:** `miniaudio.h` (`#define MINIAUDIO_IMPLEMENTATION`) appears only in `MiniaudioBackend.cpp`. The seam header `AudioBackend.hpp` stays DSP-free and engine-type-free.
- **miniaudio backend OFF by default:** `X3D_CPP_BUILD_MINIAUDIO` defaults OFF; `mise run ci` does not enable it by default — a dedicated CI job does. BuiltinDsp stays always-built.
- **Headless only:** swap-test renders to in-memory `std::vector<float>` via `ma_node_graph_read_pcm_frames` — NEVER `ma_device_init`/`ma_engine_init` (would touch ALSA/Pulse/PipeWire and break CI).
- **Equality is tolerance, calibrated:** never per-sample `==` across backends; thresholds are calibrated empirically in Task 1 and recorded in ADR-0026.
- **Spatial GREEN matrix = equalpower panning + LINEAR/INVERSE/EXPONENTIAL distance models only.** HRTF, Doppler, and the §16 Sound ellipsoid are explicitly OUT (no second reference implementation) — stay deferred under SND-3.
- **Spatial decisions (from the spec):** listener pose is a per-Panner-node param (D2); SFRotation orientation is pre-derived to forward/up SDK-side (D3); the §16 ellipsoid stays out (D5).
- **NOTICE update is part of DoD** (the docs-drift tool does not catch it).

---

### Task 1: Calibration spike — vendor miniaudio, prove the synthesis chain headless, characterize spatial

> **This task de-risks the two real unknowns (exact miniaudio API; what `ma_spatializer` computes) and PRODUCES the working miniaudio synthesis backend that later tasks promote. Its report locks the spatial equality model. STOP at the end and report findings to the controller before Task 2.**

**Files:**
- Create: `runtime/sound/miniaudio/vendor/miniaudio.h` (downloaded, unmodified)
- Create: `runtime/sound/miniaudio/vendor/MINIAUDIO_LICENSE.txt`
- Create: `runtime/sound/miniaudio/MiniaudioBackend.{hpp,cpp}` (the real backend — synthesis chain working; spatial probe)
- Create: `runtime/sound/miniaudio/spike/spike_probe.cpp` (throwaway driver; deleted/folded in Task 5)
- Modify: `CMakeLists.txt` (a temporary spike exe — removed in Task 4 when the proper lib lands)

**Interfaces:**
- Consumes: `x3d::runtime::AudioBackend`, `NodeKind`, `NodeParams`, `Param` from `AudioBackend.hpp`.
- Produces: `class x3d::runtime::miniaudio::MiniaudioBackend : public AudioBackend` rendering the synthesis chain headless; and a **spike report** at `.superpowers/sdd/audio-spike-report.md` recording: (1) the exact miniaudio symbols that worked (`ma_node_graph_*`, `ma_waveform_*`, `ma_*_node`, `ma_node_attach_output_bus`, `ma_node_graph_read_pcm_frames`); (2) the synthesis-chain tolerance actually observed vs BuiltinDsp; (3) what `ma_spatializer` computes (panning law + distance model) and whether it is comparable to equal-power, OR the decision to use a hand-rolled independent pan; (4) the calibrated thresholds for §4 of the spec.

- [ ] **Step 1: Vendor miniaudio**

```bash
mkdir -p runtime/sound/miniaudio/vendor
curl -fsSL https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h \
  -o runtime/sound/miniaudio/vendor/miniaudio.h
grep -m1 'miniaudio - v' runtime/sound/miniaudio/vendor/miniaudio.h   # record the version
```
Extract the license block from the header's bottom into `MINIAUDIO_LICENSE.txt` (dual: public-domain (Unlicense) OR MIT-0), mirroring `runtime/io/wuffs/vendor/WUFFS_LICENSE.txt`.

- [ ] **Step 2: Write a minimal `MiniaudioBackend` for the synthesis chain**

Implement `AudioBackend` over a `ma_node_graph`. `MiniaudioBackend.hpp` is engine-free (pImpl, mirroring `BuiltinDspBackend.hpp`). In `MiniaudioBackend.cpp`: `#define MINIAUDIO_IMPLEMENTATION` then `#include "miniaudio.h"`. Map: `Oscillator`→`ma_waveform` wrapped in a `ma_data_source_node`; `Biquad`→`ma_lpf_node`/`ma_hpf_node`/`ma_bpf_node` (start with Lowpass/Highpass/Bandpass); `Gain`→a passthrough node with `ma_node_set_output_bus_volume`; `Destination`→`ma_node_graph` endpoint (`ma_node_graph_get_endpoint`). `connect(dst,src)`→`ma_node_attach_output_bus(src, 0, dst, 0)` (note the swapped arg order vs the seam). `render()`→`ma_node_graph_read_pcm_frames(&graph, out.data(), frames, NULL)` into a mono buffer — **no device**.

> The exact miniaudio symbol names/signatures are confirmed against the vendored header here in the spike (the spec flagged them medium-confidence). If a mapped node needs a different facility, record the working call in the report.

- [ ] **Step 3: Build the spike probe + verify the synthesis chain agrees with BuiltinDsp**

`spike_probe.cpp`: build the same node graph two ways — via `SoundSystem` + `BuiltinDspBackend`, and via `MiniaudioBackend` — for fixtures 440Hz sine→dest and osc→lowpass(1k)→dest. Render 4096 frames @ 48kHz into two `std::vector<float>`. Compare with the existing `rms`/`goertzel` idioms (copy them or include the test helpers). Add a temporary spike exe to `CMakeLists.txt` guarded by `X3D_CPP_BUILD_MINIAUDIO`.

```bash
cmake -S . -B build-audio -G Ninja -DX3D_CPP_BUILD_MINIAUDIO=ON -DX3D_CPP_BUILD_TESTS=ON
cmake --build build-audio --target x3d_audio_spike
./build-audio/x3d_audio_spike      # prints Goertzel/RMS for both backends per fixture
```
Expected: both backends show a strong 440Hz Goertzel bin and similar RMS; the lowpass fixture shows both attenuating the 8kHz probe. Record the actual relative differences (these become the calibrated thresholds).

- [ ] **Step 4: Spatial probe — characterize `ma_spatializer`**

Add a probe that feeds raw source/listener positions (hard-left, center, hard-right, behind) through `ma_spatializer` (init a `ma_spatializer` with `channelsOut=2`, set listener + source position, process a mono tone) and prints L/R energy per position. Determine: does it pan correct-ear-louder with monotonic distance falloff? Is its panning coercible to equal-power, or must the spatial swap-test compare **structural** properties (ear sign, monotonic falloff, energy preserved) against a hand-rolled independent equal-power pan? **Write the decision into the spike report.**

- [ ] **Step 5: Write the spike report + commit**

Write `.superpowers/sdd/audio-spike-report.md` with the four deliverables above. Commit the vendored header + license + the working `MiniaudioBackend.{hpp,cpp}` + the spike probe (the probe is removed/folded in Task 5).

```bash
git add runtime/sound/miniaudio CMakeLists.txt
git commit -m "spike(audio): vendor miniaudio + working synthesis backend; characterize ma_spatializer

Claude-Session: [redacted]"
```

> **CONTROLLER CHECKPOINT:** read the spike report. Lock the spatial equality model (Task 5's spatial assertions) and confirm the miniaudio Panner mapping (Task 4) from it. If the spike shows the synthesis chain does NOT agree within a sane tolerance, that is a finding — stop and escalate before proceeding.

---

### Task 2: Seam extension — Panner NodeKind + positional params + stereo render path

**Files:**
- Modify: `runtime/sound/AudioBackend.hpp` (add `NodeKind::Panner`, positional `NodeParams` fields, `Param::PositionX/Y/Z`, the stereo render entry, extend the purity-grep comment)
- Modify: `runtime/sound/dsp/BuiltinDspBackend.{hpp,cpp}` (accept the new kind/params; stereo render signature — spatial math lands in Task 3)
- Modify: `runtime/sound/miniaudio/MiniaudioBackend.{hpp,cpp}` (accept the new kind/params; stereo render signature)
- Test: `runtime/sound/tests/sound_system_test.cpp` (extend the recording-tier test to assert a Panner node is created with positions)

**Interfaces:**
- Produces: `NodeKind::Panner`; `NodeParams` gains `float sourcePosition[3]`, `listenerPosition[3]`, `listenerForward[3]`, `listenerUp[3]`, `DistanceModel distanceModel`, `float referenceDistance`, `maxDistance`, `rolloffFactor`; `enum class DistanceModel { Linear, Inverse, Exponential }`; `Param::PositionX/PositionY/PositionZ`; a stereo render path `virtual void renderStereo(NodeHandle destination, int frames, float sampleRate, std::vector<float>& outLR) = 0;` (interleaved L,R,L,R…, length `2*frames`).

- [ ] **Step 1: Write the failing recording-tier test**

In `sound_system_test.cpp`, add a test that builds a graph with a Panner node (a `SpatialSound`-shaped fixture) and, via `RecordingBackend`, asserts a `NodeKind::Panner` create record exists carrying non-default `sourcePosition`. (Use a directly-constructed `NodeParams` path if `SpatialSound` wiring isn't in yet — that lands in Task 3; here just prove the seam carries the kind+positions.)

- [ ] **Step 2: Run it — expect FAIL (Panner/positions don't exist yet)**

```bash
cmake --build build-audio --target x3d_sound_system && ./build-audio/x3d_sound_system
```
Expected: compile error (no `NodeKind::Panner`) or assertion fail.

- [ ] **Step 3: Extend `AudioBackend.hpp`**

Add `Panner` to `NodeKind`; add the `DistanceModel` enum; add the positional fields to `NodeParams` (documented as "Panner only; POSITIONS cross the seam — backends compute pan/attenuation themselves, never a precomputed gain"); add `PositionX/Y/Z` to `Param`; add the pure-virtual `renderStereo(...)`. Extend the seam-purity comment to state `gainL`/`gainR`/`coeff`/pcm params are forbidden across the seam.

- [ ] **Step 4: Make both backends compile (stub Panner render; spatial math in Task 3)**

Add the `Panner` case + `renderStereo` to `BuiltinDspBackend` and `MiniaudioBackend` so they build. For now `renderStereo` may duplicate the mono signal to both channels for non-Panner graphs; the Panner DSP lands next task.

- [ ] **Step 5: Run the test — expect PASS; commit**

```bash
cmake --build build-audio --target x3d_sound_system && ./build-audio/x3d_sound_system
git add runtime/sound/AudioBackend.hpp runtime/sound/dsp runtime/sound/miniaudio runtime/sound/tests/sound_system_test.cpp
git commit -m "feat(audio): extend seam with Panner node + positions + stereo render (positions, not gains)

Claude-Session: [redacted]"
```

---

### Task 3: BuiltinDsp spatial path + SoundSystem SpatialSound/Listener wiring

**Files:**
- Modify: `runtime/sound/dsp/BuiltinDspBackend.cpp` (equal-power pan + distance-model gain, pure `std::math`)
- Modify: `runtime/sound/SoundSystem.hpp` (`buildChild` SpatialSound branch; `ListenerPointSource` pose → forward/up; `attach` walks Sound/SpatialSound)
- Test: `runtime/sound/tests/sound_system_test.cpp` (BuiltinDsp spatial unit test: source hard-left ⇒ L energy ≫ R; behind/far ⇒ attenuated; `gL²+gR²≈1`)

**Interfaces:**
- Consumes: `NodeKind::Panner`, the positional `NodeParams`, `renderStereo` from Task 2.
- Produces: BuiltinDsp computes, per the spec §2 closed forms, `d=|src−listener|`, the distance gain (Linear/Inverse/Exponential), azimuth in the listener forward/up frame, and equal-power `gL=cos θ`, `gR=sin θ`. SoundSystem resolves `SpatialSound`+`ListenerPointSource` into a Panner node carrying positions (no gain computed SDK-side).

- [ ] **Step 1: Write the failing BuiltinDsp spatial test**

Add a test: build an oscillator→Panner→stereo-destination graph (positions set directly in `NodeParams`), `renderStereo` via BuiltinDsp, assert: source at hard-left ⇒ `rms(L) > 4·rms(R)`; source hard-right ⇒ inverse; source at `maxDistance` ⇒ total energy ≪ source at `referenceDistance` (monotonic falloff); centered ⇒ `|rms(L)−rms(R)|` tiny and `gL²+gR²≈1`.

- [ ] **Step 2: Run — expect FAIL** (`./build-audio/x3d_sound_system`; Panner currently duplicates mono → L==R, hard-left assertion fails).

- [ ] **Step 3: Implement the equal-power + distance spatial path in `BuiltinDspBackend.cpp`**

Compute (pure `std::math`, double internally): `d = length(src − listener)`; distance gain per `DistanceModel` (LINEAR `1−rolloff·(clamp(d,ref,max)−ref)/(max−ref)`, INVERSE `ref/(ref+rolloff·(max(d,ref)−ref))`, EXPONENTIAL `(max(d,ref)/ref)^(−rolloff)`); azimuth from the source vector projected into the `(listenerForward × listenerUp)` frame; `θ=(0.5·(azimuthNorm+1))·(π/2)`; `gL=cos θ`, `gR=sin θ`; output `L=sample·distGain·gL`, `R=sample·distGain·gR`. **SoundSystem passes positions only — this computation is entirely backend-side.**

- [ ] **Step 4: Wire `SpatialSound` + `ListenerPointSource` in `SoundSystem`**

In `buildChild`, add a `dynamic_cast<SpatialSound*>` branch that reads `location`/`direction`/`referenceDistance`/`maxDistance`/`rolloffFactor`/`distanceModel` and the `ListenerPointSource` pose, **pre-derives forward/up from the SFRotation (plumbing)**, and creates a `NodeKind::Panner` carrying those positions. Extend `attach` to walk Sound/SpatialSound roots.

- [ ] **Step 5: Run the spatial test — expect PASS; commit**

```bash
cmake --build build-audio --target x3d_sound_system && ./build-audio/x3d_sound_system
git add runtime/sound/dsp runtime/sound/SoundSystem.hpp runtime/sound/tests/sound_system_test.cpp
git commit -m "feat(audio): BuiltinDsp equal-power spatial path + SoundSystem SpatialSound/Listener wiring

Claude-Session: [redacted]"
```

---

### Task 4: Promote the miniaudio backend into the flag-gated `x3d_miniaudio` lib

**Files:**
- Modify: `CMakeLists.txt` (replace the temporary spike exe with `option(X3D_CPP_BUILD_MINIAUDIO ... OFF)` + the `x3d_miniaudio` STATIC lib)
- Modify: `runtime/sound/miniaudio/MiniaudioBackend.cpp` (add the Panner mapping per the spike's locked decision)

**Interfaces:**
- Consumes: the working synthesis backend from Task 1; the spike report's spatial decision; the Panner seam from Task 2.
- Produces: `x3d_miniaudio` STATIC lib (flag-gated OFF, `miniaudio.h` PRIVATE in one TU), exposing `MiniaudioBackend`.

- [ ] **Step 1: Add the CMake option + lib (mirror the `x3d_stb` block)**

```cmake
option(X3D_CPP_BUILD_MINIAUDIO "Build miniaudio AudioBackend backend (OFF default)" OFF)
if(X3D_CPP_BUILD_MINIAUDIO)
    add_library(x3d_miniaudio STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/miniaudio/MiniaudioBackend.cpp")
    target_link_libraries(x3d_miniaudio PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_miniaudio PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/miniaudio")
    target_include_directories(x3d_miniaudio PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/miniaudio/vendor")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_miniaudio PRIVATE -w)  # vendored single header.
    endif()
endif()
```
Remove the temporary `x3d_audio_spike` exe.

- [ ] **Step 2: Implement the Panner mapping in `MiniaudioBackend.cpp`**

Per the spike report: either drive `ma_spatializer` with the raw positions, OR (if the spike found it non-comparable) implement an independent hand-rolled equal-power pan in the miniaudio backend (still independent code from BuiltinDsp). Implement `renderStereo` via the node graph.

- [ ] **Step 3: Build the lib — expect success**

```bash
cmake -S . -B build-audio -G Ninja -DX3D_CPP_BUILD_MINIAUDIO=ON -DX3D_CPP_BUILD_TESTS=ON
cmake --build build-audio --target x3d_miniaudio
```
Expected: `x3d_miniaudio` builds; `miniaudio.h` is referenced only by `MiniaudioBackend.cpp` (verify: `grep -rl miniaudio.h runtime/sound | grep -v vendor` shows only the `.cpp`).

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt runtime/sound/miniaudio
git commit -m "feat(audio): flag-gated x3d_miniaudio backend lib (miniaudio.h PRIVATE, one TU)

Claude-Session: [redacted]"
```

---

### Task 5: The swap-test — synthesis + spatial, headless, tolerance

**Files:**
- Create: `runtime/sound/tests/sound_swap_test.cpp`
- Modify: `CMakeLists.txt` (`x3d_sound_swaptest` target, gated on `X3D_CPP_BUILD_MINIAUDIO`)
- Delete: `runtime/sound/miniaudio/spike/spike_probe.cpp` (folded into the swap-test)

**Interfaces:**
- Consumes: `BuiltinDspBackend`, `MiniaudioBackend`, `SoundSystem`, the `rms`/`goertzel` helpers, the spike-calibrated thresholds.
- Produces: ctest target `x3d_sound_swaptest` asserting the spec §4 metrics over both backends.

- [ ] **Step 1: Write the swap-test**

For each fixture (mono tone; mono lowpass + a 15k/8k variant; **spatial** osc→Panner→stereo-dest at hard-left/center/hard-right/behind; optional two-source sum), build the graph via `SoundSystem`, render through BuiltinDsp and miniaudio headless, and assert: Goertzel bin `|magA−magB|/max < ε_goertzel`; total RMS `|rmsA−rmsB|/max < ε_rms`; lowpass stopband-attenuation trend agrees; **spatial:** L/R energy ratio within `ε_spatial`, **sign agreement exact** (which ear louder matches), `gL²+gR²≈1`, total energy agreement. Use the thresholds calibrated in Task 1. (Reuse `rms`/`goertzel` from `sound_system_test.cpp` — extract them to a shared `runtime/sound/tests/dsp_metrics.hpp` if cleaner.)

- [ ] **Step 2: Add the CMake target (gated on both)**

```cmake
if(TARGET x3d_miniaudio AND X3D_CPP_BUILD_TESTS)
    add_executable(x3d_sound_swaptest
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/dsp/BuiltinDspBackend.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/sound/tests/sound_swap_test.cpp")
    target_link_libraries(x3d_sound_swaptest PRIVATE x3d_cpp::x3d_cpp x3d_miniaudio)
    add_test(NAME x3d_sound_swaptest COMMAND x3d_sound_swaptest)
    set_tests_properties(x3d_sound_swaptest PROPERTIES TIMEOUT 120)
endif()
```

- [ ] **Step 3: Build + run — expect PASS**

```bash
cmake -S . -B build-audio -G Ninja -DX3D_CPP_BUILD_MINIAUDIO=ON -DX3D_CPP_BUILD_TESTS=ON
cmake --build build-audio --target x3d_sound_swaptest
ctest --test-dir build-audio --output-on-failure -R x3d_sound_swaptest
```
Expected: all fixtures pass within the calibrated tolerance, including the spatial fixture. **Anti-tautology check:** confirm each assertion would fail if one backend's DSP were stubbed to echo input (it would — both synthesize/pan independently).

- [ ] **Step 4: Commit**

```bash
git add runtime/sound/tests/sound_swap_test.cpp CMakeLists.txt
git rm runtime/sound/miniaudio/spike/spike_probe.cpp
git commit -m "test(audio): headless swap-test — BuiltinDsp vs miniaudio, synthesis + spatial (tolerance)

Claude-Session: [redacted]"
```

---

### Task 6: CI job

**Files:**
- Modify: `.github/workflows/ci.yml` (add `audio-swap` job after the existing swap-test jobs)

- [ ] **Step 1: Add the job** (mirror the `texture-swap` job — vendored dep, no service):

```yaml
  audio-swap:
    name: Audio seam swap-test
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install build tools
        run: sudo apt-get update && sudo apt-get install -y cmake ninja-build ccache g++ mold
      - name: ccache cache
        uses: actions/cache@v4
        with:
          path: ~/.cache/ccache
          key: ccache-audio-${{ github.sha }}
          restore-keys: ccache-audio-
      - name: Configure (miniaudio backend ON)
        run: cmake -S . -B build -G Ninja -DX3D_CPP_BUILD_MINIAUDIO=ON
      - name: Build (Audio swap-test target only)
        run: cmake --build build --target x3d_sound_swaptest
      - name: Swap-test (BuiltinDsp vs miniaudio, headless)
        run: ctest --test-dir build --output-on-failure -R x3d_sound_swaptest
```

- [ ] **Step 2: Validate YAML + commit**

```bash
python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); assert 'audio-swap' in d['jobs']; print('ok')"
git add .github/workflows/ci.yml
git commit -m "ci(audio): gate the BuiltinDsp-vs-miniaudio headless swap-test

Claude-Session: [redacted]"
```

---

### Task 7: U4 — freeze STABLE + ADR-0026 + docs + NOTICE

**Files:**
- Modify: `include/x3d/sdk.hpp` (if AudioBackend is re-exported, flip its `[EXPERIMENTAL]`→`[STABLE]` comment; else note it's a runtime seam)
- Create: `docs/wiki/decisions/0026-audiobackend-second-backend-swap-test.md`
- Modify: `docs/wiki/subsystems/sound.md` (the spatial node, two backends, swap-test)
- Modify: `docs/wiki/seam-status.md` (flip the Audio row GREEN + prose + front-matter)
- Modify: `docs/wiki/coverage.md` (ADR-0026 row + prose)
- Modify: `mkdocs.yml` (ADR-0026 nav entry)
- Modify: `NOTICE` (miniaudio row)
- Modify: `docs/conformance/findings.yaml` (SND-3 → partial; SND-6 partially unblocked)

- [ ] **Step 1: Author ADR-0026** from the ADR-0024 template — record: miniaudio as 2nd backend (vendored, flag-gated); the **tolerance** equality model + the **calibrated thresholds from Task 1**; the spatial-via-positions (anti-tautology) decision; the equalpower + 3-distance-model GREEN matrix; HRTF/Doppler/ellipsoid explicitly OUT; the headless `ma_node_graph_read_pcm_frames` path; the NOTICE row. Status: Accepted.

- [ ] **Step 2: Update `sound.md` + flip the seam-status Audio row GREEN** (`BuiltinDsp` + `miniaudio` + `x3d_sound_swaptest ✓` + a GREEN-row prose section; bump `updated:` + `related:` ADR-0026).

- [ ] **Step 3: Freeze the interface** — flip the AudioBackend seam `[EXPERIMENTAL]`→`[STABLE]` (in `sdk.hpp` if re-exported, and in the `AudioBackend.hpp` header banner), citing ADR-0026.

- [ ] **Step 4: coverage.md + mkdocs.yml nav + NOTICE + findings.yaml** — add the ADR-0026 coverage row + nav entry; add the NOTICE row `miniaudio  github.com/mackron/miniaudio  Unlicense OR MIT-0  (-DX3D_CPP_BUILD_MINIAUDIO=ON)` (vendored section); update findings.yaml SND-3 (spatialization → partial: SpatialSound/equalpower proven; ellipsoid/HRTF deferred) and note SND-6 stereo path landed.

- [ ] **Step 5: Gates + commit**

```bash
mise run docs-build          # strict — must pass
mise run docs-drift working  # advisory
mise run code-ingest && mise run docs-ingest
git add include/x3d/sdk.hpp docs NOTICE mkdocs.yml runtime/sound/AudioBackend.hpp
git commit -m "docs(audio): freeze AudioBackend STABLE + ADR-0026 + sound subsystem + NOTICE

Claude-Session: [redacted]"
```

---

### Task 8: Final verification

- [ ] **Step 1: Clean swap-test run**

```bash
rm -rf build-audio
cmake -S . -B build-audio -G Ninja -DX3D_CPP_BUILD_MINIAUDIO=ON -DX3D_CPP_BUILD_TESTS=ON
cmake --build build-audio --target x3d_sound_swaptest x3d_sound_system
ctest --test-dir build-audio --output-on-failure -R "x3d_sound"
```
Expected: both `x3d_sound_system` and `x3d_sound_swaptest` pass.

- [ ] **Step 2: Default build excludes miniaudio**

```bash
cmake -S . -B build-default -G Ninja
grep -i "miniaudio" build-default/CMakeCache.txt | grep -i "BOOL=OFF" && echo "OK: miniaudio OFF by default"
```

- [ ] **Step 3: No leak / no precomputed-gain across the seam**

```bash
grep -rn "miniaudio.h" runtime/sound include | grep -v vendor | grep -v MiniaudioBackend.cpp && echo "LEAK" || echo "OK: no miniaudio leak"
grep -rnE "gainL|gainR|coeff|pcm" runtime/sound/AudioBackend.hpp && echo "PURITY VIOLATION" || echo "OK: seam carries no DSP/gain"
```

---

## Self-Review

**Spec coverage:** §0 anti-tautology → Global Constraints + Task 2 Step 3 (positions-not-gains) + Task 8 Step 3 purity grep. §1 goal/headless → Tasks 1,5,6. §2 seam+spatial extension → Tasks 2,3. §3 headless swap-test/fixtures → Tasks 1,5. §4 tolerance metric → Task 1 (calibrate) + Task 5 (assert). §5 spike-first → Task 1 + the controller checkpoint. §6 decisions D2/D3/D5 → Global Constraints; D1/D4 → Task 1 spike. §7 risks → Global Constraints + Task 8. §8 DoD → Tasks 5–8.

**Placeholder scan:** The miniaudio internal API calls in Tasks 1/4 are deliberately confirmed-by-spike (Task 1's explicit deliverable), not placeholders — Task 1 writes the working code against the real header and its report records the exact symbols; later tasks build on that. Spatial-fixture thresholds (Task 5) come from Task 1's calibration, recorded in ADR-0026. This is a real dependency chain, not "TBD".

**Type consistency:** `NodeKind::Panner`, `DistanceModel{Linear,Inverse,Exponential}`, the positional `NodeParams` fields, `Param::PositionX/Y/Z`, `renderStereo(NodeHandle,int,float,std::vector<float>&)`, `MiniaudioBackend`/`x3d_miniaudio`, `X3D_CPP_BUILD_MINIAUDIO`, and `x3d_sound_swaptest` are used consistently across Tasks 2–8.

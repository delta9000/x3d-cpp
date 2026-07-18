---
title: Seam-Status Matrix
summary: The live tracker for the product's core thesis — every renderer/engine seam is proven generic only when a second independent backend runs identical fixtures to identical observable behavior, gated in CI. One row per seam; GREEN = interface frozen + ≥2 backends + a CI-gated swap-test.
tags: [seam, tracker, genericity, swap-test, thesis, meta]
updated: 2026-06-24
related:
  - index.md
  - coverage.md
  - decisions/0022-scriptengine-second-backend-swap-test.md
  - decisions/0023-assetresolver-second-backend-swap-test.md
  - decisions/0024-textureresolver-second-backend-swap-test.md
  - decisions/0026-audiobackend-second-backend-swap-test.md
  - subsystems/system-script-sai.md
  - subsystems/system-texture-decode.md
  - subsystems/sound.md
  - decisions/0025-fontmetrics-second-backend-swap-test.md
  - subsystems/system-font-metrics.md
  - subsystems/system-script-sai.md
  - subsystems/system-texture-decode.md
---

# Seam-Status Matrix

This is the **live tracker for the product's core thesis**: x3d-cpp is unopinionated and
pluggable — every place a renderer/engine plugs in is an abstract *seam*, and a seam is
**proven generic** only when a **second independent backend** runs identical fixtures to
**identical observable behavior**, gated in CI ([ADR-0022](decisions/0022-scriptengine-second-backend-swap-test.md)).

A row is **GREEN** when all of: the interface is `[STABLE]` (frozen pre-v2 in
[`include/x3d/sdk.hpp`](https://github.com/delta9000/x3d-cpp/blob/main/include/x3d/sdk.hpp)),
**two independent backends** implement it, and a **CI-gated swap-test** asserts they produce
identical observable behavior over shared fixtures. Until a seam reaches two backends + a
swap-test it is **NOT-YET-PROVEN** — one backend (or pending) is not a genericity proof, only
an unverified claim. This matrix is the live record the Seam-harness card formalizes.

## The matrix

| Seam | Interface | Backend A | Backend B | Swap-test | Findings |
|---|---|---|---|---|---|
| **ScriptEngine** | **STABLE** | Duktape (EcmaScriptBackend) | QuickJS / quickjs-ng v0.15.1 (QuickJsBackend) | `x3d_quickjs_swap` ✓ | SCR-* |
| **AssetResolver / IO** | **STABLE** | libcurl (HttpResolver) | AWS C++ SDK S3 (S3Resolver, docker minio fixture) | `x3d_assetresolver_swap` ✓ | unblocks NSN-*, PRF-6, CONF-CRITIC-2, SCR-005 |
| Physics (PhysicsBackend) | EXPERIMENTAL | Jolt (flag-gated) | — pending | — | CONF-RBP* |
| **Audio (AudioBackend)** | **STABLE** | BuiltinDsp | miniaudio (MiniaudioBackend) | `x3d_sound_swaptest` ✓ | thesis-completion (SND-3 partial) |
| **FontMetrics** | **STABLE** | stb_truetype (StbttFontMetrics) | FreeType (FreetypeFontMetrics) | `x3d_text_tests` ✓ | thesis-completion (no findings) |
| **TextureResolver** | **STABLE** | stb_image (StbTextureResolver) | wuffs v0.3.4 (WuffsTextureResolver) | `x3d_texture_tests` ✓ | thesis-completion (no findings) |
| GeoProjection | EXPERIMENTAL | flat-fallback | — pending | — | — |
| **MovieDecoder** | **STABLE** | pl_mpeg / MPEG-1 (PlMpegMovieDecoder, [ADR-0041](decisions/0041-moviedecoder-seam-royalty-free-defaults.md)) | libtheora / Ogg-Theora (TheoraMovieDecoder) | `x3d_movie_tests` ✓ (shared per-backend contract, not bit-swap — see ADR-0041) | MovieTexture conformance blanks (MPEG-1 fixed) |
| Consumer (RenderDelta) | EXPERIMENTAL | PoC renderer / CAVE (1) | — pending | — | — |

Legend: **GREEN** = interface frozen + ≥2 independent backends + a CI-gated swap-test.
**NOT-YET-PROVEN** = one backend (or pending second) and no swap-test gate. `—` = not yet
applicable.

## GREEN row: ScriptEngine — the Phase-0 pilot

The `ScriptEngine` seam is the first row to go GREEN and is the **pattern** every later seam
card follows:

- **Interface frozen `[STABLE]`.** The 6-method abstract interface
  (`load` / `initialize` / `shutdown` / `prepareEvents` / `invoke` / `eventsProcessed`) carried
  two independent backends with **no signature change** — that is the empirical proof it is
  generic, so it was promoted `[EXPERIMENTAL]` → `[STABLE]` in
  [`include/x3d/sdk.hpp`](https://github.com/delta9000/x3d-cpp/blob/main/include/x3d/sdk.hpp).
  The whole Script/SAI surface (`ScriptEngine` / `ScriptSystem` / `SaiContext`) is part of the
  same frozen seam.
- **Backend A — Duktape** (`runtime/script/EcmaScriptBackend.{hpp,cpp}`): the originally
  shipped backend (vendored Duktape 2.7.0).
- **Backend B — QuickJS** (`runtime/script/QuickJsBackend.{hpp,cpp}`, quickjs-ng **v0.15.1**):
  a second, independent backend behind the `X3D_CPP_BUILD_QUICKJS` build option, with **no core
  `#ifdef`** — QuickJS meets the seam in a single isolated TU (`x3d_quickjs` static lib),
  linked PRIVATE so its headers/flags never leak to consumers. The default build (option OFF)
  is byte-identical and behavior-unchanged.
- **Swap-test — `x3d_quickjs_swap`** (`runtime/script/tests/quickjs_swap_test.cpp`): drives the
  *same* script fixtures through both backends and asserts **identical observable behavior** —
  the set of `(node, field, value)` writes the script drives into the cascade, plus the emitted
  ROUTE-target values, must match across backends for every fixture. Equality is observable
  (the runtime's marshalled field values, post-`toValue`), not internal, so per-backend JS-side
  coercion stays inside each backend. Gated in CI (see below).

### SCR conformance is backend-independent

The `SCR-*` findings in
[`findings.yaml`](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml)
(SCR-001 … SCR-007: §29.2 lifecycle ordering, prepareEvents/eventsProcessed read-back, full
field-type marshalling round-trips) are **behavioral conformance claims about the seam, not
about Duktape**. The `x3d_quickjs_swap` test runs the same fixtures through QuickJS and asserts
identical observable behavior, so it **evidences that the SCR conformance holds identically
under the second backend** — the claims are backend-independent. (The swap-test does not change
any finding status; it re-verifies the existing claims hold for a second implementation.)

## GREEN row: AssetResolver/IO — the Phase-1 proof

The `AssetResolver` seam is the second row to go GREEN (Phase-1):

- **Interface frozen `[STABLE]`.** The single `std::function<AssetResult(url, AssetKind)>`
  callback type carried two independent backends with **no signature change** — the empirical
  proof it is generic — so it was promoted `[EXPERIMENTAL]` → `[STABLE]` in
  [`include/x3d/sdk.hpp:50`](https://github.com/delta9000/x3d-cpp/blob/main/include/x3d/sdk.hpp).
  The whole `AssetResolver` / `AssetResult` / `AssetKind` / `AssetStatus` surface is part of
  the same frozen seam.
- **Backend A — libcurl HTTP** (`runtime/io/curl/HttpResolver.{hpp,cpp}`): synchronous
  `curl_easy_perform` path behind `X3D_CPP_BUILD_CURL` (OFF default, `find_package(CURL)`),
  with **no core `#ifdef`** — libcurl meets the seam in a single isolated TU (`x3d_curl`
  static lib), linked PRIVATE so its headers/flags never leak to consumers.
- **Backend B — AWS C++ SDK S3** (`runtime/io/s3/S3Resolver.{hpp,cpp}`): `GetObject` path
  behind `X3D_CPP_BUILD_S3` (OFF default, `find_package(AWSSDK REQUIRED COMPONENTS s3)`),
  with **no core `#ifdef`** — AWS SDK meets the seam in a single isolated TU (`x3d_s3`
  static lib), linked PRIVATE. Docker minio service in CI provides the S3-compatible
  fixture.
- **Swap-test — `x3d_assetresolver_swap`** (`runtime/io/tests/asset_resolver_swap_test.cpp`):
  in-process POSIX-socket HTTP server on 127.0.0.1 (Backend A's fixture) + `PutObject` to
  seed the minio bucket (Backend B's fixture) + loop asserting
  `resultA.bytes == resultB.bytes` byte-for-byte for every fixture, plus equal
  `AssetStatus::Failed` for missing keys. Gated in CI (see below).

### What the AssetResolver seam unblocks

The IO seam being proven generic removes the blocker on four P1 dependency cards (whose
findings currently note "Blocked on the asset-resolver/IO seam"):

- **NSN-*** — LoadSensor (NSN-1..7, NSN-9): **SHIPPED** (2026-07-17, ADR-0046).
  `LoadSensorSystem` (`runtime/events/LoadSensorSystem.hpp`) is the first SDK-side
  AssetResolver caller — a time-driven System observing per-tick child URL-object
  load state. IO-free: the SDK default is the null stub; the app injects the
  concrete backend (the CLI wires the SEC-3-confined local-file resolver). See
  `docs/wiki/subsystems/system-loadsensor.md`. (NSN-11 Anchor cases (b)/(c)
  deferred to the policy hook.)
- **PRF-6** — http/urn EXTERNPROTO: the parse-time `ProtoDeclarationResolver` becomes
  writeable for `http://` / `urn:` URLs (its own future swap-test is a separate card).
- **CONF-CRITIC-2** — Script external-URL: scripts with `@url` can fetch external bytes.
- **SCR-005** — Script autoRefresh / autoRefreshTimeLimit: periodic re-fetch becomes
  possible.

The AssetResolver card itself does **not** ship these — they become **workable** now that
the IO seam is proven generic; each flips its own findings when its card ships.

## GREEN row: TextureResolver — Phase-2 proof

The `TextureResolver` decode seam is the third row to go GREEN ([ADR-0024](decisions/0024-textureresolver-second-backend-swap-test.md)).
It completes the texture pipeline: **AssetResolver fetches the bytes (`url → bytes`),
TextureResolver decodes them (`bytes → RGBA8 pixels`)**.

- **Interface frozen `[STABLE]`.** The single
  `std::function<TexturePixelResult(const std::string& url)>` callback carried two
  independent decoders with **no signature change** — the empirical proof — so it was promoted
  `[EXPERIMENTAL]` → `[STABLE]` in
  [`include/x3d/sdk.hpp`](https://github.com/delta9000/x3d-cpp/blob/main/include/x3d/sdk.hpp).
  The whole `TextureResolver` / `TexturePixels` / `TexturePixelResult` / `TextureResolveStatus`
  surface is one frozen seam.
- **Backend A — stb_image** (`runtime/io/stb/StbTextureResolver.{hpp,cpp}`): the hand-written C
  decoder already vendored for the PoC, behind `X3D_CPP_BUILD_STB` (OFF default), with **no core
  `#ifdef`** — stb meets the seam in a single isolated TU (`x3d_stb` static lib), `stb_image.h`
  included PRIVATE so it never leaks. Honors bottom-left origin via
  `stbi_set_flip_vertically_on_load`.
- **Backend B — wuffs v0.3.4** (`runtime/io/wuffs/WuffsTextureResolver.{hpp,cpp}`): a
  memory-safe decoder transpiled from a DSL, **fully independent** of stb (shares zero code),
  behind `X3D_CPP_BUILD_WUFFS` (OFF default), with **no core `#ifdef`** — the vendored single-file
  amalgamation meets the seam in one isolated TU (`x3d_wuffs` static lib, PRIVATE,
  `WUFFS_IMPLEMENTATION` in that TU only). Decodes RGBA8 non-premultiplied, rows flipped to
  bottom-left to match Backend A and the seam contract.
- **Swap-test — `x3d_texture_tests`** (`runtime/io/tests/texture_decode_tests.cpp`, one grouped
  doctest binary): decodes the *same* lossless fixtures (**PNG / BMP / GIF / TGA**) through both
  backends and asserts **byte-equal `TexturePixels.rgba`** + equal `width`/`height` for every
  fixture, plus equal `Failed` status on a corrupt/missing input. The same binary holds the
  per-backend cases and the `makeMultiFormatTextureResolver` sniff-dispatch routing cases.
  Fully in-process — no service, no network, no fixture seeding (decoders take local bytes).

### The decode array — route by sniffed magic, never by `Failed`

A real `TextureResolver` is an **array of per-format decoders behind one seam** (stb is broad;
wuffs is memory-safe; a JPEG app might want libjpeg-turbo) — the decode analog of the fetch
seam's file/http/s3 routes. `makeMultiFormatTextureResolver(map<ImageFormat, TextureResolver>)`
(in `runtime/extract/MultiFormatTextureResolver.hpp`, std-only, decoder-free) **sniffs the input's
magic bytes and dispatches to the one registered decoder before calling it**, so each decoder only
ever sees input it owns and `Failed` stays unambiguous — no `Unsupported` status, the frozen seam
type is unchanged. (TGA has no leading magic, so it is routed by a conservative header check as a
last resort.) This is the rule [ADR-0024 §7] adopts and AssetResolver's `makeSchemeRouter` will
mirror.

### Scope honesty

Unlike AssetResolver, this seam unblocks **no specific conformance findings** — its value is
**thesis-completion** (the full fetch+decode texture path is now proven generic) plus
**memory-safe decode** of the untrusted bytes AssetResolver fetches. The byte-equal gate covers
PNG/BMP/GIF/TGA only; **PNM/PPM is deferred** (wuffs v0.3.x ships no Netpbm decoder — it lands in
v0.4), and **JPEG** byte-equality is impossible by construction (lossy IDCT) — a PSNR tolerance
companion is a separate optional card.

## GREEN row: Audio (AudioBackend) — Phase-3 proof

The `AudioBackend` DSP engine seam is the fourth row to go GREEN ([ADR-0026](decisions/0026-audiobackend-second-backend-swap-test.md)).

- **Interface frozen `[STABLE]`.** The abstract `AudioBackend` interface (`createNode` /
  `connect` / `setParam` / `render` / `renderStereo`) carried two independent backends with
  **no signature change** — the empirical proof it is generic — so it was promoted
  `[EXPERIMENTAL]` → `[STABLE]` in `runtime/sound/AudioBackend.hpp` (a runtime seam, not
  re-exported in `include/x3d/sdk.hpp`). The seam was extended in a prior task
  (`NodeKind::Panner`, positional `NodeParams` fields, `renderStereo`) — those extensions
  were driven by the spatial feature, not by Backend B.
- **Backend A — BuiltinDspBackend** (`runtime/sound/dsp/BuiltinDspBackend.{hpp,cpp}`): the
  originally shipped backend — always-built, dependency-free, RBJ-Cookbook synthesis,
  equal-power spatial pan. No flag required.
- **Backend B — MiniaudioBackend** (`runtime/sound/miniaudio/MiniaudioBackend.{hpp,cpp}`):
  miniaudio v0.11.x (fetched via CMake FetchContent from `mackron/miniaudio`,
  Unlicense OR MIT-0), behind `X3D_CPP_BUILD_MINIAUDIO` (OFF default), with **no core
  `#ifdef`** — miniaudio meets the seam in a single isolated TU (`x3d_miniaudio` static lib,
  `MINIAUDIO_IMPLEMENTATION` in that TU only, PRIVATE). The headless render path
  (`ma_node_graph_read_pcm_frames`) requires no audio device. The default build is
  unaffected; the dual license adds no compatibility concern.
- **Swap-test — `x3d_sound_swaptest`** (`runtime/sound/tests/sound_swap_test.cpp`):
  headless, in-process, no audio device. Two tiers:
  - **Synthesis tier (tight numeric)**: same `OscillatorSource → BiquadFilter → Gain →
    AudioDestination` fixture, both backends. Asserts RMS ±2%, Goertzel at the signal
    frequency ±5%, F2 in-band ±15% (the ±15% bound is explicit and honest — it reflects
    the RBJ-vs-independent-derivation filter-law delta at the filter knee).
  - **Spatial tier (structural invariants)**: same `OscillatorSource → Panner →
    Destination` fixture at multiple positions and all three `DistanceModel` modes. Asserts
    ear-sign (left source → L louder), centered symmetry (ahead source →
    `|rmsL − rmsR| < threshold`), and monotonic distance falloff across five distances
    per model. Energy allowed to differ by up to ±40% (equal-power vs. amplitude+1/d —
    two legitimately different panning laws agreeing on physical invariants, not on numbers).
  Gated in CI (see below).

### Scope honesty for the Audio seam

The synthesis proof is tight (numeric spectral agreement). The spatial proof is real-but-coarse:
two different panning laws agree on physical invariants, not on sample values. This is the right
approach for a spatial seam, and it is explicitly documented in ADR-0026.

The following remain **explicitly OUT** and deferred under SND-3 (partial):

- **HRTF spatialization** — no second reference HRTF renderer; LabSound deferred.
- **Doppler shift** — no second Doppler implementation.
- **§16 Sound ellipsoid** (`minFront`/`maxFront`/`minBack`/`maxBack`) — seam carries no
  ellipsoid params; SpatialSound does not fully map to `Panner`.

The `x3d_sound_swaptest` failing would mean one of the proven invariants broke — that is a
real regression, not test noise.

## GREEN row: FontMetrics — Phase-3 proof

The `FontMetrics` seam is the fourth row to go GREEN ([ADR-0025](decisions/0025-fontmetrics-second-backend-swap-test.md)).
It proves the font advance path is backend-agnostic: **the layout engine calls the seam; both
backends return the same advance**.

- **Interface frozen `[STABLE]`.** The single `std::function<GlyphResult(const FontKey&)>`
  callback carried two independent backends with **no signature change** — the empirical proof
  it is generic — so it was promoted `[EXPERIMENTAL]` → `[STABLE]` in
  [`include/x3d/sdk.hpp`](https://github.com/delta9000/x3d-cpp/blob/main/include/x3d/sdk.hpp).
  The whole `FontMetrics` / `FontKey` / `GlyphMetrics` / `GlyphResult` / `GlyphStatus` /
  `makeMonospaceStub` surface is one frozen seam.
- **Backend A — stb_truetype** (`runtime/io/stbtt/StbttFontMetrics.{hpp,cpp}`): the
  hand-written C truetype parser, behind `X3D_CPP_BUILD_STBTT` (OFF default), with **no core
  `#ifdef`** — stb_truetype meets the seam in a single isolated TU (`x3d_stbtt` static lib),
  `stb_truetype.h` included PRIVATE so it never leaks.
- **Backend B — FreeType** (`runtime/io/freetype/FreetypeFontMetrics.{hpp,cpp}`): the
  reference-quality font library via vcpkg, **fully independent** of stb_truetype (shares zero
  implementation), behind `X3D_CPP_BUILD_FREETYPE` (OFF default), with **no core `#ifdef`** —
  FreeType meets the seam in a single isolated TU (`x3d_freetype` static lib, PRIVATE).
- **Swap-test — `x3d_text_tests`** (`runtime/io/tests/font_metrics_tests.cpp`, one grouped
  doctest binary): queries the *same* Liberation font fixtures (**SERIF / SANS / TYPEWRITER** ×
  `{'A','i','M',' ','0'}`) through both backends and asserts **exact bit-identical `advanceEm`**
  (strict `==`, no epsilon) for every PLAIN-style glyph, plus equal `Failed` status on absent
  codepoints. 99/99 assertions passed. Dropped cells: synthetic BOLD/ITALIC (stb has no
  embolden/oblique API; both correctly return `Failed`); atlas-UV/outline (not font-table-derived).
  Fully in-process — Liberation TTFs are vendored, no service, no network.

### Exact-equality criterion

The `==` equality (no epsilon) is structurally guaranteed: both backends read the same raw
`uint16 unitsPerEm` from the font's `head` table and the same integer `hmtx` advance, then
compute `(float)advance / (float)unitsPerEm`. Because the operands are identical integers and
the operation is the same IEEE-754 `float / float` divide, the result is bit-identical on any
conformant hardware.

### Scope honesty

Like TextureResolver, this seam unblocks **no specific conformance findings** — its value is
**thesis-completion** (the font advance path is now proven generic) and providing a real
advance source for `Text` node layout. Synthetic BOLD/ITALIC and atlas-UV paths are explicitly
out of scope; they are separate future cards.

## CI gate

The genericity proof is a **permanent merge gate** — one job per GREEN row:

- **QuickJS seam swap-test** (`[.github/workflows/ci.yml](https://github.com/delta9000/x3d-cpp/blob/main/.github/workflows/ci.yml)`
  `quickjs-swap` job): `-DX3D_CPP_BUILD_QUICKJS=ON` (FetchContent fetches quickjs-ng
  v0.15.1) + `ctest -R 'x3d_quickjs(_backend|_swap)'` on every pull request.
- **AssetResolver seam swap-test** (`.github/workflows/ci.yml` `assetresolver-swap` job):
  `-DX3D_CPP_BUILD_CURL=ON -DX3D_CPP_BUILD_S3=ON` + docker `minio/minio:latest` service
  on port 9000 + `ctest -R 'x3d_assetresolver(_backend|_swap)'` on every pull request.
- **TextureResolver decode seam swap-test** (`.github/workflows/ci.yml` `texture-swap` job):
  `-DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_WUFFS=ON` + `ctest -R x3d_texture` on every pull
  request. Both decoders are vendored single files — no vcpkg, no docker, no FetchContent — so
  this gate runs sub-minute.
- **AudioBackend headless swap-test** (`.github/workflows/ci.yml` `audio-swap` job):
  `-DX3D_CPP_BUILD_MINIAUDIO=ON` + `ctest -R x3d_sound_swaptest` on every
  pull request. miniaudio is a vendored single file — no vcpkg, no docker, no audio device
  — sub-minute gate.
- **FontMetrics seam swap-test** (`.github/workflows/ci.yml` `fontmetrics-swap` job):
  `-DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_FREETYPE=ON` + `ctest -R x3d_text` on every pull
  request. stb_truetype is a vendored single file; FreeType is pulled via vcpkg. Liberation
  TTF fixtures are vendored — no network, no service container.

If a future change breaks observable parity between the two backends of any seam,
the corresponding job fails.

## How a seam goes GREEN (the recipe for the other rows)

For each NOT-YET-PROVEN row, the path to GREEN mirrors the ScriptEngine pilot:

1. Land a **second independent backend** behind a build option, with **no core `#ifdef`**
   (isolated TU, linked PRIVATE) — if the second backend forces an interface change, that is a
   *finding*, not a failure: the leak is what this exercise surfaces.
2. Write a **swap-test** that drives shared fixtures through both backends and asserts identical
   **observable** behavior.
3. **Gate it in CI** (a dedicated flag-gated job, like `QuickJS seam swap-test`).
4. Then **freeze the interface** `[STABLE]` (in `sdk.hpp` for public seams; in the seam header
   itself for runtime seams like `AudioBackend.hpp`) and flip the row GREEN here.

See `docs/contributor/card-to-done-workflow.md` (contributor material, outside
this wiki) for how a seam card is driven to this documented-completion state.

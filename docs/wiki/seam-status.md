---
title: Seam-Status Matrix
summary: Current seam interfaces, shipped backends, and contract or parity tests.
tags: [seam, tracker, genericity, swap-test, thesis, meta]
updated: 2026-09-30
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
  - decisions/0053-geo-projection-seam.md
  - subsystems/system-font-metrics.md
  - subsystems/system-script-sai.md
  - subsystems/system-texture-decode.md
---

# Seam-Status Matrix

This page records the shipped implementations and tests for each seam. The
asset, audio, font, and movie interfaces remain stable after removal of unused
second backends. Their remaining tests check behavior without claiming current
parity between two shipped implementations. See the amendments to the linked
decisions for the removals.

Behavior registration uses `System` and `X3DExecutionContext::addSystem`.
The unused per-node compatibility path has been removed; see the
[registration amendment](decisions/0011-ecs-flavored-not-ecs.md#amendment-2026-11-05).

## The matrix

| Seam | Interface | Backend A | Backend B | Swap-test | Findings |
|---|---|---|---|---|---|
| **ScriptEngine** | **STABLE** | Duktape (EcmaScriptBackend) | QuickJS / quickjs-ng v0.15.1 (QuickJsBackend) | `x3d_quickjs_swap` ✓ | SCR-* |
| **AssetResolver / IO** | **STABLE** | libcurl HTTP/FTP; local files | — | `x3d_assetresolver_swap` (HTTP contract) | Asset loading |
| Physics (PhysicsBackend) | EXPERIMENTAL | Jolt (flag-gated) | — pending | — | CONF-RBP* |
| **Audio (AudioBackend)** | **STABLE** | BuiltinDsp | — | Sound component tests | HRTF and Doppler deferred |
| **FontMetrics** | **STABLE** | stb_truetype (StbttFontMetrics) | — | `x3d_text_tests`, `x3d_text_atlas_tests` | Glyph metrics and atlas |
| **TextureResolver** | **STABLE** | stb_image (StbTextureResolver) | wuffs v0.3.4 (WuffsTextureResolver) | `x3d_texture_tests` ✓ | thesis-completion (no findings) |
| GeoProjection | Mathematical interface FROZEN (ADR-0053); context ownership per ADR-0058 | BuiltinGeoProjection | PROJ (`X3D_CPP_BUILD_PROJ`) | `x3d_proj_geo_swap` (toleranced) | Geospatial |
| **MovieDecoder** | **STABLE** | pl_mpeg / MPEG-1 | — | `x3d_movie_tests` (lifecycle and composition contract) | MPEG-1 playback |
| Consumer (RenderDelta) | EXPERIMENTAL | PoC renderer; CPU/SVG consumers | Native host CPU-mirror contract (not a renderer swap proof) | `x3d_native_host_contract` + installed/relocated package smoke | [Native host boundary](guides/native-host.md); Vulkan/CAVEOS unverified |

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
  This promise applies to the abstract `ScriptEngine` backend interface only.
  Concrete runtime ownership and author-field APIs are experimental; the
  [scene-owned field migration](decisions/0057-scene-owned-author-fields.md)
  removes the global store and requires concrete consumers to rebuild.
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

## AssetResolver/IO

The stable `AssetResolver` callback carries bytes and status. FileResolver and
the libcurl HTTP/FTP implementations remain. With `X3D_CPP_BUILD_CURL=ON`,
`x3d_assetresolver_swap` checks three HTTP fixtures against their source bytes
and verifies that a missing asset returns Failed with empty bytes. This test
uses an in-process loopback server and requires no external service.

The unused S3 backend was removed; see the
[decision amendment](decisions/0023-assetresolver-second-backend-swap-test.md#amendment-2026-11-05).

The libcurl backend's resolved-address guard also rejects IPv4 multicast,
reserved/broadcast, protocol-assignment and benchmarking ranges, including
embedded IPv4 in IPv6. `x3d_http_address_guard` checks these ranges without
making network requests.

### What the AssetResolver seam unblocks

The IO callback supports loading for the following runtime features:

- **NSN-*** — LoadSensor (NSN-1..7, NSN-9): **SHIPPED** (2026-07-17, ADR-0046).
  `LoadSensorSystem` (`runtime/events/LoadSensorSystem.hpp`) is the first SDK-side
  AssetResolver caller — a time-driven System observing per-tick child URL-object
  load state. IO-free: the SDK default is the null stub; the app injects the
  concrete backend (the CLI wires the SEC-3-confined local-file resolver). See
  `docs/wiki/subsystems/system-loadsensor.md`. (NSN-11 Anchor cases (b)/(c)
  deferred to the policy hook.)
- **PRF-6** — http/urn EXTERNPROTO: the parse-time `ProtoDeclarationResolver` becomes
  writeable for `http://` / `urn:` URLs. The two composer pieces now ship: `makeSchemeRouter`
  (`runtime/extract/SchemeRouter.hpp`) dispatches by URL scheme, and `protoResolverFrom`
  (`runtime/parse/AssetProtoResolver.hpp`) adapts an `AssetResolver` into a
  `ProtoDeclarationResolver`. urn resolves only when the router owns that scheme.
- **CONF-CRITIC-2** — Script external-URL: scripts with `@url` can fetch external bytes.
- **SCR-005** — Script autoRefresh / autoRefreshTimeLimit: periodic re-fetch becomes
  possible.

Each feature has its own behavior tests and conformance findings.

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
  surface is one frozen seam. [ADR-0059](decisions/0059-cube-map-images-through-texture-decode.md) added
  one defaulted member, `TexturePixels::layers` (1 for 2D, 6 for a cube image); the callback
  and lifecycle are unchanged and existing aggregate initialisers still compile.
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
seam's file/http/ftp routes. `makeMultiFormatTextureResolver(map<ImageFormat, TextureResolver>)`
(in `runtime/extract/MultiFormatTextureResolver.hpp`, std-only, decoder-free) **sniffs the input's
magic bytes and dispatches to the one registered decoder before calling it**, so each decoder only
ever sees input it owns and `Failed` stays unambiguous — no `Unsupported` status, the frozen seam
type is unchanged. (TGA has no leading magic, so it is routed by a conservative header check as a
last resort.) This is the rule [ADR-0024 §7] adopts and AssetResolver's `makeSchemeRouter`
(`runtime/extract/SchemeRouter.hpp`) mirrors.

### Scope honesty

Unlike AssetResolver, this seam unblocks **no specific conformance findings** — its value is
**thesis-completion** (the full fetch+decode texture path is now proven generic) plus
**memory-safe decode** of the untrusted bytes AssetResolver fetches. The byte-equal gate covers
PNG/BMP/GIF/TGA only; **PNM/PPM is deferred** (wuffs v0.3.x ships no Netpbm decoder — it lands in
v0.4), and **JPEG** byte-equality is impossible by construction (lossy IDCT) — a PSNR tolerance
companion is a separate optional card.

## Audio (AudioBackend)

`AudioBackend` remains stable. `BuiltinDspBackend` provides dependency-free
synthesis, equal-power stereo panning, distance attenuation, ellipsoid sound,
and decoded PCM playback. Sound component tests exercise the shipped behavior.
HRTF and Doppler remain deferred. The unused miniaudio backend and its parity
gate were removed; see the
[decision amendment](decisions/0026-audiobackend-second-backend-swap-test.md#amendment-2026-11-05).

## FontMetrics

`FontMetrics` remains stable. stb_truetype provides font-table metrics and a
glyph atlas through `x3d_stbtt`. Enable `X3D_CPP_BUILD_STBTT` to run
`x3d_text_tests` and `x3d_text_atlas_tests`. The unused FreeType backend and its
parity gate were removed; see the
[decision amendment](decisions/0025-fontmetrics-second-backend-swap-test.md#amendment-2026-11-05).

## MovieDecoder

`MovieDecoder` remains stable. `x3d_plmpeg` provides MPEG-1 frames and movie
MP2 audio. Enable `X3D_CPP_BUILD_PLMPEG` to run `x3d_movie_tests`, which covers
decoder lifecycle, a std-only test backend, and format composition. The unused
Theora implementation was removed; see the
[decision amendment](decisions/0041-moviedecoder-seam-royalty-free-defaults.md#amendment-2026-11-05).

## CI gate

The genericity proof is a **permanent merge gate** — one job per GREEN row:

- **QuickJS seam swap-test** (`[.github/workflows/ci.yml](https://github.com/delta9000/x3d-cpp/blob/main/.github/workflows/ci.yml)`
  `cpp` job): `-DX3D_CPP_BUILD_QUICKJS=ON` (FetchContent fetches quickjs-ng
  v0.15.1) + `ctest -R 'x3d_quickjs(_backend|_swap)'` on every pull request that touches C++.

- **TextureResolver decode seam swap-test** (`.github/workflows/ci.yml` `cpp` job):
  `-DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_WUFFS=ON` + `ctest -R x3d_texture` on every pull
  request that touches C++. Both decoders are vendored single files — no vcpkg, no docker, no FetchContent — so
  this gate runs sub-minute.

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

### GeoProjection ownership boundary

The mathematical swap-test and unchanged virtual interface do not freeze the
concrete runtime layout or the removed process-global selection API.
[ADR-0058](decisions/0058-context-owned-geo-projection.md) makes the backend a
constructor-fixed context/session owner and requires explicit references in
shared conversion helpers. Existing consumers must migrate and rebuild.
World-isolation tests complement numerical backend parity; neither proof is a
claim of complete Geospatial or SAI conformance.

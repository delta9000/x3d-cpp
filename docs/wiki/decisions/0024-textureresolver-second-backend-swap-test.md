---
title: "ADR-0024: Second TextureResolver Backend (wuffs) + Decode Swap-Test as the Genericity Proof"
summary: An interface is *proven generic* only when a second, independent backend runs identical fixtures to identical observable behavior, gated in CI — applied to the TextureResolver decode seam by adding a wuffs decoder alongside an stb_image decoder and a CI-gated swap-test asserting byte-identical RGBA over lossless image formats. The pattern from ADR-0022 / ADR-0023; completes the texture pipeline (AssetResolver fetches the bytes, TextureResolver decodes them).
tags: [adr, seam, genericity, textureresolver, decode, stb-image, wuffs, swap-test, ci-gate, thesis]
updated: 2026-06-23
related:
  - ../architecture.md
  - ../seam-status.md
  - 0023-assetresolver-second-backend-swap-test.md
  - 0022-scriptengine-second-backend-swap-test.md
---

# ADR-0024: Second TextureResolver Backend (wuffs) + Decode Swap-Test as the Genericity Proof

## Status

Accepted (2026-06-23). Implemented on `feat/textureresolver-decode`: Backend A
(stb_image, `runtime/io/stb/StbTextureResolver.cpp`, `-DX3D_CPP_BUILD_STB=ON`) and
Backend B (wuffs v0.3.4, `runtime/io/wuffs/WuffsTextureResolver.cpp`,
`-DX3D_CPP_BUILD_WUFFS=ON`) decode each lossless fixture (PNG/BMP/GIF/TGA) to
byte-identical RGBA8 under the `x3d_texture_tests` swap-test, gated by the
`texture-swap` CI job. The seam is frozen `[STABLE]` in `include/x3d/sdk.hpp`.
PNM was dropped from the byte-equal matrix — wuffs v0.3.x has no Netpbm decoder
(it lands in v0.4); see §3 and "Deferred".

## Context

The product thesis is that x3d-cpp is **unopinionated and pluggable**: every place a
renderer/engine plugs in is an abstract **seam**, and the runtime core stays spec-correct and
backend-free. The `TextureResolver` seam (`runtime/extract/TextureResolver.hpp`) is the
**decode path**: how a CONSUMER turns image bytes into decoded RGBA8 pixels
(`TexturePixels{width, height, rgba}`, tight stride, bottom-left origin). It is the structural
twin of, and natural successor to, the `AssetResolver` seam ([ADR-0023](0023-assetresolver-second-backend-swap-test.md)):
AssetResolver fetches the bytes (`url → bytes`); TextureResolver decodes them (`bytes → pixels`).
Together they are the complete texture pipeline.

Today the only concrete decode is the PoC's stb_image call in `examples/poc_renderer/main.cpp`;
the only seam in production code is the `std::function` type itself plus `makeNullTextureResolver()`
(the always-`Failed` stub). One implementation is not a proof. The way to find out whether the
interface is generic is to make a **second, independent** decoder satisfy the same contract and
prove the two are interchangeable by **behavior**, not by inspection.

This is the third seam to take that proof, after [ADR-0022](0022-scriptengine-second-backend-swap-test.md)
(ScriptEngine: Duktape + QuickJS) and [ADR-0023](0023-assetresolver-second-backend-swap-test.md)
(AssetResolver: libcurl + S3). It is the **lowest-friction** next seam — both decoders are
single-file, header-style libraries with no service container and no vcpkg cold-build, so the CI
gate is fully in-process and deterministic.

The two backends chosen are **stb_image** and **wuffs**. They share zero implementation
(stb_image is a hand-written C single-header decoder; wuffs is C transpiled from a memory-safe DSL,
with caller-owned buffers and no allocations), so a parity failure would surface a real interface
or format-handling leak, not a coincidence. wuffs additionally brings **compile-time-proven memory
safety** to the untrusted-bytes path that AssetResolver feeds — image decoders are a historically
heavy CVE source.

### The exact-equality boundary (why lossless only)

The swap-test's strength is asserting **byte-identical** decoded pixels. That holds only for
**lossless** formats, where the spec defines exact pixel values and two correct decoders must
agree. The swap matrix is the **lossless intersection** of the two decoders' codec sets: with the
vendored **wuffs v0.3.4** that intersection is **PNG, BMP, GIF, TGA**. PNM/PPM, though lossless,
is **not** in the gate — wuffs v0.3.x ships no Netpbm decoder (it lands in v0.4); stb has one, but
a matrix entry needs *both* backends. **Lossy** formats (JPEG, …) decode with impl-defined IDCT
and chroma upsampling, so two correct decoders legitimately differ; those are explicitly **out of
the byte-equal gate** and, if covered at all, belong in a separate PSNR/tolerance companion test.

## Decision

**An interface is _proven generic_ only when a second independent backend runs identical fixtures
to identical observable behavior, gated in CI** (the ADR-0022/0023 rule, restated for this seam).

Concretely, for the `TextureResolver` decode seam:

- **Backend A — stb_image** (`runtime/io/stb/StbTextureResolver.{hpp,cpp}`), behind the
  `X3D_CPP_BUILD_STB` build option, with **no core `#ifdef`** — stb_image meets the seam in a
  single isolated TU (`x3d_stb` static lib, the decoder header included PRIVATE so it never leaks
  to consumers). The default build (option OFF) is byte-identical and behavior-unchanged. Honors
  the seam's bottom-left origin via `stbi_set_flip_vertically_on_load`.
- **Backend B — wuffs** (`runtime/io/wuffs/WuffsTextureResolver.{hpp,cpp}`), behind
  `X3D_CPP_BUILD_WUFFS`, with **no core `#ifdef`** — the wuffs single-file amalgamation meets the
  seam in a single isolated TU (`x3d_wuffs` static lib, PRIVATE). Same bottom-left V-flip.
- **Decode swap-test** (`x3d_texture_tests`, one grouped doctest binary): decodes the *same*
  lossless image fixtures (PNG / BMP / GIF / TGA) through Backend A and Backend B and asserts
  **byte-equal `TexturePixels.rgba`** plus equal `width`/`height` for every fixture, and equal
  `Failed` status for a corrupt/missing input. The same binary holds the U1/U2 per-backend cases
  and the U2.5 `makeMultiFormatTextureResolver` sniff-dispatch routing cases. Equality is
  observable (the pixels the resolver hands back), never internal. Fully in-process — no service
  container, no network.
- **Permanent CI merge gate**: a `TextureResolver seam swap-test` job in
  `.github/workflows/ci.yml`, flag-gated `-DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_WUFFS=ON`,
  scoped build of only the `x3d_texture_tests` target, `ctest -R x3d_texture`. No vcpkg, no
  docker, no FetchContent — both decoders are vendored single files, so the gate runs sub-minute.
- Because the interface carried two backends with no signature change, it is promoted
  `[EXPERIMENTAL]` → `[STABLE]` in `include/x3d/sdk.hpp` — the whole `TextureResolver` /
  `TexturePixels` / `TextureResolveStatus` surface as one frozen seam.
- **Attribution:** `NOTICE` lists stb_image (MIT OR Public Domain) and **wuffs (Apache-2.0)** with
  their build flags. wuffs stays flag-gated OFF by default so the default SDK remains MIT/PD-clean
  (Apache-2.0 is permissive but incompatible with GPLv2-only downstreams).

## Consequences

**Positive:**

- The genericity claim for `TextureResolver` is now **empirical**, not aspirational — and the
  permanent CI gate keeps it true. The texture pipeline (AssetResolver + TextureResolver) is proven
  generic end-to-end.
- The swap-test is the **cheapest** of the seam gates: no docker, no vcpkg, deterministic, sub-minute.
- wuffs brings **memory-safe decoding** to the untrusted bytes AssetResolver fetches — a real
  security posture improvement for the consumer.
- A genericity *leak* becomes a **finding**, not a silent risk: if wuffs ever forces a
  `TextureResolver` signature change, or the two decoders disagree on a format (palette expansion,
  BMP row order, GIF first-frame), the swap-test fails loudly.

**Trade-offs / costs:**

- Two more vendored single-file libraries (stb_image already present for the PoC; wuffs new).
  Apache-2.0 enters the optional-dependency set — mitigated by the OFF-default flag.
- The byte-equal gate covers **lossless formats only**; JPEG parity (if ever wanted) is a separate
  tolerance test, documented as not part of this gate.
- Two decoders must stay in lockstep on the seam's pixel contract (RGBA8, tight stride,
  bottom-left) — though the swap-test makes divergence a loud CI failure.
- No specific conformance findings unblock (unlike AssetResolver); the value is thesis-completion
  and the proven texture pipeline.

## Related

- [Seam-Status Matrix](../seam-status.md) — the live tracker this ADR turns the TextureResolver row green in.
- [ADR-0023: Second AssetResolver/IO Backend (S3 SDK) + Swap-Test](0023-assetresolver-second-backend-swap-test.md)
  — the fetch half of the texture pipeline; this ADR is the decode half.
- [ADR-0022: Second ScriptEngine Backend (QuickJS) + Swap-Test](0022-scriptengine-second-backend-swap-test.md)
  — the original pattern.
- Design spec: `docs/superpowers/specs/2026-06-23-textureresolver-decode-design.md`.

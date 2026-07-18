---
title: "ADR-0025: Second FontMetrics Backend (FreeType) + Advance Swap-Test as the Genericity Proof"
summary: An interface is *proven generic* only when a second, independent backend runs identical fixtures to identical observable behavior, gated in CI — applied to the FontMetrics seam by adding a FreeType backend alongside an stb_truetype backend and a CI-gated swap-test asserting exact bit-identical advanceEm over Liberation font fixtures (PLAIN style). The pattern from ADR-0022 / ADR-0023 / ADR-0024; completes the font-metrics pipeline (layout engine calls the seam, both backends return the same advance).
tags: [adr, seam, genericity, fontmetrics, freetype, stb-truetype, swap-test, ci-gate, thesis]
updated: 2026-06-24
related:
  - ../architecture.md
  - ../seam-status.md
  - 0024-textureresolver-second-backend-swap-test.md
  - 0023-assetresolver-second-backend-swap-test.md
  - 0022-scriptengine-second-backend-swap-test.md
---

# ADR-0025: Second FontMetrics Backend (FreeType) + Advance Swap-Test as the Genericity Proof

## Status

Accepted (2026-06-24). Implemented on `feat/fontmetrics-seam-proof`: Backend A
(stb_truetype, `runtime/io/stbtt/StbttFontMetrics.cpp`, `-DX3D_CPP_BUILD_STBTT=ON`) and
Backend B (FreeType via vcpkg, `runtime/io/freetype/FreetypeFontMetrics.cpp`,
`-DX3D_CPP_BUILD_FREETYPE=ON`) return **exact bit-identical `advanceEm`** for every
PLAIN-style codepoint in the Liberation font fixtures, asserted by the `x3d_text_tests`
swap-test, gated by the `fontmetrics-swap` CI job. The seam is frozen `[STABLE]` in
`include/x3d/sdk.hpp`.

## Context

The product thesis is that x3d-cpp is **unopinionated and pluggable**: every place a
renderer/engine plugs in is an abstract **seam**, and the runtime core stays spec-correct and
backend-free. The `FontMetrics` seam (`runtime/extract/FontMetrics.hpp`) is the **font
advance path**: how the layout engine obtains per-codepoint advance widths and optional atlas
UV / outline data from a consumer-supplied callback. The layout engine calls
`FontMetrics(FontKey{family, style, codepoint})` and reads `GlyphResult.metrics.advanceEm`
to compute text extent and character placement; it never opens a font file or calls a
rasterizer.

Before this ADR the only concrete implementation was a **monospace stub**
(`makeMonospaceStub()`, `advanceEm = 0.6f` always). One stub is not a proof. The way to find
out whether the interface is generic is to make two **independent** real backends satisfy the
same contract and prove they are interchangeable by **behavior**, not by inspection.

This is the fourth seam to take the genericity proof, after
[ADR-0022](0022-scriptengine-second-backend-swap-test.md) (ScriptEngine: Duktape + QuickJS),
[ADR-0023](0023-assetresolver-second-backend-swap-test.md) (AssetResolver: libcurl + S3), and
[ADR-0024](0024-textureresolver-second-backend-swap-test.md) (TextureResolver: stb_image +
wuffs). It shares the same four-step recipe established in ADR-0022.

The two backends chosen are **stb_truetype** and **FreeType**. stb_truetype is a
single-header C truetype parser already adjacent to the PoC renderer's stb_image; FreeType is
the reference-quality font library available via vcpkg. They share **zero implementation**, so
a parity failure would surface a real interface or font-table handling leak, not a coincidence.

### The exact-equality criterion (why advance is bit-identical)

The swap-test asserts **exact bit-identical `advanceEm`** with **no epsilon or tolerance**,
and it held across all 99 tested (family × codepoint) combinations. This equality is
structurally guaranteed, not coincidental: both backends read the same raw `uint16`
`unitsPerEm` from the font's `head` table and the same integer `hmtx` advance for each
codepoint, then compute:

```
advanceEm = (float)hmtxAdvance / (float)unitsPerEm;
```

Because the operands are the same integers and the operation is the same IEEE-754 `float /
float` divide, the result is bit-identical on any IEEE-754-conformant hardware. This is
materially different from the texture decode seam (where `byte-equal` holds only for lossless
formats and only after careful pixel-contract alignment) — for advance widths the equality is
guaranteed by the data source and the division arithmetic.

### Dropped cells in the swap matrix

The swap matrix covers **PLAIN style only**:

- **Synthetic BOLD/ITALIC** — out of scope. stb_truetype has no embolden or oblique API; the
  `FontFaceMap` used by both backends is constructed with PLAIN faces only. Both backends
  return `Failed` for `BOLD`, `ITALIC`, and `BOLDITALIC` requests — that is the correct
  contract (they were never loaded), not a parity failure. Synthetic style variants (emboldening,
  obliquing) are a separate future card.
- **Atlas UV / outline** — out of scope. Neither backend fills `hasAtlasUv = true` in this
  proof; atlas upload is an embedder-side integration that does not derive from the font tables.
  The swap-test covers the `advanceEm` path only; the atlas-UV / outline paths are a separate
  card. Extruded/outline 3D text geometry (turning glyph outlines into extruded Text geometry,
  rather than a flat glyph quad) has been a long-standing, recurring author request in X3D
  content workflows — a real constituency for this deferred card, not a purely hypothetical one.

### The `.notdef` contract rule

A codepoint absent from the font's `cmap` (glyphIndex == 0 → `.notdef`) returns
`GlyphStatus::Failed`, not a fallback advance. Both backends enforce this: when the cmap
lookup yields glyph index 0 (the `.notdef` sentinel), the factory returns
`GlyphStatus::Failed`. The swap-test verifies this parity on `U+1F600` (😀), which is absent
from Liberation fonts.

## Decision

**An interface is _proven generic_ only when a second independent backend runs identical
fixtures to identical observable behavior, gated in CI** (the ADR-0022/0023/0024 rule,
restated for this seam).

Concretely, for the `FontMetrics` seam:

- **Backend A — stb_truetype** (`runtime/io/stbtt/StbttFontMetrics.{hpp,cpp}`), behind the
  `X3D_CPP_BUILD_STBTT` build option, with **no core `#ifdef`** — stb_truetype meets the seam
  in a single isolated TU (`x3d_stbtt` static lib, the decoder header included PRIVATE so it
  never leaks to consumers). The default build (option OFF) is byte-identical and
  behavior-unchanged. Takes a `FontFaceMap` (`map<string,string>` of family → TTF path),
  opened once at construction.
- **Backend B — FreeType** (`runtime/io/freetype/FreetypeFontMetrics.{hpp,cpp}`), behind
  `X3D_CPP_BUILD_FREETYPE` (via vcpkg), with **no core `#ifdef`** — FreeType meets the seam
  in a single isolated TU (`x3d_freetype` static lib, PRIVATE). Same `FontFaceMap` interface.
  FreeType is initialized once per construction and faces are cached.
- **Advance swap-test** (`x3d_text_tests`, one grouped doctest binary): queries the *same*
  Liberation font fixtures (`SERIF`, `SANS`, `TYPEWRITER` families × `{'A','i','M',' ','0'}`
  codepoints) through Backend A and Backend B and asserts **exact bit-identical `advanceEm`**
  (strict `==`, no epsilon) for every PLAIN-style glyph, and equal `Failed` status for absent
  codepoints. 99/99 assertions passed. The same binary holds the U1/U2 per-backend cases and
  the `.notdef` contract cases. Fully in-process — no service container, no network.
- **Permanent CI merge gate**: a `fontmetrics-swap` job in `.github/workflows/ci.yml`,
  flag-gated `-DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_FREETYPE=ON`,
  scoped build of only the `x3d_text_tests` target, `ctest -R x3d_text`.
- Because the interface carried two backends with no signature change, it is promoted
  `[EXPERIMENTAL]` → `[STABLE]` in `include/x3d/sdk.hpp` — the whole `FontMetrics` /
  `FontKey` / `GlyphMetrics` / `GlyphResult` / `GlyphStatus` / `makeMonospaceStub` surface as
  one frozen seam.
- **Attribution:** `NOTICE` lists `stb_truetype.h` (MIT OR Public Domain) and the Liberation
  Fonts fixtures (OFL-1.1) as vendored entries; FreeType (FTL OR GPL-2.0) as an optional
  build-time dependency.

## Consequences

**Positive:**

- The genericity claim for `FontMetrics` is now **empirical**, not aspirational — and the
  permanent CI gate keeps it true.
- The exact-equality criterion is stronger than the texture swap-test's byte-equal criterion:
  advance values are guaranteed identical by IEEE-754 arithmetic on shared integer inputs, not
  merely empirically confirmed.
- A genericity *leak* becomes a **finding**, not a silent risk: if FreeType ever forces a
  `FontMetrics` signature change, the swap-test fails loudly.
- The Liberation OFL fixtures are small TTF files, fully vendored — no network, no platform
  font service. The CI gate runs in-process, sub-minute.

**Trade-offs / costs:**

- FreeType enters the optional-dependency set via vcpkg (`-DX3D_CPP_BUILD_FREETYPE=ON`); it is
  dual-licensed FTL OR GPL-2.0. Attribution applies to binaries that link it. The default build
  (OFF) is unaffected.
- stb_truetype is added as a vendored single-header file (MIT OR Public Domain). It is
  already adjacent to stb_image in the PoC renderer's third-party tree.
- Synthetic BOLD/ITALIC and atlas-UV paths are explicitly out of scope for this proof; they
  are separate future cards.
- No specific conformance findings unblock (unlike AssetResolver); the value is
  thesis-completion.

## Related

- [Seam-Status Matrix](../seam-status.md) — the live tracker this ADR turns the FontMetrics row green in.
- [ADR-0024: Second TextureResolver Backend (wuffs) + Decode Swap-Test](0024-textureresolver-second-backend-swap-test.md)
  — the immediately preceding seam proof; this ADR follows the same pattern.
- [ADR-0023: Second AssetResolver/IO Backend (S3 SDK) + Swap-Test](0023-assetresolver-second-backend-swap-test.md)
- [ADR-0022: Second ScriptEngine Backend (QuickJS) + Swap-Test](0022-scriptengine-second-backend-swap-test.md)
  — the original pattern.
- Design spec: `docs/superpowers/specs/2026-06-24-fontmetrics-seam-genericity-design.md`.

# FontMetrics seam — second backend + metrics swap-test (design spec)

**Date:** 2026-06-24
**Seam:** `FontMetrics` (`runtime/extract/FontMetrics.hpp`) — the per-codepoint glyph-metrics path.
**Decision:** [ADR-0025](../../wiki/decisions/0025-fontmetrics-second-backend-swap-test.md) (to be authored)
**Pattern:** mirrors the TextureResolver card
([ADR-0024](../../wiki/decisions/0024-textureresolver-second-backend-swap-test.md),
spec `2026-06-23-textureresolver-decode-design.md`) and the ScriptEngine/AssetResolver cards
([ADR-0022](../../wiki/decisions/0022-scriptengine-second-backend-swap-test.md),
[ADR-0023](../../wiki/decisions/0023-assetresolver-second-backend-swap-test.md)).
**Status:** proposed plan (card-ready, design approved 2026-06-24).

---

## 1. Thesis & goal

x3d-cpp is unopinionated and pluggable: every renderer/engine plug-in point is an abstract seam,
**proven generic only when a second independent backend runs identical fixtures to identical
observable behavior, gated in CI** (ADR-0022). This card applies that proof to the `FontMetrics`
seam — the consumer-supplied per-codepoint glyph-metrics callback that the text-extraction path
(`TextExtract::buildTextMesh`) uses to lay out `Text` geometry.

**Goal:** add two independent real backends behind the existing `FontMetrics` seam
(**stb_truetype** and **FreeType**), prove they are interchangeable by a swap-test asserting
identical observable behavior over shared vendored font fixtures, gate it in CI, and freeze the
interface `[EXPERIMENTAL]` → `[STABLE]`.

**Why this seam is the right next pick.** It can be an *exact-equality* seam (the strongest proof
type) for its core observable, and one of its backends (stb_truetype) is a vendored single-header
drop-in matching the proven stb_image/wuffs model — no service-container or cold-build fights for at
least one half of the proof.

## 2. Seam contract (exists today; this card adds documented adapter rules)

```cpp
using FontMetrics = std::function<GlyphResult(const FontKey&)>;   // runtime/extract/FontMetrics.hpp
```

- **`FontKey`** = `{ std::string family; std::string style; std::uint32_t codepoint; }` —
  `family` ∈ {`SERIF`,`SANS`,`TYPEWRITER`} (or named), `style` ∈
  {`PLAIN`,`BOLD`,`ITALIC`,`BOLDITALIC`} (empty → `PLAIN`), `codepoint` is UTF-32.
- **`GlyphResult`** = `{ GlyphStatus status; GlyphMetrics metrics; }` with `Ready`/`Pending`/`Failed`
  and `makeReady/makePending/makeFailed` factories.
- **`GlyphMetrics`** = `{ float advanceEm; bool hasAtlasUv; float u0,v0,u1,v1; }`. `advanceEm` =
  horizontal advance ÷ em-size; the layout engine scales it by `FontStyle.size`.

Today the only backend is `makeMonospaceStub()` (always `Ready`, `advanceEm = 0.6f`, no atlas UV).
The seam types already mirror TextureResolver, so the seam is structurally ready for real backends.

**Live consumer (proves the seam is real):** `TextExtract::buildTextMesh(const X3DNode&, const
FontMetrics&, …)` calls `fm(FontKey{family, style, cp})` once per codepoint
(`runtime/extract/TextExtract.hpp:213`, `:275`); `makeLayoutMetricsAdapter` bridges the
per-codepoint seam down to `TextLayout`'s per-string `FontMetricsCallback`. The monospace stub stays
as the SDK-internal default; the two new backends are flag-gated additions, not replacements.

### 2.1 Adapter contract rules (NEW — documented in `FontMetrics.hpp`)

These four rules are what make the two backends observably identical. They are part of what this card
freezes into the contract:

1. **Unscaled metrics only.** FreeType: `FT_LOAD_NO_SCALE`; stb_truetype: the inherently-unscaled
   `stbtt_GetCodepointHMetrics`. No hinting, no grid-fitting.
2. **Raw integer em size + one identical divide.** Read `head.unitsPerEm` as the raw integer from
   both libraries and compute `advanceEm = float(advance) / float(unitsPerEm)` — **not**
   stb's `1 / stbtt_ScaleForMappingEmToPixels(...)` reciprocal, which inserts an extra rounding step.
3. **Missing glyph → Failed.** `glyphIndex == 0` (`.notdef`) ⇒ `makeFailed()` in both adapters. Glyph
   0 carries a valid `hmtx` advance, so without this rule both backends would return
   `Ready(.notdef-advance)`; the rule yields a clean exact-`Failed` parity cell and consistent
   missing-glyph semantics for consumers.
4. **No atlas UV.** Both adapters set `hasAtlasUv = false`; the atlas/raster path is out of scope.

## 3. The swap matrix (the heart of the proof)

Assertion boundary: the **`FontMetrics` callback** — the raw `GlyphResult` for a given `FontKey`
(consistent with every prior seam asserting at the seam's direct output).

| Observable | FontKey combo | Assertion | Rationale |
|---|---|---|---|
| `advanceEm` | family ∈ {SERIF,SANS,TYPEWRITER}, **PLAIN**, codepoint **present** in cmap | **EXACT-EQUAL** (tolerance fallback — see §3.1) | Both backends read the same `hmtx` advance + `head.unitsPerEm` integers; adapter rules 1–2 make the float divide identical. |
| status | **PLAIN**, codepoint **absent** | **EXACT `Failed` parity** | Adapter rule 3 (`glyphIndex==0 → Failed`) in both backends. |
| `advanceEm` | **BOLD / ITALIC / BOLDITALIC** | **OUT-OF-SCOPE v1** | stb_truetype has **no** synthetic embolden/oblique API; FreeType synthesizes and *changes* the advance. They cannot agree by construction — the font analogue of the trimmed wuffs-Netpbm cell. Recorded in the ADR. |
| `hasAtlasUv`, `u0..v1` | any | **OUT-OF-SCOPE** | Atlas UVs are embedder/atlas-packing-defined, not font-table-derived; both adapters set `hasAtlasUv=false` (assertion excludes u/v). |
| `Pending` status | n/a | **OUT-OF-SCOPE v1** | Belongs to the async rasterization/atlas path a metrics-only proof excludes. |
| kerning | n/a | **NOT APPLICABLE** | `FontKey` is a single codepoint, not a pair. |

### 3.1 Equality model

**Exact, with a documented tolerance fallback.** Primary assertion is bit-identical `advanceEm` for
the PLAIN/present cell, achieved via adapter rules 1–2. *If and only if* implementation reveals a
persistent last-ULP gap (a genuine float-determinism difference between the two libraries' integer
reads), fall back to a tight epsilon and **record the gap and the chosen epsilon in ADR-0025**.
Tolerance is a weaker proof; exact is preferred and expected to be attainable.

## 4. Scope boundaries (YAGNI)

- **Metrics-only proof is sufficient** to flip the row GREEN: `advanceEm` is the only
  font-table-derived observable; the atlas/raster path (`hasAtlasUv`, UVs, `Pending`) is not a
  cross-backend equality target by construction.
- **Synthetic BOLD/ITALIC are out of scope for v1** (see matrix). Proving native bold/italic faces
  would require a style→face adapter convention the seam does not currently express — a future card.
- **`makeMultiFontMetrics` composer deferred** to a separate follow-up card (precedented by
  `makeMultiFormatTextureResolver`, but not required for the genericity proof).

## 5. Work breakdown (U1–U4, per the proven pattern)

Backend home follows the TextureResolver precedent exactly: the seam header stays in
`runtime/extract/`; the backends live under `runtime/io/` (as stb_image/wuffs do for the
`runtime/extract/TextureResolver.hpp` seam).

### U1 / U2 — the two isolated backend libs

- **CMake flags** (`option(... OFF)`): `X3D_CPP_BUILD_STBTT`, `X3D_CPP_BUILD_FREETYPE` — per the
  `X3D_CPP_BUILD_<BACKEND>` convention (texture set is `_STB`/`_WUFFS`).
- **Backend A — stb_truetype** (`runtime/io/stbtt/`): vendor `stb_truetype.h` (MIT OR Public Domain);
  one TU `StbttFontMetrics.cpp` does `#define STB_TRUETYPE_IMPLEMENTATION` then includes it; returns
  `std::function<GlyphResult(const FontKey&)>`. `add_library(x3d_stbtt STATIC …)`; seam header PUBLIC,
  vendored header PRIVATE, `-w` on the vendored TU. Mirrors `x3d_stb`.
- **Backend B — FreeType** (`runtime/io/freetype/`): one TU `FreetypeFontMetrics.cpp`;
  `add_library(x3d_freetype STATIC …)`; FreeType acquired via **vcpkg** (`find_package(Freetype)` /
  the vcpkg-exported target), heavy headers linked PRIVATE; the std-only seam header PUBLIC.
- Both adapters implement the §2.1 rules. Each backend header carries the
  "Core MUST NEVER include this file" banner (template: `StbTextureResolver.hpp`).

### U3 — the swap-test

- One grouped doctest binary `x3d_text_tests` (analogue of `x3d_texture_tests`), built only
  `if(TARGET x3d_stbtt AND TARGET x3d_freetype AND X3D_CPP_BUILD_TESTS)`.
- Holds per-backend cases + the swap-test asserting the §3 matrix over the vendored fixture fonts.
- **ABI-skew rule (AssetResolver lesson):** the test TU includes **no** backend header — only the
  seam factories, which exchange std-only `FontKey`/`GlyphResult`. Naturally satisfied here.
- Fonts wired via `target_compile_definitions(... FIXTURES_DIR="…")`.

### U4 — freeze + flip + gate

- **`include/x3d/sdk.hpp`** — two-site edit: flip the `FontMetrics` include-line comment and the
  using-block comment `[EXPERIMENTAL]` → `[STABLE]` with the proof citation (template: the texture
  STABLE line + the ScriptEngine frozen prose block).
- **`docs/wiki/seam-status.md`** — flip the FontMetrics row GREEN (both backend names + `x3d_text_tests`),
  add a GREEN-row prose section + CI-gate bullet, bump `updated:` and `related:` (cite ADR-0025).
- **ADR-0025** — author from the ADR-0024 template: restate the rule, name backends/paths/flags, the
  swap-test target/fixtures/**equality criterion (record exact-vs-tolerance outcome + dropped
  synthetic-style cells)**, the CI job, the `[STABLE]` promotion, the NOTICE rows.
- **Docs** — new `docs/wiki/subsystems/system-font-metrics.md` (template `system-texture-decode.md`);
  add a 'covered' row to `coverage.md` + the ADR row; add `mkdocs.yml` nav entries for both pages.
  `docs-build --strict` fails on nav orphans / dead links, so all land in the same diff.

## 6. Build & CI plan

- **stb_truetype:** vendored single-header (license embedded; no separate license file needed).
- **FreeType:** acquired via **vcpkg** (user decision — version-pinned + consistent with the S3 job).
  The CI job **must enable vcpkg binary caching** to blunt the cold-build cost the seam-genericity
  memory warns about (AssetResolver burned time on cold vcpkg AWS-SDK builds).
- **Flag-gating:** both flags default OFF; `mise run ci` does **not** enable them. The card **must add
  a dedicated CI job** (template: the texture-swap job): toolchain + vcpkg FreeType (cached);
  `cmake -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_FREETYPE=ON`; build target `x3d_text_tests`;
  `ctest -R x3d_text --output-on-failure`.
- **Fixture fonts:** vendored **Liberation (OFL-1.1)** covering SERIF/SANS/TYPEWRITER (single clean
  SPDX id → simple NOTICE; metric-compatible with Times/Arial/Courier). Downloaded fresh from upstream
  and vendored (never copied from a system path); full OFL text reproduced alongside the binaries
  (wuffs precedent: `WUFFS_LICENSE.txt`). Fixed codepoint set, e.g. `'A','i','M',' ','0'` present +
  ≥1 deliberately-absent codepoint for the `.notdef` cell.

## 7. NOTICE (part of Definition of Done)

`docs-drift` does **not** catch NOTICE staleness; update it in the same diff:

- **Section 2 (Vendored):** `stb_truetype.h` (MIT OR Public Domain, `-DX3D_CPP_BUILD_STBTT=ON`);
  the Liberation fonts (OFL-1.1).
- **Section 3 (Optional fetched/installed):** `FreeType … FTL OR GPL-2.0 (-DX3D_CPP_BUILD_FREETYPE=ON)`
  — state **FTL** to keep the stack permissive.

## 8. Risks & prior-lesson traps

1. **Synthetic-style trap (the wuffs-Netpbm analogue).** stb cannot synthesize BOLD/ITALIC;
   designed around by making the exact gate PLAIN-only. Do **not** naively add BOLD cells.
2. **Exact-equality ULP risk.** Bit-identical `advanceEm` is conditional on adapter rules 1–2; the
   ULP caveat is the top open risk. Mitigation: adapter discipline + the §3.1 tolerance fallback.
3. **.notdef silent-Ready trap.** Without adapter rule 3 an absent codepoint returns
   `Ready(.notdef-advance)` on both backends — "parity" proving the wrong contract. Rule 3 is a shared,
   documented convention.
4. **vcpkg cold-build cost.** Chosen deliberately; mitigated by CI binary caching (§6).
5. **ABI-skew trap.** Test TU includes no backend headers (naturally satisfied — std-only seam types).
6. **Default-build license cleanliness.** Both backends OFF by default keeps the shipped SDK
   license-clean (non-MIT/PD deps stay flag-gated OFF).

## 9. Definition of Done

- [ ] `x3d_stbtt` + `x3d_freetype` backend libs (flag-gated OFF, heavy dep PRIVATE, no core `#ifdef`).
- [ ] Adapter rules 1–4 implemented in both backends and documented in `FontMetrics.hpp`.
- [ ] `x3d_text_tests` swap-test asserts the §3 matrix; exact-equality (or documented tolerance).
- [ ] Dedicated CI job (vcpkg FreeType + binary cache) gates the swap-test, green.
- [ ] `FontMetrics` frozen `[EXPERIMENTAL]` → `[STABLE]` in `include/x3d/sdk.hpp`.
- [ ] seam-status row GREEN; ADR-0025 Accepted; `system-font-metrics.md` + coverage/nav rows.
- [ ] NOTICE updated (stb_truetype, FreeType-FTL, Liberation fonts).
- [ ] `mise run docs-drift` reviewed; `mise run docs-build` (strict) green; RAG stores refreshed.

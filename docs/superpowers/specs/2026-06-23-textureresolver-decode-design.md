# TextureResolver decode seam — second backend + decode swap-test (design spec)

**Date:** 2026-06-23
**Seam:** `TextureResolver` (`runtime/extract/TextureResolver.hpp`) — the image **decode** path.
**Decision:** [ADR-0024](../../wiki/decisions/0024-textureresolver-second-backend-swap-test.md)
**Pattern:** mirrors the AssetResolver card
([ADR-0023](../../wiki/decisions/0023-assetresolver-second-backend-swap-test.md),
spec `2026-06-23-assetresolver-http-design.md`).
**Status:** proposed plan (not yet a card).

---

## 1. Thesis & goal

x3d-cpp is unopinionated and pluggable: every renderer/engine plug-in point is an abstract seam,
proven generic only when **a second independent backend runs identical fixtures to identical
observable behavior, gated in CI**. This card applies that proof to the `TextureResolver` decode
seam — the natural successor to AssetResolver. Together they are the full texture pipeline:
AssetResolver does `url → bytes`; TextureResolver does `bytes → RGBA8 pixels`.

**Goal:** add a second, independent image decoder behind the existing `TextureResolver` seam,
prove the two decoders are interchangeable by a byte-equal swap-test, gate it in CI, and freeze the
interface `[EXPERIMENTAL]` → `[STABLE]`.

## 2. Seam contract (already exists, unchanged)

`TextureResolver = std::function<TextureResult(const std::string& url, TextureKind)>` returning
`TexturePixels{ uint32 width, uint32 height, vector<uint8> rgba }` — RGBA8, tightly packed
(stride = width·4), **bottom-left origin** (GL convention), with the same `Ready` / `Pending` /
`Failed` lifecycle as AssetResolver contract (A). `makeNullTextureResolver()` is the always-`Failed`
stub. The seam header is std-only and a leaf; **no signature change is expected** (the whole point
of the proof — if a backend forces one, that is a finding).

## 3. Backend choice

| | Backend A | Backend B |
|---|---|---|
| Library | **stb_image** (single-header C) | **wuffs** (single-file C, transpiled from a memory-safe DSL) |
| Already in repo | yes (`examples/poc_renderer/third_party/stb_image.h`) | no — vendor the amalgamation |
| License | MIT OR Public Domain | **Apache-2.0** |
| Independence | hand-written C decoder | caller-owned buffers, zero-alloc, provably memory-safe |

They share zero implementation, so a parity failure is a real leak. wuffs additionally brings
**memory-safe decoding** to the untrusted bytes AssetResolver fetches.

### The exact-equality boundary

Byte-equal RGBA holds only for **lossless** formats. The swap matrix is the lossless intersection
of the two decoders: **PNG, BMP, GIF, TGA, PNM/PPM**. **JPEG and other lossy formats are excluded**
from the byte-equal gate (impl-defined IDCT/upsampling); a PSNR tolerance test is a possible
separate follow-up, never part of this gate. Fixtures are controlled **8-bit** images (no 16-bit,
no `gAMA`/`iCCP` color-management chunks) to keep decode deterministic across libraries.

## 4. Units (one PR on `feat/textureresolver-decode`, U1–U4 as commits)

**U1 — Backend A: stb_image decoder.**
`x3d_stb` STATIC lib behind `X3D_CPP_BUILD_STB` (OFF default). Single TU
`runtime/io/stb/StbTextureResolver.cpp` is the only place stb_image meets the seam (header included
PRIVATE; `StbTextureResolver.hpp` is decoder-free). `makeStbTextureResolver()` returns a
`TextureResolver`. Honors bottom-left origin (`stbi_set_flip_vertically_on_load(1)`). Per-backend
unit test: URL/format rejection + a known fixture decodes to expected dimensions.

**U2 — Backend B: wuffs decoder.**
Vendor the wuffs single-file amalgamation. `x3d_wuffs` STATIC lib behind `X3D_CPP_BUILD_WUFFS`
(OFF default). Single TU `runtime/io/wuffs/WuffsTextureResolver.cpp` (PRIVATE include,
`WUFFS_IMPLEMENTATION` in this TU only). `makeWuffsTextureResolver()`. Same bottom-left V-flip.
Per-backend unit test mirrors U1.

**U2.5 — multi-format composer (the array case).**
A std-only `makeMultiFormatTextureResolver(map<ImageFormat, TextureResolver>)` in
`runtime/extract/` (decoder-free) that **sniffs the input's magic bytes** (`\x89PNG`, `\xFF\xD8\xFF`
JPEG, `GIF8`, `BM`, PNM `P1..P6`, …), picks the *one* registered decoder for that format, and calls
it. Because dispatch happens **before** calling a decoder, each decoder is only ever handed input it
owns, so `Failed` keeps meaning "real decode failure" — no `Unsupported` status is needed and the
seam type is unchanged. Unit test: routes PNG→Backend B, BMP/TGA→either, etc.; returns `Failed`
for an unsniffable blob. (See §7.)

**U3 — decode swap-test + CI gate.**
`x3d_texture_swap` ctest: for each lossless fixture (PNG/BMP/GIF/TGA/PNM), decode through A and B
and assert **byte-equal `rgba` + equal `width`/`height`**; plus **Failed parity** on a
corrupt/unsupported input. Test TU talks only through the seams (`makeStbTextureResolver` /
`makeWuffsTextureResolver`) — no decoder headers in the test TU (the AssetResolver ABI-skew lesson;
trivially clean here since the factories exchange std types). New CI job `texture-swap`:
`-DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_WUFFS=ON`, **scoped build** of only the texture targets
(`cmake --build build --target x3d_texture_backend_a_test x3d_texture_backend_b_test
x3d_texture_swap`), `ctest -R 'x3d_texture(_backend|_swap)'`. No services, no vcpkg.

**U4 — freeze + docs + attribution.**
Promote `TextureResolver` `[EXPERIMENTAL]` → `[STABLE]` in `include/x3d/sdk.hpp`. Flip the
seam-status matrix row GREEN. New subsystem page `docs/wiki/subsystems/system-texture-decode.md`
(+ `mkdocs.yml` nav + `coverage.md` row). ADR-0024 → Accepted. **Add stb_image + wuffs to `NOTICE`**
(wuffs Apache-2.0; both flag-gated).

## 5. Definition of Done

- [ ] Spec + ADR-0024 on a dated branch (this).
- [ ] `TextureResolver` frozen `[EXPERIMENTAL]` → `[STABLE]`.
- [ ] Backend A (stb_image) behind `X3D_CPP_BUILD_STB`, no core `#ifdef`.
- [ ] Backend B (wuffs) behind `X3D_CPP_BUILD_WUFFS`, no core `#ifdef`.
- [ ] **Swap-test green**: byte-equal RGBA over PNG/BMP/GIF/TGA/PNM + Failed parity, in CI.
- [ ] `makeMultiFormatTextureResolver` sniff-dispatch composer + routing test (§7).
- [ ] seam-status matrix row GREEN + new `system-texture-decode` subsystem page.
- [ ] **`NOTICE` updated** for stb_image + wuffs (per the card→done DoD).
- [ ] `mise run docs-build` (strict) green.

## 6. Why this seam, and why it'll be cheap

- **Cleanest proof:** lossless decode is exact-equality, like AssetResolver's byte-equal bytes —
  no tolerance fudging (which the Physics/FontMetrics/GeoProjection seams would force).
- **Lowest friction:** both decoders are single-file; no minio, no vcpkg cold build, no ABI-skew
  trap (factories exchange `std::vector<uint8_t>`). The gate should run sub-minute from day one.
- **Pipeline completion:** proves the fetch+decode texture path generic end-to-end.
- **Honest caveat:** unblocks no specific conformance findings; value is thesis-completion +
  memory-safe decode of untrusted bytes.

## 7. Multi-backend composition — backends are naturally an array

A single decoder backend rarely covers every format an embedder needs (stb is broad; wuffs is
memory-safe; a JPEG-heavy app might want libjpeg-turbo). So a real `TextureResolver` is an **array
of format decoders behind one seam**. This is the decode analog of the fetch seam's `file`/`http`/
`s3` routes.

**How AssetResolver did it (and the wart to avoid):** the seam is a single `std::function`; each
backend is scheme-scoped and returns `Failed` for "not my scheme" (`HttpResolver` → `http(s)://`,
`S3Resolver` → `s3://`); composition was left to the embedder, and no router was shipped. The wart:
`Failed` is overloaded — "wrong route" vs "right route, genuinely failed" — which is ambiguous for a
try-and-fall-through chain. It never bit because no multi-backend chain was wired.

**The rule this seam adopts: route by a cheap key _before_ calling; never use `Failed` to route.**

- Fetch routes by URL **scheme** (lexical prefix — decidable before the call).
- Decode routes by **sniffed magic bytes** (decidable before the call — the URL extension can lie).

Because the dispatcher selects the one correct backend up front, each backend only ever sees input
it owns, so `Failed` stays unambiguous and **no `Unsupported`/`NotHandled` status is required** — the
frozen `Ready`/`Pending`/`Failed` interface composes cleanly. The "array" is a thin std-only
**composer** (`makeMultiFormatTextureResolver`, U2.5), not a change to the seam type. The genericity
swap-test stays orthogonal: it proves two decoders agree byte-for-byte on shared formats; the
composer is the production wiring.

**Consistency follow-up:** AssetResolver should get the same treatment — a `makeSchemeRouter(
map<scheme, AssetResolver>)` helper (additive; the seam is already frozen). Tracked as a card in the
[GitHub Project](https://github.com/users/delta9000/projects/2).

## 8. Deferred / out of scope

- JPEG (and other lossy) byte-equal parity — impossible by construction; a PSNR tolerance companion
  test is a separate optional card.
- 16-bit / HDR / color-managed decode.
- Replacing the vendored `tinfl.h` DEFLATE path with wuffs' zlib/deflate (a possible future
  dependency consolidation — wuffs ships those codecs — but explicitly out of scope here).

## 9. Execution notes for the implementing session (handoff)

Captured at the end of the planning session so a fresh session starts warm. These reflect lessons
from the AssetResolver seam (ADR-0023) and the doctest test rollout that both landed on `main` after
this spec was first drafted.

- **doctest from the start.** The repo's tests are now grouped doctest binaries, not per-file
  `assert()`/`main()` (see `main`: `runtime/test_support/doctest/`, `scripts/doctest_convert.py`,
  `scripts/doctest_regroup.py`). Write the new texture tests as `TEST_CASE`s and register ONE grouped
  `x3d_texture_tests` binary (doctest_main.cpp + the cases), not N separate executables. Use
  `CHECK((expr))` (decomposition off) for reliability. Per-backend U1/U2 tests can be `TEST_CASE`s in
  the same grouped binary or tiny separate ones — your call.
- **The ABI-isolation lesson (why the swap-test TU must be decoder-free).** On AssetResolver, the
  swap-test TU `#include`d the AWS SDK directly and segfaulted at runtime because it compiled against a
  *different* SDK header than the linked lib (system vs vcpkg). Decoders here are vendored single
  headers so the version-skew risk is lower, but keep the discipline: the swap-test TU talks ONLY
  through the seam factories (`makeStbTextureResolver`/`makeWuffsTextureResolver`) — never include
  `stb_image.h` or the wuffs header in the test TU. If a fixture-seeding helper is ever needed (it
  isn't here — decoders take bytes directly), put it behind an AWS-free-style helper lib like
  `x3d_s3_testsupport`.
- **This gate is trivial vs AssetResolver — do NOT reach for the heavy machinery.** stb + wuffs are
  vendored single headers. There is **no vcpkg, no docker/minio, no service container, no FetchContent**.
  The CI job is just: apt the toolchain, configure with `-DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_WUFFS=ON`,
  scoped build of the texture targets, `ctest -R 'x3d_texture'`. Expect a sub-minute gate. (Contrast the
  AssetResolver gate's minio/vcpkg fight — none of that applies.)
- **NOTICE is part of DoD.** Add stb_image (MIT/PD; already vendored for the PoC) and **wuffs
  (Apache-2.0)** to `NOTICE` in the same change. Keep wuffs flag-gated OFF so the default build stays
  MIT/PD-clean.
- **Vendoring wuffs without network friction.** `git clone --depth 1 --branch <tag>
  https://github.com/google/wuffs` works (github HTTPS is reachable); copy the single-file
  amalgamation from `release/c/`. Confirm its current codec list (PNG/BMP/GIF/TGA/PNM at least) before
  locking the swap matrix.
- **Env/ops quirks in this workspace.** SSH push is broken — use `gh`/HTTPS
  (`git push https://github.com/delta9000/x3d-cpp.git HEAD:<branch>`; `gh pr ...`). Merges are squash
  (so stacked branches need rebasing after a parent merges). The Workflow tool's `args` does not survive
  the VM boundary cleanly — hardcode lists in the script or read them from a file on disk.
- **Branch hygiene.** Start the implementation on a fresh `feat/textureresolver-decode` off `main`,
  and cherry-pick this spec + ADR-0024 as the opening commit (the card→done convention: spec+ADR on the
  feature branch, merged with the implementation — not a standalone plan merge). `docs/textureresolver-plan`
  is just the draft holder.

---
title: Texture Decode Seam
summary: The TextureResolver decode seam (bytes → RGBA8 pixels) proven generic by two independent backends — stb_image + wuffs — and a CI-gated byte-equal swap-test over lossless formats; with a std-only multi-format sniff-dispatch composer.
tags: [subsystem, texture, decode, seam, stb-image, wuffs, swap-test, genericity, thesis]
updated: 2026-06-23
related:
  - ../architecture.md
  - ../seam-status.md
  - ../subsystems/extract-textures.md
  - ../subsystems/system-asset-io.md
  - ../decisions/0024-textureresolver-second-backend-swap-test.md
  - ../decisions/0023-assetresolver-second-backend-swap-test.md
---

# Texture Decode Seam

## Purpose

The `TextureResolver` seam turns **image bytes into decoded RGBA8 pixels**. It is the second
half of the texture pipeline and the structural twin of the [Asset Resolver / IO
seam](system-asset-io.md): **AssetResolver fetches the bytes (`url → bytes`), TextureResolver
decodes them (`bytes → RGBA8 pixels`)**. The SDK itself **never decodes an image** — the seam is
a consumer-supplied callback, so the spec-correct runtime core carries no codec. The decoded
surface is `TexturePixels{ width, height, rgba }`: RGBA8, tightly packed (stride = width·4),
**bottom-left origin** (GL convention), with the `Ready` / `Pending` / `Failed` lifecycle shared
with AssetResolver.

The seam type is a single value callback:

```cpp
using TextureResolver =
    std::function<TexturePixelResult(const std::string& url)>;
```

`makeNullTextureResolver()` is the always-`Failed` stub (the PoC renders a white fallback).
`PixelTexture` (inline `SFImage` data) is already decoded and never goes through this seam — only
`TextureRef::Source::Url` does.

!!! note "Seam proven generic — two independent decoders (stb_image + wuffs)"
    The decode seam is **GREEN** ([ADR-0024](../decisions/0024-textureresolver-second-backend-swap-test.md)):
    two fully independent decoders implement the *unchanged* `TextureResolver` interface, and a
    CI-gated swap-test asserts they decode every lossless fixture to **byte-identical RGBA8**.
    Because the interface carried two backends with **no signature change**, it was promoted
    `[EXPERIMENTAL]` → `[STABLE]` in `include/x3d/sdk.hpp`. See the
    [Seam-Status Matrix](../seam-status.md).

## Key files

| File / directory | Role |
|---|---|
| `runtime/extract/TextureResolver.hpp` | The seam: `TextureResolver` callback type, `TexturePixels` / `TexturePixelResult` / `TextureResolveStatus`, `makeNullTextureResolver()`. std-only, leaf, decoder-free. |
| `runtime/extract/MultiFormatTextureResolver.hpp` | std-only **composer**: `makeMultiFormatTextureResolver(map<ImageFormat, TextureResolver>)` + `sniffImageFormat()`. Routes by sniffed magic bytes before calling a decoder. Decoder-free. |
| `runtime/io/stb/StbTextureResolver.{hpp,cpp}` | **Backend A** — stb_image. `makeStbTextureResolver()`. The `.cpp` is the one TU that `#include`s `stb_image.h` (PRIVATE). Built behind `X3D_CPP_BUILD_STB`. |
| `runtime/io/wuffs/WuffsTextureResolver.{hpp,cpp}` | **Backend B** — wuffs. `makeWuffsTextureResolver()`. The `.cpp` is the one TU that defines `WUFFS_IMPLEMENTATION` and `#include`s the vendored amalgamation (PRIVATE). Built behind `X3D_CPP_BUILD_WUFFS`. |
| `runtime/io/wuffs/vendor/wuffs-v0.3.c` | The vendored wuffs v0.3.4 single-file amalgamation (Apache-2.0; see `WUFFS_LICENSE.txt`). |
| `runtime/io/tests/texture_decode_tests.cpp` | The grouped doctest binary `x3d_texture_tests`: per-backend cases, the byte-equal swap-test, Failed-parity, and the composer routing cases. Talks only through the seam factories — no decoder headers. |
| `runtime/io/tests/fixtures/texture/` | Controlled 8-bit fixtures (PNG/BMP/GIF/TGA) + `generate_fixtures.py` + corrupt/garbage inputs. |

## The two backends

Both backends read the file at `url` and decode **from memory** (identical IO path, so the
swap-test isolates the *decoder* as the only variable), emit 4-channel straight-alpha RGBA8, and
flip to bottom-left origin to honor the seam contract.

- **Backend A — stb_image** (`x3d_stb` static lib, `X3D_CPP_BUILD_STB`, OFF default). The
  hand-written C decoder already vendored for the PoC renderer. Bottom-left via
  `stbi_set_flip_vertically_on_load(1)`. `stb_image.h` is included **PRIVATE** in the single TU,
  so consumers linking `x3d_stb` inherit no stb headers — the public `StbTextureResolver.hpp` is
  decoder-free. **No core `#ifdef`.**
- **Backend B — wuffs v0.3.4** (`x3d_wuffs` static lib, `X3D_CPP_BUILD_WUFFS`, OFF default). A
  **memory-safe** decoder transpiled from a DSL — it shares **zero implementation** with stb, so a
  byte-equal parity failure is a real correctness leak, not shared-bug masking. Uses the
  `wuffs_aux::DecodeImage` C++ API with a `SelectPixfmt` override forcing `RGBA_NONPREMUL`; rows
  are flipped to bottom-left after decode. `WUFFS_IMPLEMENTATION` and the amalgamation live in the
  single TU only. **No core `#ifdef`.** wuffs additionally brings memory-safe decoding to the
  untrusted bytes AssetResolver fetches.

## The genericity proof — byte-equal decode swap-test

`x3d_texture_tests` (`runtime/io/tests/texture_decode_tests.cpp`) decodes the **same** lossless
fixtures through both backends and asserts **byte-equal `TexturePixels.rgba`** + equal
`width`/`height`, plus equal `Failed` status on corrupt/missing input. The test TU talks **only
through the seam factories** and includes no decoder header — the ABI-isolation discipline from
the [AssetResolver seam](system-asset-io.md) (whose swap-test once segfaulted on AWS SDK
header-vs-lib skew). Gated by the `texture-swap` CI job
(`-DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_WUFFS=ON`, scoped build, `ctest -R x3d_texture`) — no
vcpkg, no docker, no FetchContent, so it runs sub-minute.

### The exact-equality boundary

Byte-equal RGBA holds only for **lossless** formats. The swap matrix is the lossless intersection
of the two decoders' codec sets: with the vendored wuffs v0.3.4 that is **PNG, BMP, GIF, TGA**.

- **PNM/PPM** is excluded — lossless, but wuffs v0.3.x ships no Netpbm decoder (it lands in v0.4);
  a matrix entry needs *both* backends.
- **JPEG** (and other lossy) byte-equality is impossible by construction (impl-defined IDCT /
  chroma upsampling); a PSNR tolerance companion is a separate optional card.

Fixtures are controlled 8-bit images with no `gAMA`/`iCCP` color-management chunks, so decode is
deterministic across libraries.

## Backends are an array — route by sniffed magic, never by `Failed`

A real `TextureResolver` is an **array of per-format decoders behind one seam** (stb is broad;
wuffs is memory-safe; a JPEG-heavy app might want libjpeg-turbo) — the decode analog of the fetch
seam's `file`/`http`/`s3` routes. `makeMultiFormatTextureResolver` (std-only, decoder-free)
**sniffs the input's magic bytes and dispatches to the one registered decoder _before_ calling
it**. Because dispatch picks the correct decoder up front, each decoder only ever sees input it
owns, so `Failed` keeps meaning "real decode failure" — **no `Unsupported`/`NotHandled` status is
needed and the frozen `Ready`/`Pending`/`Failed` seam type is unchanged**.

- Fetch routes by URL **scheme** (a lexical prefix); decode routes by **sniffed magic bytes** (the
  URL extension can lie).
- PNG/JPEG/GIF/BMP/PNM have leading magic. **TGA has none** (its only signature is an 18-byte
  *trailer*), so it is routed last by a conservative header plausibility check — the same
  compromise stb and wuffs make internally.

This mirrors the rule [ADR-0024 §7] adopts; AssetResolver's `makeSchemeRouter` is the carded
follow-up that applies the same treatment to the fetch seam.

## Scope / what this seam does and doesn't unblock

Unlike AssetResolver, the decode seam unblocks **no specific conformance findings** — its value is
**thesis-completion** (the full fetch+decode texture path is now proven generic end-to-end) plus
**memory-safe decode** of untrusted bytes. The PoC's `examples/poc_renderer/main.cpp` stb_image
call is unchanged; this seam is the SDK-side, swap-tested decode path that a production embedder
wires in. Out of scope: lossy/JPEG parity, 16-bit/HDR/color-managed decode, and replacing the
vendored `tinfl.h` DEFLATE path with wuffs' zlib (a possible future dependency consolidation).

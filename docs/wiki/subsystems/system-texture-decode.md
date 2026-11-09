---
title: Texture Decode Seam
summary: TextureResolver pixels, optional stb and Wuffs decoders, DDS layers, and format routing.
tags: [subsystem, texture, decode, seam, stb-image, wuffs]
updated: 2026-10-02
related:
  - ../architecture.md
  - ../seam-status.md
  - ../subsystems/extract-textures.md
  - ../subsystems/system-asset-io.md
  - ../decisions/0024-textureresolver-second-backend-swap-test.md
  - ../decisions/0023-assetresolver-second-backend-swap-test.md
---

# Texture decoding

`runtime/extract/TextureResolver.hpp` defines `TextureResolver`, a host callback
from URL to `TexturePixelResult`. The result is `Ready`, `Pending`, or `Failed`.
Ready pixels are tightly packed straight-alpha RGBA8 with a bottom-left origin.
The default null resolver always fails.

Inline PixelTexture pixels are already decoded. URL-backed textures use the
resolver supplied by the host. See [texture extraction](extract-textures.md).

## Implementations

Enable the corresponding CMake option and link its library:

| Option | Library | Factory header |
|---|---|---|
| `X3D_CPP_BUILD_STB` | `x3d_stb` | `runtime/io/stb/StbTextureResolver.hpp` |
| `X3D_CPP_BUILD_WUFFS` | `x3d_wuffs` | `runtime/io/wuffs/WuffsTextureResolver.hpp` |

The no-argument factories load local files. The stb overload taking an `AssetResolver` fetches bytes, propagates
Pending/Failed, and decodes Ready bytes in memory. Decoder-library types remain private to the implementations.

## DDS and cube-map images

Both implementations dispatch `DDS ` bytes to `runtime/io/dds/DdsDecode.hpp`.
It supports first-level 2D images and six-face cube maps with uncompressed
channel masks or BC1/BC2/BC3 compression. Cube layers use X3D order: front,
back, left, right, top, bottom; a face absent from a partial cube decodes as
opaque white. Unsupported arrays, volumes, and truncated data fail.

## Format routing

`runtime/extract/MultiFormatTextureResolver.hpp` supplies
`makeMultiFormatTextureResolver` and `sniffImageFormat()`. It chooses a registered
decoder by magic bytes before invoking it. TGA uses a conservative header check.
A selected decoder's `Failed` result remains a decode failure; it does not mean
try every other decoder.

## Decode comparison boundary

`runtime/io/tests/texture_decode_tests.cpp` compares stb and Wuffs results on
controlled 8-bit PNG, BMP, GIF, and TGA fixtures without color-management chunks.
These lossless fixtures require equal dimensions and RGBA bytes. JPEG is outside
this exact-equality boundary. DDS exercises a shared decoder, so equality there
is not independent implementation evidence.

The decision is recorded in
[ADR-0024](../decisions/0024-textureresolver-second-backend-swap-test.md).

---
title: "ADR-0059: Cube-Map Images Through the Texture Decode Seam"
summary: An ImageCubeMapTexture is one url Source::Cube ref; the TextureResolver decodes its single file to six layers (TexturePixels::layers) in X3D face order, with a std-only DDS decoder shared by the stb and wuffs backends.
tags: [adr, textureresolver, decode, cube-map, dds, seam]
updated: 2026-10-10
related:
  - 0024-textureresolver-second-backend-swap-test.md
  - ../subsystems/system-texture-decode.md
  - ../subsystems/extract-textures.md
---

# ADR-0059: Cube-Map Images Through the Texture Decode Seam

## Status

Accepted.

## Problem

§34.4.3 ImageCubeMapTexture names one image file that holds all six faces of a
cube map, and recommends Microsoft DDS as the format. The SDK extracted it as a
plain 2D url ref, the decode seam returned one 2D image, and no bundled decoder
read DDS, so the node never rendered (REQ-CUBE).

## Decision

- `MaterialSystem::refOf` maps ImageCubeMapTexture to `TextureRef::Source::Cube`
  with its `url` (honouring `load`) and no `cubeFaces`. `resolveTextureRefs`
  resolves such a ref through the embedder's `TextureResolver` like a url ref;
  a ComposedCubeMapTexture (with `cubeFaces`) still resolves face by face.
- `TexturePixels` gains `layers` (default 1). A cube image decodes to
  `layers = 6` equal faces stored one after another in X3D face order front,
  back, left, right, top, bottom, each bottom-left origin and upright as seen
  from the cube's centre. The callback type and the Ready/Pending/Failed
  lifecycle are unchanged; existing 2D producers and consumers are unaffected.
- The SDK core still decodes nothing. `runtime/io/dds/DdsDecode.hpp` is a
  std-only, header-only DDS decoder that both decode backends (`x3d_stb`,
  `x3d_wuffs`) call when the bytes start with `DDS `. It covers uncompressed
  8/16/24/32-bit masks and BC1-BC3, by FourCC or DX10 header, first mip only.
- DDS lays cube faces out for a left-handed space (+X, -X, +Y, -Y, +Z, -Z). X3D
  is right-handed, so the decoder mirrors z: front = +Z, back = -Z, left = -X,
  right = +X, top = +Y, bottom = -Y. Each face is then upright from inside,
  matching the ComposedCubeMapTexture and Background panorama convention.

## Consequences

Both reference hosts consume a six-layer image exactly like six composed faces.
Other cube formats (KTX, a cross-layout PNG) can be added to a backend without a
seam change. A 2D consumer handed a six-layer image by mistake reads the first
layer (front). The decoder is not part of the stb/wuffs byte-equal swap matrix,
since both backends run the same code; `texture_dds_cube_and_2d` checks each.

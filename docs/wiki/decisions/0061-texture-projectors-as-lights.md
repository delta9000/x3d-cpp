---
title: "ADR-0061: Texture Projectors as Textured Lights"
summary: TextureProjector and TextureProjectorParallel surface as world-resolved ProjectorDesc values beside lights; both reference hosts light the scene through them as unattenuated lights filtered by the projected texel.
tags: [adr, texture-projection, lighting, render-feed, consumer]
updated: 2026-10-11
related:
  - 0060-generated-cube-maps-local-frame.md
  - ../subsystems/extract-textures.md
---

# ADR-0061: Texture Projectors as Textured Lights

## Status

Accepted.

## Problem

§42 texture projectors are light nodes (X3DTextureProjectorNode derives from
X3DLightNode) that "project a texture onto geometry", but the standard does not
say how the projected texture combines with the surface's own shading, and the
§17 lighting equation has no projector term. The SDK also had no way to hand a
consumer a projector: `LightDesc` has no texture, no projection volume and no
near/far range.

## Decision

- **A separate descriptor.** `SceneExtractor::projectors()` returns one
  `ProjectorDesc` per active projector, collected by
  `LightSystem::collectProjectors` with the same scene walk as lights (Switch
  and LOD selection, per-path transforms, `global`/`scopeRoot`). `LightDesc` is
  unchanged, so existing consumers do not light the scene with a projector by
  accident.
- **World-resolved volume.** The descriptor holds the world location,
  direction and up vector, the view and projection matrices, and near/far
  distances and parallel extents scaled by the local frame. The up vector is
  `upVector` for TextureProjector and +Y for TextureProjectorParallel; when it is
  parallel to the direction it falls back to +Y, then +Z. `fieldOfView` covers
  the shorter side of the image, as Viewpoint's does. `ProjectorDesc::project()`
  maps a world point to (s, t) or rejects it.
- **Lighting model.** A projector lights a surface like a light without
  attenuation: TextureProjector is positional at `location`, and
  TextureProjectorParallel is directional along `direction`. Its
  `color` x `intensity` (and `ambientIntensity`) is multiplied by the projected
  texel's rgb x alpha inside the volume, white when no texture resolves, and is
  zero outside the volume, behind the projector, or outside
  `nearDistance`/`farDistance`. This plugs into the §17 per-light sum unchanged,
  so it works the same way in Phong and PBR.
- **aspectRatio output.** `TextureProjectorSystem` emits `aspectRatio` for a
  PixelTexture, whose size the runtime knows. For a url texture the runtime does
  not decode the image, so only `ProjectorDesc.aspectRatio` (from the resolved
  pixels) carries it.

## Consequences

Projected colour scales with the surface's diffuse colour and its angle to the
projector, so a projector on a black or back-facing surface shows nothing.
Projector shadows are carried in the descriptor but not rendered. A MultiTexture
projector texture has no TextureRef, and author shaders do not receive
projectors. The GL PoC binds up to three projectors (texture units 13-15) and
shares the eight light slots with ordinary lights, which take them first.

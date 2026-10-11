---
title: "ADR-0060: Generated Cube Maps and the Local Cube Frame"
summary: GeneratedCubeMapTexture is a Source::Cube ref the consumer renders (TextureRef::generatedCube) with a runtime System for NEXT_FRAME_ONLY; every cube map is fixed to its geometry's local frame, so eye-space lookup directions are carried into that frame.
tags: [adr, cube-map, environment-texture, render-feed, consumer]
updated: 2026-10-10
related:
  - 0059-cube-map-images-through-texture-decode.md
  - ../subsystems/extract-textures.md
---

# ADR-0060: Generated Cube Maps and the Local Cube Frame

## Status

Accepted.

## Problem

§34.4.2 GeneratedCubeMapTexture is rendered from the scene at run time, so no
decoder can supply it: the consumer has to draw it, and the SDK has to tell the
consumer what to draw and when. Its `update` field also changes on its own
(NEXT_FRAME_ONLY becomes NONE "at the start of the next frame"), which is an
event the scene graph must see.

Both reference hosts also looked cube maps up with the camera-space direction
directly, so a cube turned with the camera rather than with the geometry. That
is harmless for a fixed camera looking down -Z, but a generated cube is drawn
along the geometry's axes, and a lookup in another frame would read the wrong
faces.

## Decision

- `MaterialSystem::refOf` maps GeneratedCubeMapTexture to a `Source::Cube` ref
  with no url and no `cubeFaces`, carrying `TextureRef::generatedCube`: the
  texture node (the consumer's render-target cache key), the current `update`
  token and `size`. The SDK renders nothing.
- `GeneratedCubeMapSystem` (attached by `attachStandardRuntime`) resets
  NEXT_FRAME_ONLY to NONE as an input event at the start of the tick after the
  one whose frame first showed it, so `update_changed` reaches ROUTEs. The frame
  is the one the extractor emits after a tick.
- A consumer re-renders a cube whose `update` is not NONE before the frame:
  six `size` x `size` views with a pi/2 field of view from the local origin of
  the first Shape using it, along that Shape's local axes, in Figure 34.1 face
  order. Shapes that use the cube are left out of it. The faces persist across
  frames, so NONE keeps the last ones; a cube never rendered samples white.
- Every cube map (composed, image or generated) is fixed to its geometry's local
  frame: the Figure 34.1 axes are read as local axes. Lookup directions from
  eye-space generator modes (CAMERASPACENORMAL, CAMERASPACEPOSITION,
  CAMERASPACEREFLECTIONVECTOR and the no-generator default, COORD-EYE,
  NOISE-EYE, SPHERE-REFLECT) are carried into that frame by the inverse of the
  upper 3x3 of view * model. Local modes are used as is.

## Consequences

A rotated Shape rotates its cube with it, and a generated cube reflects the
same surroundings whatever its Transform. With the default camera and no
rotation, results are unchanged. The views come from the local origin, not the
bounding-box centre, so geometry authored off its origin reflects from that
origin. A cube shared by Shapes with different transforms is drawn once, from
the first. In the GL PoC a cube's faces are stored for the first Shape's
material model (sRGB-decoding for PBR); a cube shared by PBR and non-PBR Shapes
is decoded one way for both.

---
title: HAnim Deformation Core
summary: Humanoid-local skin bindings, live joint palettes, and CPU skin and displacer evaluation.
tags: [subsystem, hanim, skinning]
updated: 2026-09-27
related:
  - ../decisions/0055-hanim-skinning-descriptor.md
  - extract.md
---

# HAnim Deformation Core

## Purpose

`runtime/hanim/HAnimSkin.hpp` defines a binding compiled from one HAnimHumanoid and a pose evaluated from its current skeleton. The implementation in `HAnimSkinImpl.hpp` deforms source coordinates without writing to the authored Coordinate node. The result stays in humanoid-local space; the humanoid transform places it in the scene.

This page describes the deformation core. Extraction and tick scheduling consume its API separately.

## Binding and pose

`compileBinding()` takes joint order from `HAnimHumanoid.joints`, removes repeated references, then appends skeleton-reachable weighted joints missing from that list. Binding arrays associate by the original joints-list position. A single binding value applies to every joint. The binding keeps all valid vertex influences in CSR form and normalizes their weights per vertex. It records malformed list lengths, indices, and sums in `diagnostics`.

Authored `skinBindingCoords` and `skinBindingNormals` replace the corresponding skin source arrays. Coordinates can be `Coordinate` or `CoordinateDouble`. Without joint binding fields, inverse bind matrices are identity, matching the BASIC/v1 rest-pose rule in ADR-0055.

`evaluatePose()` walks the skeleton and composes the same local matrices as `TransformSystem`. It excludes the HAnimHumanoid transform. The palette is the current joint matrix multiplied by its inverse bind matrix. Displacer weights are read at evaluation time.

## Deformation

`deform()` blends every weighted source position through the palette. An unweighted position keeps its bind value. Supplied normals use each palette matrix's inverse transpose 3x3, then the blended normal is normalized. Joint displacements are added after skinning through the owning joint's 3x3 matrix.

`displaceSegmentPoints()` applies live Segment displacer weights to a caller-owned point array. `compileBinding()` builds a weak reverse lookup from Segment geometry Coordinate identity, including DEF/USE references. The source Coordinate is not modified.

## Tests and limits

`runtime/hanim/tests/hanim_skin_test.cpp` uses small constructed scenes for arm rotation, blended normals, more than four influences, binding fields, displacers, and bad input. The core currently has no extraction or event-system hookup; those consumers need to call the API. A shared Coordinate used by more than one Segment is associated with each discovered Segment by the reverse lookup.

---
title: "ADR-0030: Validate fieldOfView as a Fixed 4-Tuple, Do Not Retype (ClassicVRML Compatibility)"
summary: "OrthoViewpoint.fieldOfView keeps its MFFloat type (retyping to SFVec4f breaks ClassicVRML); the engine instead validates it as a fixed SFVec4f-semantics 4-tuple, shared with TextureProjectorParallel."
tags: [adr, validation, fieldofview, classicvrml, conformance]
updated: 2026-06-25
related:
  - ../architecture.md
  - ../../conformance/findings.yaml
---

# ADR-0030: Validate fieldOfView as a Fixed 4-Tuple, Do Not Retype

## Status

Proposed — 2026-06-25. Conformance finding `FOV-TYPE`.

## Context

`OrthoViewpoint.fieldOfView` and `TextureProjectorParallel.fieldOfView` carry identical 4-tuple
semantics (`min_x min_y max_x max_y`, default `-1 -1 1 1`), but the X3DUOM types them differently:
`OrthoViewpoint.fieldOfView` = **MFFloat**, `TextureProjectorParallel.fieldOfView` = **SFVec4f**.
MFFloat admits arity-illegal values (`[]`, `[1 2 3]`, `[1 2 3 4 5]`) that the prose's
`min_x < max_x; min_y < max_y` clause forbids.

The "strictest type that validates" principle suggests retyping OrthoViewpoint to SFVec4f, but that
is a defective fix: in ClassicVRML/VRML2 an `MFFloat` with more than one element must be wrapped in
brackets `[ -1 -1 1 1 ]`, while the `sfvec4fValue` grammar forbids brackets. Retyping would break
every existing ClassicVRML OrthoViewpoint (Kamburelis; Mantis 1398/1468, 4.0 frozen). Today x3d-cpp
stores `MFFloat _fieldOfView{-1,-1,1,1}` (`OrthoViewpoint.hpp:111/177`) with no arity or ordering
validation anywhere.

## Decision

Keep `MFFloat` as the storage/wire type for `OrthoViewpoint.fieldOfView` (preserving ClassicVRML
round-trip) and add a thin fixed-4-tuple validation layer applied to **both** fieldOfView fields:

1. **Arity normalization (lenient read, per ADR-0003)** — on parse/set, if `size() != 4` emit
   `FOV_TUPLE_ARITY` (WARNING): `0`→spec default, `<4`→default, `>4`→first 4. Never hard-fail.
2. **Ordering check** — enforce `min_x < max_x`, `min_y < max_y` → `FOV_EXTENT_ORDER` (WARNING; do
   not mutate). Same check for `TextureProjectorParallel.fieldOfView` (`x<z`, `y<w`).
3. **Typed accessor** — add a non-breaking `SFVec4f` convenience accessor on OrthoViewpoint
   (generator special-case, not a UOM edit) so callers get the same `{min_x,min_y,max_x,max_y}` shape
   as TextureProjectorParallel without changing the serialized type.

The vendored `X3dUnifiedObjectModel-4.0.xml` stays byte-identical to upstream (no type change).

## Consequences

**Positive:**
- ClassicVRML/VRML2 content keeps parsing; no content break.
- Both nodes share one validation path and a uniform vec4 accessor.
- Arity/ordering problems are surfaced as warnings instead of being silently stored.

**Trade-offs:**
- OrthoViewpoint callers still see a length-erased `MFFloat` member unless they use the convenience
  accessor — the API asymmetry with TextureProjectorParallel is reduced, not eliminated.
- Establishes a reusable "MF-typed but fixed-arity" validation pattern that other fields may need.

## X3D 4.1 alignment

Validated. The 4.1 X3DUOM **keeps** `OrthoViewpoint.fieldOfView` = MFFloat — the committee declined
the retype for exactly this ClassicVRML reason — so validate-don't-retype is the correct and durable
engine answer. The proposed upstream resolution (amend the `sfvec4fValue` grammar to accept optional
brackets *together with* a node retype, never one alone) remains open.

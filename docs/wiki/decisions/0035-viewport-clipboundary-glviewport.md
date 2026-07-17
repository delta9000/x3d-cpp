---
title: "ADR-0035: Viewport.clipBoundary Uses glViewport (Remap) Semantics"
summary: "The runtime defines clipBoundary as a sub-region the layer view is mapped (rescaled) into, with aspect ratio derived from the sub-region; it exposes a derived ViewportRegion{x,y,w,h,aspect} contract for renderers."
tags: [adr, layering, viewport, renderer-contract, conformance]
updated: 2026-06-25
related:
  - ../architecture.md
  - ../../conformance/findings.yaml
---

# ADR-0035: Viewport.clipBoundary Uses glViewport (Remap) Semantics

## Status

Proposed — 2026-06-25. Conformance finding `VIEWPORT-CLIP`.

## Context

§35.4.3 (`layering.md:204`) defines `clipBoundary` as "fractions of the normal render surface in the
sequence left/right/bottom/top … the output will only appear in the specified subset of the render
surface" — and stops there. It never states whether the projection is **remapped/rescaled** into the
sub-region (glViewport semantics: aspect follows the sub-rect) or merely **clipped** while the
full-surface projection is retained (glScissor semantics: aspect stays full-surface). These produce
different aspect ratios and content scaling, so conforming implementations diverge. This is Mantis 326
(open). Nothing in §35.4.x relates a layer's viewpoint aspect ratio to its viewport region.

x3d-cpp is renderer-agnostic, so it cannot be "wrong" at the pixel level — but it must pin down which
semantics the sub-region it exposes represents, so every integrating renderer frames identically.
Today `Viewport::setClipBoundary` (`generated_cpp_bindings/x3d/nodes/Viewport.hpp:89`) only validates each
component ∈ [0,1] and stores the 4-tuple (default `{0,1,0,1}`); there is no ordering check, no sub-rect
computation, and no consumer of `clipBoundary` anywhere in `runtime/` (only whole-surface
`ViewportSize` is exposed).

## Decision

Adopt **glViewport (remap/rescale) semantics** and expose them as the runtime's documented contract.
Given surface pixel size `(W, H)` and `clipBoundary = [l, r, b, t]` (lower-left origin):

- Sub-rect: `x = l·W`, `y = b·H`, `w = (r−l)·W`, `h = (t−b)·H`. The renderer sets its device viewport
  to this rect (projection mapped into it; content rescaled).
- **Aspect ratio for the layer viewpoint = `w / h`** (from the sub-rect, **not** `W/H`).
- Scissor to the sub-rect is required (so overlay layers do not bleed).
- Guard `l ≤ r`, `b ≤ t`; zero-area ⇒ no output. Default `0 1 0 1` ⇒ full surface, `aspect = W/H`.

The runtime computes and exposes a derived `ViewportRegion{x, y, w, h, aspect}` (extract/scene side);
the renderer issues the actual glViewport/scissor. Justified by the node name "Viewport" (which in
every graphics API means a region the projection is *mapped into*) and §35.2.3/§35.2.4 (each layer
owns its viewpoint; the CAD front/side/back/oblique quadrant use case requires each view to frame its
own projection into its region).

## Consequences

**Positive:**
- All renderers integrating against the runtime frame Viewport content identically.
- Matches the node name and the layer-as-independent-view intent; serves the CAD quadrant use case.
- Adds the missing ordering guard (`Viewport.hpp:89` currently validates range but not order).

**Trade-offs:**
- Fixes a spec-open choice in the engine; if 4.1 later mandates glScissor semantics this is superseded.
- The runtime now owns a small derived geometry contract (`ViewportRegion`) that renderers must honor.

## X3D 4.1 alignment

Unresolved upstream (Mantis 326 "not sure it still needs addressing"; still noted in 2026). The engine
contract is ahead of the spec; an upstream erratum (§35.4.3 stating glViewport-style remap + the
aspect/ordering rules) is owed.

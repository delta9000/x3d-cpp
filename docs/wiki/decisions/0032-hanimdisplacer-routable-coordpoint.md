---
title: "ADR-0032: HAnimDisplacer Produces a Routable Derived coord.point"
summary: "In the headless runtime, a Segment's coord.point current value reflects the weighted sum of its Displacers and emits point_changed; the neutral pose is retained internally and never destructively overwritten."
tags: [adr, hanim, displacer, events, headless, conformance]
updated: 2026-06-25
related:
  - ../architecture.md
  - ../subsystems/event-cascade.md
  - ../../conformance/findings.yaml
---

# ADR-0032: HAnimDisplacer Produces a Routable Derived coord.point

## Status

Proposed — 2026-06-25. Conformance finding `HANIM-DISP` (deferred on a DisplacerSystem).

## Context

X3D §26.3.1/§26.3.5 defer HAnimDisplacer field semantics to ISO/IEC 19774 §6.6, which is also silent
on whether evaluating a Displacer **mutates** the parent Segment's `coord.point` (so ROUTEs/Scripts
reading `point_changed` see displaced vertices) or applies displacement only **transiently at render**.
X_ITE applies Displacers in the GLSL shader, so `coord.point` is unchanged and no `point_changed`
fires; Andreas Plesch's `pointOutput_Tongue.x3d` ROUTEs the displaced `coord.point` into a
PositionInterpolator, which only works if displacement is reflected in the data model.

x3d-cpp today: `HAnimDisplacer` is a generated pure-data node; **no System consumes it** (only
`TransformSystem` references HAnim, for Humanoid/Joint TRS). `coordIndex`/`displacements`/`weight` are
loaded but never evaluated — Displacers are inert and `coord.point` shows the rest pose regardless of
`weight`.

x3d-cpp is headless and renderer-agnostic — "apply in the shader" is unavailable, and the runtime
exists to expose a correct, routable data model.

## Decision

Add a `DisplacerSystem` with this contract:

1. **Non-destructive** — the author's neutral `coord.point` is retained internally as the recompute
   basis; it is never overwritten (a `weight` 0 Displacer is the identity; round-trip fidelity matters).
2. **Derived current value** — `coord.point`'s *current* value (what `getPoint()` / the field table /
   readers return) is, per vertex *i*:
   `point_neutral[i] + Σ_d (weight_d × displacements_d[k])`
   over every Displacer *d* whose `coordIndex` references *i*. Commutative ⇒ order-independent.
3. **Routable** — a change to any contributing `weight`/`displacements`/`coordIndex` recomputes (gated
   by the existing `DirtyTracker`) and **emits `point_changed`** from the Segment's Coordinate node.

This satisfies Plesch's ROUTE use case and is a strict superset of X_ITE's visual result (x3d-cpp also
surfaces the data X_ITE keeps inside the GPU).

## Consequences

**Positive:**
- Displacers become observable and routable — the right behavior for a data-exposure runtime.
- Neutral pose preserved (round-trip safe); recompute is order-independent and dirty-gated.

**Trade-offs:**
- Diverges from majority-browser (X_ITE) *data-model* behavior (which leaves `coord.point` untouched);
  the divergence is deliberate and recorded here.
- Requires a new `DisplacerSystem` (the finding is `deferred` until it lands).

## X3D 4.1 alignment

Unresolved upstream (the 2025 thread left it "open to browser interpretation"). The engine policy is
ahead of the spec; an upstream erratum specifying the `coord.point`/`point_changed` contract is owed.

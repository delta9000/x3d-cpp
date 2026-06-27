---
title: "ADR-0037: Graph-Walk Traversal Budget for Acyclic Fan-Out"
summary: The per-path scene-graph walks (extractor, light collection, pick) carry a node-visit budget that bounds an acyclic "doubling DAG" fan-out, surfaced as a graceful budgetExceeded signal; worldOf instead memoizes since it is a first-found search.
tags: [adr, traversal-budget, doubling-dag, dos, scene-graph, extractor, pick, instancing]
updated: 2026-06-26
related:
  - 0016-cycle-breaker.md
  - ../subsystems/extract.md
  - ../subsystems/scene-graph.md
---

# ADR-0037: Graph-Walk Traversal Budget for Acyclic Fan-Out

## Status

Accepted — 2026-06-26

## Context

[ADR-0016](0016-cycle-breaker.md) (sever cycles once at build) plus the MEM-1
self-guards (path-membership + depth cap on the extractor/pick/light walks) make
every scene-graph walk **crash-proof** against USE-cyclic and pathologically deep
graphs. They do **not** make it **DoS-proof** against a legitimate *acyclic*
graph that fans out exponentially.

Construct a "doubling DAG": `G0 children [G1,G1]`, `G1 children [G2,G2]`, …
~30 levels deep, a single Shape at the leaf. There is **no containment
back-edge** (so `breakContainmentCycles` leaves it intact) and depth stays well
under `kMaxNestingDepth` (1000, so the depth cap never fires) — yet the leaf is
reachable via `2^30` distinct paths. Surfaced by the adversarial review of the
MEM-1 work (issue [#21](https://github.com/delta9000/x3d-cpp/issues/21)).

Because per-path traversal is **intentional** for USE-instancing — a node USE'd
under N distinct placements legitimately yields N RenderItems / LightDescs / pick
candidates, each in its own world frame — the emitters **cannot** be deduplicated
by a visited set without collapsing valid instancing. They explode:

- `SceneExtractor::walk` emits one RenderItem **per path** → ~10⁹ emissions;
- `LightSystem::walk` emits one LightDesc per path;
- `PickSystem::pickNode` tests geometry per path (each placement's world hit
  differs).

`PickSystem::worldOfRec` is the exception: it is a **search** that returns the
*first* DFS match. Pruning already-explored subtrees does not change that result.

## Decision

A hybrid, mirroring the two semantics — both non-throwing (the extractor/pick are
read paths; a throw would surprise an embedder mid-frame).

1. **`WalkBudget`** (`runtime/RecursionLimits.hpp`) — a per-traversal node-visit
   counter. Each guarded walker calls `if (!budget.spend()) return;` at entry;
   when exhausted `spend()` latches `tripped` and the walk stops descending,
   returning a partial result. Default ceiling `kMaxGraphWalkVisits = 1'000'000`
   is far above any legitimate placement count yet bounds the fan-out to a finite,
   fast walk.

2. **The per-path emitters take the budget.** `SceneExtractor` owns one
   `WalkBudget` per `fullSnapshot()` / `delta()` — cap overridable via
   `MeshBuildOptions.maxWalkVisits` — and **shares it** across light collection
   (`LightSystem::collect(scene, budget)`) and the geometry walk, exposing
   `bool budgetExceeded() const` (mirrors the existing `skippedGeometryCounts()`
   pull accessor). `PickSystem::pickClosest` owns a `WalkBudget` (cap via a new
   `maxVisits` parameter) and sets `PickResult.budgetExceeded`.

3. **`worldOfRec` memoizes instead.** A `visited` set prunes already-explored
   subtrees; the first-found result is unchanged, but cost drops from O(paths) to
   O(nodes) — so it *succeeds* on a legitimately wide DAG where a budget would
   falsely give up.

## Consequences

**Positive:**

- Every public graph walk is now both crash-proof (MEM-1) and DoS-bounded. The
  doubling DAG terminates in bounded time/memory with a detectable signal.
- `worldOf` additionally becomes correct *and* fast on legitimately wide DAGs
  (heavy USE-instancing), not merely bounded.
- Codegen-free: the golden binding hash is byte-identical.
- The budget default is a no-op on every real scene (orders of magnitude below
  1M visits); regression-pinned by `runtime/scene/tests/walker_budget_test.cpp`.

**Trade-offs / costs:**

- The cap is a **policy ceiling**, not a correctness fix: per-path emission is the
  intended instancing semantics, so a genuinely >1M-placement scene must raise
  `maxWalkVisits` (and read `budgetExceeded()` to detect truncation).
- A capped snapshot still allocates up to ~cap RenderItems, so the budget
  **bounds** — does not eliminate — worst-case memory.
- The overflow signal is a pull accessor / result flag, not a per-tick push
  channel (a `RenderDelta` push field is a later follow-up if a consumer needs
  per-frame notification — same deferral shape as `skippedGeometry_`).

## Related

- [ADR-0016: Containment Cycles Severed Once at Scene Build](0016-cycle-breaker.md)
- [Extract subsystem](../subsystems/extract.md)
- [Scene Graph subsystem](../subsystems/scene-graph.md)
- Primary implementation: `runtime/RecursionLimits.hpp` (`WalkBudget`,
  `kMaxGraphWalkVisits`), `runtime/extract/SceneExtractor.hpp`,
  `runtime/extract/LightSystem.hpp`, `runtime/scene/PickSystem.hpp`
- Design spec: `docs/superpowers/specs/2026-06-26-graph-walk-traversal-budget-design.md`
- Regression test: `runtime/scene/tests/walker_budget_test.cpp`
- Discovery context: adversarial review of the SEC-1/SEC-2/MEM-1 hardening (#20)

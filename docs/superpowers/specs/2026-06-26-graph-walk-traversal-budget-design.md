# Graph-walk traversal budget — design spec

**Date:** 2026-06-26
**Issue:** [#21](https://github.com/delta9000/x3d-cpp/issues/21) — PERF/SEC acyclic "doubling DAG" exponential per-path emission
**Status:** Approved (design); implementation in progress

## Context

The MEM-1 cycle+depth guards (PR #20) made the extractor/pick/light walks
**crash-proof** against USE-cyclic and pathologically deep scenes. They do **not**
make them **DoS-proof** against a legitimate *acyclic* scene that fans out
exponentially.

Construct a "doubling DAG": `G0 children [G1,G1]`, `G1 children [G2,G2]`, …
~30 levels deep, a single Shape at the bottom. There is **no containment
back-edge**, so `breakContainmentCycles()` leaves it intact, and depth stays well
under `kMaxNestingDepth` (1000). But the leaf is reachable via `2^30` distinct
paths, so:

- `SceneExtractor::walk` emits one RenderItem **per path** → ~10⁹ emissions.
- `LightSystem::walk` emits one LightDesc per path.
- `PickSystem::pickNode` tests geometry per path (each placement has its own
  world transform).
- `PickSystem::worldOfRec` re-explores the fan-out searching for a target.

This is a CPU/memory denial-of-service distinct from the stack-overflow class.

## Key insight: the fix differs per walker

Per-path emission is **intentional** for USE-instancing — a node USE'd under N
distinct placements legitimately produces N RenderItems / LightDescs / pick
candidates, each in its own world frame. So the emitters **cannot** be memoized
(that would collapse legitimate instancing). They must instead be **bounded**.

`worldOfRec` is different: it is a **search** that returns the *first* DFS match.
Pruning already-explored subtrees with a visited set does not change the
first-found result — and on a legitimately wide DAG it *succeeds* (O(nodes))
where a budget would falsely give up.

## Decision

Hybrid, with graceful (non-throwing) overflow:

1. **`WalkBudget`** — a new per-traversal node-visit counter in
   `runtime/RecursionLimits.hpp`:
   ```cpp
   struct WalkBudget {
     std::size_t remaining; bool tripped = false;
     explicit WalkBudget(std::size_t cap) : remaining(cap) {}
     bool spend() { if (!remaining) { tripped = true; return false; } --remaining; return true; }
   };
   inline constexpr std::size_t kMaxGraphWalkVisits = 1'000'000;
   ```
   Each guarded walker calls `if (!budget.spend()) return;` at entry. Once
   exhausted it latches `tripped` and stops descending. The default cap is far
   above any legitimate placement count (corpus/kelp scenes are orders of
   magnitude smaller) yet bounds the doubling DAG to a fast, finite traversal.

2. **Emitters take the budget (graceful stop + signal):**
   - `SceneExtractor` owns one `WalkBudget` per `fullSnapshot()` — cap overridable
     via a new `MeshBuildOptions.maxWalkVisits` (default `kMaxGraphWalkVisits`) —
     threads it into `LightSystem::collect(scene, budget&)` and `walk(...)`, and
     exposes `bool budgetExceeded() const`, reset each snapshot (mirrors the
     existing `skippedGeometryCounts()` pull accessor).
   - `PickSystem::pickClosest` owns a `WalkBudget`, threads it into `pickNode`,
     and sets a new `PickResult.budgetExceeded`.

3. **`worldOfRec` takes memoization, not a budget:** a
   `std::unordered_set<const X3DNode*> visited` threaded through the recursion
   prunes already-explored subtrees. Result identical (first-found); cost drops
   to O(nodes).

No throw: the extractor/pick are non-throwing read paths, so an exhausted budget
returns a **partial** result plus the `budgetExceeded` / `PickResult.budgetExceeded`
flag for an embedder to detect and react.

## Scope boundaries (YAGNI)

- Cap configurability is added only where it is most likely needed — the
  extractor's `MeshBuildOptions`. `PickSystem`/`LightSystem` use the shared
  constant default; a setter can be added later if an embedder needs it.
- No per-frame push channel for the overflow signal (the pull accessor mirrors
  the existing `skippedGeometry_` precedent); a `RenderDelta` push field is a
  later follow-up if a consumer needs per-tick notification.

## Testing (TDD)

- `scene_extractor_budget_*` — a doubling DAG (~depth 22) through `fullSnapshot()`
  terminates fast, `budgetExceeded()==true`, `itemCount() <= cap`.
- `pick_system_budget_*` — same DAG with a Shape; `pickClosest` terminates,
  `PickResult.budgetExceeded==true`.
- `light_system_budget_*` — a light at the fan-out leaf; collection terminates and
  trips the budget (surfaced via the extractor's `budgetExceeded()`).
- `worldof_wide_dag_*` — `worldOf(present target)` returns the correct transform
  without blowup; `worldOf(absent target)` terminates.
- The MEM-1 `walker_cycle_guard_test` still passes (budget is additive; the small
  cyclic scene never trips the cap).

## Consequences

**Positive:** every public graph walk is now both crash-proof (MEM-1) and
DoS-bounded. `worldOfRec` additionally becomes correct/fast on legitimately wide
DAGs. Codegen-free → golden hash byte-identical.

**Trade-offs:** the default cap is a heuristic; an embedder with a genuinely
>1M-placement scene must raise `maxWalkVisits` (and gets `budgetExceeded()` to
detect truncation). A capped snapshot still allocates up to ~cap RenderItems, so
the cap bounds — not eliminates — worst-case memory. The acyclic exponential
emission is the *intended* per-path semantics, so the guard is a policy ceiling,
not a correctness fix.

# Deferral Backlog — DEPRECATED (2026-06-22)

> **This file is retired.** Tracking moved to two purpose-built, complementary
> trackers. The full historical content (every closed row + commit SHA) lives in
> this file's **git history** — `git log -p docs/superpowers/BACKLOG.md`.

## Where tracking lives now

| What | Where | Why |
|------|-------|-----|
| **Behavioral / spec-conformance gaps** (does a node behave per ISO 19775?) | [`docs/conformance/findings.yaml`](../conformance/findings.yaml) → [generated view](../conformance/INDEX.md) | The source of truth. Schema-validated, spec-clause-anchored, and **gated** in `mise run ci` (`conformance-gate`). Facts auto-derived from the bindings/registry; only judgments are hand-edited. |
| **Engineering / planning deferrals** (build, codec, extraction, SDK, seams) | [GitHub Project — *x3d-cpp project*](https://github.com/users/delta9000/projects/2) | Workflow board (Status / Priority / Size). No spec anchoring; tracks queued vs. in-progress vs. done. |

## Why it was deprecated

The backlog tried to be both a conformance register *and* a planning board, with
**no gate** — so it drifted in both directions. The migration validation (each row
re-checked against the actual code, not its label) caught two stale rows:

- **`M2E-1`** claimed "*superseded — ProximitySensor emits `centerOfRotation_changed` now*."
  Code grep: that event is emitted **nowhere**. The correct truth was already in
  `findings.yaml` as **`ENV-03` (open)** — stale-*optimistic* drift.
- **`CONF-VIEWNAV`** was marked `DEFERRED-DESIGN / critical`, but `findings.yaml`
  `BIND-01..08` were all **closed** with commits — stale-*pessimistic* drift.

Every other `CLOSED` row was verified genuinely done (real symbol + test in tree),
so those moved to history. The `findings.yaml`-tracked behavioral gaps stayed there
(no duplication). Conformance "decision residue" that had no findings home
(`CONF-TDN1V`, `CONF-CRITIC`) was migrated into `findings.yaml`.

## Migration map (backlog ID → new home)

The 13 still-actionable engineering deferrals became Project cards:

| Project card | Source backlog ID(s) |
|---|---|
| http/urn EXTERNPROTO resolver | PRF-6 |
| Structural re-index on add/remove children | M2C-2 |
| Per-Layer binding stacks + per-layer view volumes | M2C-BIND-1, M2E-5 |
| Dynamic-construction / external SAI | SCR-SAI-DYN (S2+) |
| Async / spontaneous scripts | SCR-ASYNC |
| MultiTexture compositing | T-TEX-D1 |
| MovieTexture frame sequence | T-TEX-D2 |
| CubeMap / environment textures | T-TEX-D3 |
| Bidi / complex text shaping | T-TEXT-D1 |
| Exact Text bounds via FontMetrics | T-TEXT-D2, M2B-1 |
| NURBS / 2D-geometry component | M25-1 residue, SMK-3 |
| XML/JSON writer version floor | VP2-L1 |
| Thread-safe `nodesImplementing()` cache | PIV-2 |

Behavioral rows (FOL-OVL→FOL-5, M2E-1→ENV-03, M2E-3→ENV-01, M2E-4→ENV-05,
M2B-2→ENV-02/CONF-GEO, NAV-COLLISION→CONF-NAV-COLLISION, SCR-ASYNC/REFRESH→SCR-005,
the whole CONF-RBP family, …) remain in `findings.yaml`. Intentional/informational
rows (`VP2-L2`, `NAV-EXTRA`, `SMK-3`) were not carried forward.

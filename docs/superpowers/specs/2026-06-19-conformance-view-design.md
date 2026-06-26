# Conformance View — design (2026-06-19)

## Problem

The conformance picture is trapped in flat prose: `BACKLOG.md` (438 lines), the
per-finding `conformance-gap-register.md` tables, and memory. There is no way to
**zoom out** — no rollup that says "Interpolation: 13/13 nodes exist, behaves ◑,
2 critical gaps open" at a glance. The prose also rots because the same fact
(status of a gap) is restated in three places. We want the doxygen-style node
reference that x3dom / instantreality / web3d generated, modernized for the AI
era: **a generated, navigable, machine- and human- and agent-readable conformance
view that is the single source of truth for signaling where we are.**

## Core principle — separate facts from judgments

Two kinds of data, each with exactly one home:

| Data | Examples | Source of truth | Currency |
|------|----------|-----------------|----------|
| **Facts** | node exists? extracts? has a System wired? component/level? profile? | **the code** (generated bindings, registry, runtime) | auto-derived — cannot rot |
| **Judgments** | does it *behave per ISO prose*? severity? status? clause? | **`findings.yaml`** (authored, structured) | one file you edit |

Nothing is parsed out of prose. The behavioral findings are *promoted* from the
register tables into one structured file that becomes canonical; every human view
(and the machine model) is rendered *from* it joined with the code-derived facts.

## Architecture

```
docs/conformance/                      ← THE discoverable home
├── README.md          hand-written, tiny: what this is + how to add/close a finding
├── findings.yaml      SOURCE OF TRUTH for judgments (edit only this)
├── profiles.yaml      reviewed reference: ISO profile → {component: minLevel}
│
│   scripts/conformance_view.py   joins findings.yaml + profiles.yaml + CODE FACTS
│
├── model.json         generated: merged machine/agent/RAG model
├── INDEX.md           generated: ZOOM-OUT (bug picture + component × dimension matrix)
└── components/<Comp>.md  generated: zoom-in (node rows + that component's findings)
```

### Code-fact extractors (`scripts/conformance_view.py`)

- **nodes** — scan `generated_cpp_bindings/*.hpp`: class name, `componentName()`,
  `componentLevel()`, concrete-vs-abstract (abstract = name starts with `X3D` and
  is an interface/base; detected by `componentName()` absence or `X3D…Node`
  abstract list). Gives the **Exists** dimension + component grouping.
- **registry** — parse `X3DInterfaceRegistry.cpp` `table()` rows
  `{"Name", {InterfaceId::A, …}}` → node → interfaces. Classifies **behavioral**
  nodes (implements one of the behavioral abstract interfaces:
  X3DInterpolatorNode, X3DTimeDependentNode, X3DSensorNode (+ Pointing/Key/Network/
  Environmental/Drag sub-types), X3DScriptNode, X3DBindableNode, X3DFollowerNode,
  X3DSequencerNode, X3DTriggerNode).
- **systems** — scan `runtime/**/*.hpp` for concrete `dynamic_cast<Concrete *>`
  targets in `System::attach` + `…System<Concrete, …>` template instantiations in
  registration sites. Gives **behaves: wired?** (a heuristic; findings override it,
  see join). Raw detected set is emitted into `model.json` for auditability.
- **extraction** — scan the extractor / mesh-builder dispatch for handled geometry
  node types; the documented skipped set (NURBS, 2D primitives — M25-1/SMK-3)
  marks the rest. Gives **Extracts** for geometry nodes (`n/a` otherwise).
- **profiles** — `profiles.yaml` maps each ISO profile (Core, Interchange,
  Interactive, Immersive, Full, + CAD/Medical best-effort) to `{component:
  minLevel}`. A node is in profile P iff P includes its component at a level
  `>=` the node's `componentLevel()`.

### `findings.yaml` schema

```yaml
- id: INTERP-01                 # stable id (campaign convention)
  interfaces: [X3DInterpolatorNode]
  nodes: [SplinePositionInterpolator, SquadOrientationInterpolator, EaseInEaseOut]
  component: Interpolation      # optional; else inferred from nodes
  clause: "19.2.4, 19.4.10-13"
  severity: critical            # critical | major | minor | low
  status: closed                # open | deferred | fixed | closed
  wave: 5                       # optional campaign wave
  commit: 07c31ca               # optional (for fixed/closed)
  summary: "Spline/Squad/Ease interpolators had no System; set_fraction was a no-op."
  note: "..."                   # optional closure plan / deferral reason
```

Statuses: `open` (live gap), `deferred` (known, blocked — keep with reason),
`fixed`/`closed` (done — keep for history + closed-rate). Schema is validated by
the gate.

### The join → status per node

- **exists**: ✓ if a concrete node header exists.
- **extracts**: ✓ / ◑ / ✗ for geometry nodes; `n/a` otherwise.
- **behaves**: `n/a` (node implements no behavioral interface) · `✗ inert`
  (behavioral, no System wired, or an OPEN finding says inert) · `◑ partial`
  (wired but ≥1 open/deferred finding) · `✓` (wired, no open findings).
  **Findings authoritatively override the wired-heuristic** (e.g. CONF-TDN5 marks
  AudioClip inert even if a base cast matched).
- **referential integrity**: a finding naming a node/interface that does not exist
  in the bindings is **loudly reported** in `INDEX.md` (catches stale findings).

### Outputs

- **`model.json`** — `{meta, summary, components: [{name, levels, profiles,
  nodes: [{name, level, exists, extracts, behaves, interfaces, clause, findings:[ids]}],
  findings: [...]}], unresolved_findings: [...]}`. Agent/RAG-queryable.
- **`INDEX.md`** — (1) **Bug Picture**: open/deferred gap counts by severity,
  worst-offender components, inert behavioral nodes, closed-this-wave, unresolved
  findings; (2) the **component × {exists, extracts, behaves, profiles}** matrix,
  one row per component with rollup counts. One screen.
- **`components/<Comp>.md`** — node table (one row per node: exists/extract/behave +
  spec clause link) + the findings tagged to that component. Node = row (no 343-file
  explosion).

### Tasks & freshness

- `mise run conformance` — regenerate `model.json` + Markdown.
- `mise run conformance-gate` — validate `findings.yaml`/`profiles.yaml` schema
  **and** regenerate to a temp dir and diff vs committed; fail on drift (mirrors
  `golden`). Added to `mise run ci`.

### Tests (pytest)

- node-header parser, registry parser, systems scanner, extraction scanner on
  small fixtures;
- findings schema validation (rejects bad severity/status, missing id);
- the join/classifier (behaves override, referential integrity);
- a smoke test that the real repo generates without unresolved-finding errors.

## Migration & consolidation (one-time)

1. Move the ~33 register findings + the deferred **behavioral** items (NAV-COLLISION,
   CONF-TDN5/SCR5/IMG, ENV/BIND/INTERP deferrals) into `findings.yaml`.
2. Replace the register's finding tables with a pointer to `docs/conformance/`;
   keep its **campaign methodology** prose (audit workflow, verify protocol, tuning
   insights) — that is process, not findings.
3. `BACKLOG.md` stays the broader engineering-deferral tracker (build/codec/
   extraction milestones); its behavioral rows link into the view instead of
   restating gaps.

## Non-goals

- Per-node HTML/CSS site (Markdown chosen for git-diff + AI-native reading).
- Auto-deriving *judgments* from code (behavioral conformance is human knowledge).
- Profile table exhaustiveness beyond the main profiles (reviewed reference,
  correctable in one file; provenance noted in `profiles.yaml`).
```

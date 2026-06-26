# Architecture Validation & Milestone Resequencing (2026-06-07)

**Status:** authoritative resequencing of the end-goal roadmap (M1→M4). Supersedes
the *ordering* implied by `2026-06-03-event-system-design.md` and the week plan;
does **not** change the architecture (validated below). Inputs: a deep-research
pass (25/25 claims confirmed, 0 refuted) over USD/Hydra, peer-reviewed scene-graph
research, the X3D ISO execution model, and shipping X3D runtimes (Castle/X3DOM/
X_ITE); plus an empirical sweep of the 17.7k-file corpus.

---

## 1. Verdict: the architecture is validated — do not restructure

The three core bets all hold up against the dominant prior art:

1. **Node-as-truth + systems-update-in-place + renderer-as-consumer.** This is
   *exactly* USD/Hydra's structure: scene delegate → render index → render delegate,
   renderers decoupled as `HdSceneIndexObserver`s that never own or mutate scene
   data. The same node-as-truth + render-cache + incremental-propagation pattern is
   independently established in peer-reviewed work (VRVis, *Lazy Incremental
   Computation for Efficient Scene Graph Rendering*, HPG '13) and in OSG / Open
   Inventor / Java3D / SceniX. **Keep it.**

2. **Per-tick incremental "changed-set" at the renderer seam** (not full
   re-extraction each frame). This is Hydra's `HdChangeTracker` + dirty-list-driven
   `SyncAll`: the render index holds only handles and resyncs *just* the dirtied
   prims. VRVis corroborates with version-number dirty-checking. **Validated.**

3. **ECS-flavored batching only at hot paths, NOT a full ECS.** Castle Game Engine
   proves a retained-mode X3D-node-as-truth model manipulated in place is directly
   viable in production. No major X3D runtime uses ECS. **Keep the node model as
   authority; ECS stays a possible runtime-side projection, taken only on profiling
   evidence.**

**The codegen moat is de-risked at the mechanism level but is still the project's
own bet at the "chase the external ISO spec" level.** USD's `usdGenSchema`
generates typed C++ schema classes (thin wrappers over `UsdPrim`) from a
declarative `.usda` and *preserves hand-authored methods across regeneration* (the
`// --(BEGIN CUSTOM CODE)--` delimiter) — so codegen-vs-hand-written is a false
dichotomy, and generating typed nodes atop a single source-of-truth node is proven
production practice. **But** USD generates from its *own internal* schema, and
Castle's generator runs off *hand-authored* per-node definitions — neither
generates its whole typed model from a third-party standards-body XML. No prior art
proves "generate from the official spec → win on conformance." That proof is **M3's
job** (see §4). This reframes M3 from "a milestone" to "the moat validation."

### Two design decisions the research forces

- **Pull vs push at the extraction seam.** Every *shipping* comparable (legacy
  Hydra, OSG, Castle) is **pull-based**: the renderer queries scene state on demand
  via `Sync`. Hydra 2.0 is migrating toward push (Scene Index observers).
  **Recommendation (default): pull-of-an-incrementally-maintained-dirty-set.** The
  runtime maintains dirty bits during `tick(now)`; the consumer pulls the changed
  set *after* the tick returns and controls its own cadence. Lowest risk, matches
  every shipping runtime, no threading/ownership surprises. Revisit push only if a
  consumer needs change *notifications* rather than change *queries*. **This is the
  one open decision to confirm when M2.5 lands — not now.**

- **The changed-set quantum is spec-defined, not invented.** ISO/IEC 19775-1: all
  events in a cascade share the initial timestamp (instantaneous), and nodes emit
  *at most one event per field per timestamp* — the mandatory guard that breaks
  routing loops (the ROUTE graph may be cyclic; the transform hierarchy may not).
  So the per-tick dirty-set is simply "every field touched during this timestamp's
  cascade." Our existing `X3DEventCascade` already owns this quantum — the dirty
  layer hangs directly off it.

---

## 2. Corpus evidence that drives the ordering

Sweep of `<x3d-render-workspace>/testdata` (17,717 files: 16,890 `.x3d`,
662 `.wrl`, 90 `.x3dv`, 75 `.json`). Files containing each mechanism:

| Mechanism | Files | % | Roadmap bucket |
|---|---:|---:|---|
| Viewpoint / NavigationInfo (binding stacks) | 4,339 | 24.5% | **M2 — not started** |
| TimeSensor | 1,235 | 7.0% | ✅ M1 done |
| Interpolators | 1,145 | 6.5% | ✅ M1 done |
| PROTO / EXTERNPROTO | 1,066 | 6.0% | M1 — captured, not expanded |
| **Script / SAI** | 747 | 4.2% | **tabled (S1–S4)** |

(Script count is a generous upper bound — rough grep, includes test dirs and field
names.) The signal is unambiguous: **binding-stack content outnumbers Script
content ~5.8×, and PROTO outnumbers Script too.** Deferring Script/SAI past the
scene-graph runtime is empirically defensible; prioritizing binding stacks and
PROTO expansion is empirically mandated.

The X3D normative execution model confirms the *dependency* direction the corpus
suggests: the event/ROUTE runtime is the **substrate** the scene-graph runtime
rides on. Interpolators are pure event consumers (`set_fraction` in →
`value_changed` out), externally clocked by TimeSensor via ROUTE. Binding stacks
are event-driven (`set_bind` inputOnly / `isBound` outputOnly). So M1's cascade
*must* precede M2's binding/animation behaviors — which is the order we already
built. (Nuance, from a 2-1 vote: camera/viewpoint binding *resolution* is step (a)
of each tick, **before** the sensor/route cascade — so "events strictly first" is a
simplification; binding-stack *state* is read at the top of the tick, but it is
*mutated* by the event runtime. The dependency still points event→scenegraph.)

---

## 3. Resequenced milestones

Current position: original 7-phase modernization ✅ complete & pushed; M1 behavior
runtime ✅ mostly done (cascade, 8 interpolators, TimeSensor, Scene→graph bridge,
all-4-encoding parsing at ~100% corpus conformance); HEAD = the tabled S1 spec.

The spine (M0→M4) is unchanged. The ordering of remaining work is:

### Step 0 — Pay the owed lenient-read warning debt *(small, do first)*
The lenient-read decision was "keep out-of-range values **+ warnings**"; the
warnings half was never built. Add a validation/warning-collection pass that
surfaces which values were kept out-of-range (a graph walk emitting a diagnostics
list; typed setters still throw, the reflection path still keeps + now *records*).
Closes a written promise before new scope opens. No codegen/golden change expected.

### Step 1 — M1 closeout: PROTO / EXTERNPROTO expansion *(6% of corpus)*
Readers already capture PROTO bodies structurally (`Scene.protoInstances`, no
expansion). Implement instantiation: bind a ProtoDeclare's interface fields to
each instance, expand the body into real nodes, wire IS-mapped fields through to
the cascade. Higher corpus leverage than Script and unblocks more real files.
**Keep Script/SAI (S1–S4) tabled** until a consumer needs it.

### Step 2 — M2 scene-graph runtime *(ordered by spec tick + corpus frequency)*
- **M2a — Dirty-tracking layer + world-transform propagation** *(foundation).*
  Build the `HdChangeTracker` analog first: a per-field/per-node dirty-set fed by
  the event cascade (the §1 quantum). Then world-transform propagation down the
  Transform hierarchy, consuming dirtied local transforms and marking world
  transforms dirty. Everything downstream (bounds, picking, extraction) reads this.
- **M2b — Bounding volumes.** Local→world bbox propagation; depends on M2a.
- **M2c — Binding stacks: Viewpoint / NavigationInfo / Background / Fog**
  *(24.5% — highest leverage).* Per-type stack via `X3DBindableNode`, one active
  per layer, `set_bind`/`isBound` event-driven (sits directly on M1's cascade).
  Resolved at step (a) of the tick.
- **M2d — Picking / ray intersection + navigation math.** Depends on M2a+M2b.
- **M2e — LOD / visibility.** Depends on active Viewpoint (M2c) + bounds (M2b).

### Step 3 — M2.5 the three seams
- **Extraction API** — the payoff of M2a's dirty layer: pull-of-changed-set
  read-out of world transforms / geometry / materials / lights / camera. Confirm
  pull-vs-push here (default: pull, §1).
- **Asset-resolver seam** — embedder supplies bytes by URL.
- **Input/sensor seam** — embedder feeds pointer-ray / key / time.

### Step 4 — M3 headless proof = **the moat validation**
Conformance validator + scene-state snapshotter run on the official X3D example
archive, provable without a renderer. This is where the "generate-from-spec →
conformance" bet gets *empirically demonstrated* (no prior art proves it for us).
Treat M3 as the strategic payoff that justifies M2, not a postscript.

### Step 5 — Script/SAI (S1–S4), then M4
Un-table Script when a consumer needs it (4.2%, least-common behavior mechanism;
S1 spec already written). Then M4: embed against two consumers to prove the
contract is renderer-agnostic.

---

## 4. What changed vs the prior plan

- **No architecture change.** Node-as-truth + systems + extraction-seam +
  incremental changed-set is the validated, dominant pattern. Locked in.
- **Script/SAI explicitly demoted** from "next in M1" to "after M2/M2.5/M3,"
  empirically justified (4.2% vs binding's 24.5%).
- **PROTO expansion promoted** to the M1 closeout (6% > Script's 4.2%).
- **A dirty-tracking layer (M2a) is named as the connective tissue** between the
  M1 cascade, the M2 transform/bounds systems, and the M2.5 extraction seam —
  previously implicit, now the explicit foundation, modeled on `HdChangeTracker`.
- **M3 reframed as the moat validation**, because prior art does not prove the
  "chase the external ISO spec via codegen" thesis — only the project's own
  conformance run can.
- **One open decision deferred to M2.5:** pull vs push at the extraction seam
  (recommended default: pull).

## 5. Sources (high-confidence, verified)
- USD/Hydra: scene delegate / render index / render delegate; `HdChangeTracker`;
  `usdGenSchema` + custom-code preservation — openusd.org, docs.nvidia.com.
- VRVis HPG '13 *Lazy Incremental Computation* — cg.tuwien.ac.at.
- ISO/IEC 19775-1 (concepts, core, interpolators, navigation) — web3d.org.
- Castle Game Engine X3D node model — castle-engine.io/x3d.

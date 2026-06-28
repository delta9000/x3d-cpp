# X3D Workbench — Product Architecture & Interaction Design

> **Status:** draft consolidation (2026-06-28). Supersedes the framing of
> `2026-06-28-x3d-workbench-design-language.md`, which remains as the origin
> record. This doc is **decoupled from the visual design language**: it governs
> product architecture, data contracts, and interaction. The visual language
> (type/density tokens, component inventory, the status glyph+colour registry)
> lives in `2026-06-28-x3d-workbench-visual-language.md`.
>
> **Home:** stays in `docs/plans/` while it is a rough plan. When it matures,
> promote the durable content to `docs/wiki/subsystems/workbench.md` (gated by
> `docs-drift` / `docs-build`; needs an `mkdocs.yml` nav entry + a `coverage.md`
> row). Not yet — intentionally a working draft.
>
> **Provenance:** consolidates four review passes — an X3D subject-matter panel
> (spec/render/runtime/H-Anim), a developer-tools interaction review, a
> longevity review, and two smoke tests (against the codebase, and against prior
> art: LSP, glTF-Validator, WPT, RenderDoc). Findings are folded in only where
> verified.

## Purpose

The current `examples/poc_renderer` target should split into three related but
distinct products:

1. **Minimal SDK example** — a small, readable x3d-cpp consumer that proves
   parse, runtime tick, extraction, and rendering can be embedded without policy
   or UI baggage. No ImGui. Stays intentionally plain.
2. **Scene inspector/editor** — an interactive workbench for understanding and
   eventually editing X3D scenes.
3. **Conformance browser** — a corpus-facing browser that explains what x3d-cpp
   parsed, ignored, extracted, rendered, or rejected.

This doc governs products 2 and 3, and the shared core beneath them.

## Product Thesis

The workbench is an **X3D truth surface**: a tool that makes the full pipeline
visible and navigable. The core interaction is *click anything and see the chain*
from authored X3D node to runtime state to extracted render item to GPU draw.

```text
Source -> Scene Graph -> Runtime -> Extraction -> Shading/Material -> GPU Draw
```

The pipeline has six edges, not five. The **Shading/Material** stage (lit vs
unlit decision, GLSL build, uniform binding, `ComposedShader` bind-or-not) is
where most rendering divergence lives, so it gets its own edge and its own
inspector layer. A feature earns its place by making at least one edge clearer.

## Design Influences

Combine, without being defined by any one:

- A viewer's directness (load, navigate, inspect, jump viewpoints, capture).
- A debugger's honesty (show exact runtime/extraction/fallback/render state).
- An editor's locality (structure, fields, controls beside the selected thing).
- A conformance tool's precision (support status per scene/node/field/asset/path).

Internal risks to avoid: "supports everything" vagueness; full-editor sprawl
before inspection is excellent; dead logs that cannot navigate; hidden runtime
behaviour (ROUTEs, sensors, bindings, time); UI that needs setup before showing
value.

---

# Core Contracts

The product lives or dies on a handful of data contracts. Every review pass
converged here: the four-zone shell is easy; the spine is where it succeeds or
calcifies. Lock these before panels switch on them.

## C1 — Selection: address by path, not by enum

The original design modelled selection as a closed variant set
(`None/Scene/Node*/RenderItemId/TextureRef/…`). **No mature inspection tool does
this** — glTF-Validator addresses any object by JSON Pointer, LSP by `range`,
browser DevTools by DOM path, compilers by `file:line:col`. The SDK already has
the right primitive: `RenderItem` identity is **per-path** (`PathKey`,
`runtime/extract/RenderItem.hpp`). Three independent lines — prior art, the
codebase, and the "I can't select a field" review finding — agree.

A `Selection` is therefore:

```text
Selection {
  scene:   SceneId            // scoping; corpus mode switches scenes
  target:  Path               // in-session = PathKey (raw ptrs); persisted = serializable DEF+index path; addresses
                              // any node at any depth, including DEF/USE instances
  field?:  FieldRef           // optional: a specific field of the target
  kind:    SelectionKind      // lightweight tag for panel routing/icons
}
```

Rules:

- **Field-addressable.** A warning like `ImageTexture.url[0] failed` selects the
  field, not just the node. The milestone's "highlight the relevant field"
  depends on this.
- **DEF/USE instances are distinguished** because identity is per-path — each
  placement is a distinct path. (This refutes the panel's worry that a viewport
  pick couldn't tell instances apart; the capability exists, it just needs
  stating.)
- **One global selection** drives every surface (tree, viewport pick, warning
  list, conformance rows, route events, texture browser). Panels are independent
  consumers.
- **Selection history** (back/forward + breadcrumb) and **hover ≠ select** (a
  transient `hovered` distinct from the committed selection) are required — both
  are table stakes for inspection tools and both are architecture decisions, not
  later polish.
- **Multi-select** is a set, not a scalar. "Toggle wireframe for the selection",
  corpus compare, "select all Unsupported" all need it. Decide set-semantics now.
- **Two identity tiers — the persistent one is net-new work.** In-session,
  selection and picking use `PathKey` (the raw `vector<const X3DNode*>` root..leaf
  chain — collision-safe but session-scoped; the pointers die on reload). Never
  persist `RenderItemId` (a dense `uint32`, stable only "while topology is stable"
  — not across edits or commits), and do **not** persist a raw `PathKey` either.
  Bookmarks, regression baselines, and captured evidence need a **serializable**
  stable path (DEF name + child-index chain), which does not exist yet (see Engine
  Prerequisites).
- A **warning is not a selection state.** It is a navigation *action* that
  resolves to a `(target, field, inspector-section)`. It lives in Diagnostics and
  produces selections; it is never itself the selected thing.

## C2 — Diagnostics: adopt the LSP shape

The SDK already emits *typed* diagnostics, but **fragmented** across subsystems
(`ProtoWarning.Kind`, `InlineWarning.Kind`, `RangeDiagnostic`, collected into
separate `X3DDocument` fields). The work is not "add types"; it is **unify them
into one stream** with the field set LSP converged on after a decade of editor
tooling:

```text
Diagnostic {
  code:            string       // STABLE identifier — the regression signature key
  severity:        Error | Warning | Info | Hint
  message:         string       // presentation only; never the identity
  target:          Selection    // where it points (C1)
  tags:            Tag[]         // Inert (render faded), Fallback (strikethrough/marked)
  codeDescription: URI?          // deep link to the findings.yaml row / ISO clause
  related:         Diagnostic[]  // e.g. "fallback active because <this other thing>"
}
```

- **`code` is a stable contract**, not display text — the same discipline as
  glTF-Validator message codes and ESLint rule IDs. Cross-commit regression and
  saved "failure signatures" key on `code`; renaming one is a breaking change.
  Message wording can change freely.
- **`tags` carry the parsed-but-inert axis.** LSP's `DiagnosticTag.Unnecessary`
  renders faded, `Deprecated` renders struck-through. "Parsed-but-inert" and
  "fallback" are *tags* (a rendering treatment orthogonal to severity), not new
  status chips. This collapses chip proliferation into a clean 2-axis model.
- **`codeDescription`** is the mechanical home for the spec-citation requirement:
  point it at the `findings.yaml` row (`id`, `clause`) so every warning is
  traceable to the standard and to our tracker.

## C3 — Status vocabulary: two axes, one data-driven registry

There is **no single SDK enum** to derive chips from. The SDK has fragmented
per-resolver triads (`TextureResolveStatus`/`AssetStatus`/`FrameStatus`/
`GlyphStatus` = `Ready/Pending/Failed`) and the conformance pipeline has a
*lifecycle* status (`open/deferred/fixed/closed` in `findings.yaml`). These are
**different axes** and must not be conflated:

- **Disposition** (per node / per render item): what the pipeline did with it.
  Derived by *aggregating* the fragmented SDK signals (resolver status +
  extraction presence + render outcome). This is a normalization layer the
  workbench owns.
- **Conformance lifecycle** (per finding): sourced read-only from
  `findings.yaml`. The browser joins the two axes; it never collapses them.

The disposition vocabulary is a **registry** — a data table, one row per status
(`id, label, glyph, colour, precedence`), owned by one module. Adding a status
edits the table, not a `switch` in every panel. Rows:

| Status | Meaning |
|---|---|
| `Rendered` | Drew **and** every extracted attribute was consumed by the GPU path. |
| `Rendered (partial)` | Drew, but ≥1 extracted attribute was dropped at GPU. Must enumerate them. |
| `Nonconformant` | Drew, but known to disagree with the spec (e.g. a transposed alpha enum). |
| `Extracted` | Produced render data, not currently visible. |
| `Parsed` | In the document model, not active in runtime/render. Annotate the stage it stopped at. |
| `Runtime` | Affects behaviour, no direct draw item. |
| `Bound` | Currently bound bindable (Viewpoint, NavigationInfo, Background, Fog, …). |
| `Stacked` | On a bindable stack but not the bound top. |
| `Animated` | Driven by time/ROUTE/interpolator/sensor/follower. Drill-down names which. |
| `Fallback` | Substituted because the authored thing failed/unsupported (white texture, unlit-because-no-normals). |
| `Default` | Spec-correct behaviour in absence of an authored value (headlight, creaseAngle normals). Not a warning. |
| `Unsupported` | Known node/field, no implementation. |
| `Ignored` | Deliberately a no-op though implementable. Requires a cited reason. |
| `Invalid` | Malformed/unusable scene data. |
| `Missing Asset` | External resource resolution failed. |

Discipline that keeps the vocabulary honest:

- **Green is earned, not given.** `Rendered` (full) is *unreachable* whenever the
  GPU stage drops anything Extraction surfaced — see C4. A scene of dropped
  shadows and uncut MASK alpha must not read as a pass on a thumbnail wall.
- **`Default` ≠ `Fallback`.** The discriminator is: *would a correct independent
  implementation do the same?* If yes (headlight, creaseAngle-generated normals,
  default material values), it is `Default` and not a warning — otherwise every
  legacy VRML file lights up yellow and the corpus view drowns in noise.
- **`Parsed` carries its stop-stage** ("Parsed · stopped before Runtime",
  "Extracted · dropped at GPU"). A reviewer needs to know *which edge* a feature
  fell off, because the fix lives in different code.
- **Encode redundantly.** Glyph + label + colour, never colour alone (WCAG 1.4.1;
  the eleven-to-six colour collision; LSP's faded/struck non-colour channels).
  The glyph alone must disambiguate.

## C4 — Pipeline fidelity: Extraction↔GPU reconciliation

The most valuable single view in the tool, and the one prior art (RenderDoc's
"bound-but-unused", CSS computed-pane's struck-through overrides) already
validates. For every attribute Extraction produced, the GPU layer shows its
disposition:

```text
alphaMode:   MASK (extracted) -> OPAQUE (gpu, DROPPED)     // the transposed-enum class, made visible
anisotropy:  16  (extracted) -> 1      (gpu, DROPPED)
wrapS:       CLAMP (extracted) -> applied
shadows:     TRUE (extracted) -> not rendered (DROPPED)
```

The summary chip is the **minimum fidelity** across these rows. This is what
makes the seam gaps honest instead of invisible: the extraction seam is narrower
than the node model (Fog, per-light `shadows`/`shadowIntensity`, `Background`
panorama, NURBS, 2D primitives, `lineWidth` are parsed but unsurfaced), and this
reconciliation is where that narrowness becomes a feature rather than a silent
lie.

---

# Design Principles

- **Inspection first, mutation second.** Editing starts as safe, local mutation
  of visible properties, then grows. Do not make "edit mode" the centre until
  selection, explanation, and diagnostics are strong.
- **Selection drives context** (see C1).
- **Evidence near controls.** Mesh controls near mesh facts; texture controls
  near texture status; viewpoint commands next to viewpoint rows. Detached
  settings pages only for genuinely global settings.
- **Warnings are navigation** (see C2). A warning is a typed link into the scene,
  not a string.
- **Honest status vocabulary** (see C3).
- **Truth before polish.** Prefer `Parsed`/`Ignored`/`Fallback`/`Unsupported`/
  `Invalid` over silence. A clean viewport hiding dropped behaviour is worse than
  a noisy one with precise explanation.
- **Consume the seam, widen the seam — never reach past it.** The workbench reads
  only stable SDK surfaces. When it needs data the seam does not carry, the fix
  is to widen the seam (which the status vocabulary then renders honest), *not*
  to read fields off the node. Precedent: `examples/cpu_raster/main.cpp` reads
  `Background` panorama URLs by field name straight off the node, bypassing
  extraction — that line breaks the instant the seam changes. The workbench is
  far larger and must not repeat it.
- **Failure is a first-class state.** Render useful structure from a partial or
  failed parse (the tree from a half-parsed document; the diagnostic that
  explains a hard failure). Never a blank panel. Design the loading, empty,
  parse-failed, and asset-missing states explicitly — for a truth tool they *are*
  the product.

# Design Lenses

Review every panel, control, overlay, and workflow against these. They double as
a **PR checklist** (see Governance). A feature needs a clear answer for the
relevant ones; weak answers mean it is not ready for the base.

1. **Pipeline** — does it make one of the six edges clearer?
2. **The selected thing** — does it create, explain, or act on selection?
3. **The next question** — what adjacent follow-up does it make easy? (`Texture
   failed` → path/resolver/source/fallback; `Unlit` → *material model first*,
   then normals/topology/shader; `Animated` → which driver + current value;
   `Bound Viewpoint` → matrix/fields/jump.)
4. **Truth before polish** — is it honest when the result is incomplete?
5. **Locality** — is the control beside the evidence it changes?
6. **Reversibility** — can the user poke safely? Distinguish *view toggles*
   (wireframe, bounds, headlight — ephemeral, no history) from *edits* (material,
   transform — need the command stack, see Architecture).
7. **Corpus scale** — does it still work across **thousands** of scenes (the
   curated Web3D archive is ~4000), not just hundreds? Aggregation, virtualized
   lists, off-thread indexing.
8. **Embedder clarity** — does it keep the minimal SDK example small? Heavy UI
   state, policy, caching, corpus logic, diagnostics belong in the workbench, not
   the SDK example. This lens is longevity-critical: it fences ImGui/GL/corpus
   deps away from the embeddable SDK.
9. **Evidence capture** — can the user preserve what they learned (screenshot,
   copied viewpoint, diagnostic text keyed by `code`, conformance row)?
10. **Implementation gravity** — does it strengthen the shared models
    (`Selection`, `Diagnostics`, `SceneSession`, `Renderer`) rather than create
    one-off UI state or special-case render paths?

# Subsystem Coverage

The vocabulary and the chain must reach the demanding subsystems, not just the
textured box.

- **H-Anim is first-class, not absent.** `HAnimHumanoid`/`Joint`/`Segment`/
  `Site`/`Displacer` are selectable kinds; the skeleton is a real subtree in the
  left pane; the viewport has a **skeleton overlay** (joint centres + parent→child
  bones) and a skin-vs-skeleton view. The inspector surfaces `center` (not
  `translation`) prominently for a Joint and flags a non-zero `translation` as a
  likely error. The conformance browser checks **animation portability** (required
  joints for the LOA, canonical `l_`/`r_` naming, rooting at HumanoidRoot). Today
  the runtime treats H-Anim skin/Displacer/Motion as inert — so the honest
  milestone surface for a real humanoid is "renders in bind pose; skeleton parsed;
  skin does not deform; motion ignored" shown loudly via a `Parsed (inert)` tag,
  not a clean still figure with a green `Animated` chip.
- **Seam gaps surfaced honestly.** Fog, per-light shadows/`shadowIntensity`,
  `Background` panorama, NURBS, 2D primitives, `lineWidth` render as
  `Unsupported`/parsed-inert with their stop-stage. A `Bound` Fog whose values the
  renderer ignores is exactly the dishonest-green case the vocabulary exists to
  prevent.
- **Runtime honesty.** A Script-`eventIn` ROUTE that `deliver()` never dispatches
  (it walks static `fields()`, not `effectiveFields()`) is shown as `Dropped`,
  **not** drawn as an event pulse. A `ProximitySensor` in a culled `Switch`/`LOD`
  branch shows both `enrolled` and `in-active-branch` so the discrepancy is itself
  navigable.

---

# Layout Grammar

Four zones, but flexible:

```text
Left:   Scene structure        Center: Viewport (or corpus grid, in Corpus mode)
Right:  Inspector              Bottom: Console, timeline, corpus, event log
```

- **All panes collapsible/poppable.** "Viewport primary" is achieved by letting
  the user collapse the other three, not by fixing four panes on a laptop frame.
- **Modes carry layout presets.** `Corpus` mode replaces the centre viewport with
  a virtualized scene grid — that is a layout change, not an overlay. Treat modes
  as Blender-style workspaces (layout presets), or make Corpus a peer surface.

### Left: Scene structure

Scene-graph outline; DEF/USE indicators; layers and bindable stacks; viewpoints;
routes; **the H-Anim skeleton subtree**; filters (rendered/unsupported/animated/
warnings). When a *field* is selected, show its **incoming/outgoing ROUTEs** —
the local fan-in/fan-out, the X3D analogue of DOM "event listeners" and the
single highest-value runtime view. PROTO/ExternProto instances: state whether
expanded internals are selectable and whether they read as the instance or the
PROTO body.

### Center: Viewport

Overlays explain the scene: selection outline; node/mesh/scene bounds (authored
`bbox` vs computed, flag mismatch); normals/tangents (legend states *which*:
authored vs generated, the winding actually used; grey out tangents where no
normal map is bound); wireframe (the post-extraction GPU index buffer, so it
audits real triangulation); **skeleton**; light vectors (with per-light
`shadows: requested → not rendered` when dropped); viewpoint frustum;
dispatched-vs-dropped route pulses; unsupported/fallback markers.

### Right: Inspector

Layered from human summary to implementation detail, with the Shading stage added
and the C4 reconciliation made visible:

1. **Summary** — node type, DEF/USE, status chips, short reason.
2. **X3D fields** — actual fields grouped by semantic category and access type.
   (For a Joint, `center` leads.)
3. **Runtime** — bindings, sensors (enrolled vs active-branch), active time
   state, routes, current values *at end-of-tick* (see Timeline).
4. **Extraction** — `RenderItem`, geometry id, material descriptor, lights,
   bounds, texture refs (with `SamplerParams`: wrap/filter/mipmaps/anisotropy,
   channel count, colour space).
5. **Shading/Material** — lit vs unlit decision, alpha mode (Opaque/Mask/Blend +
   cutoff/blend func), shader path, `ComposedShader` bound-or-not.
6. **GPU** — draw mode, culling, depth write, texture handles, mesh counts, **and
   the per-attribute Extraction→GPU reconciliation** (applied/dropped/default).

### Bottom: Console, timeline, corpus

Clickable diagnostics (keyed by `code`); a **dispatched-vs-dropped** ROUTE/event
log; pause/step/**reset-and-replay**; screenshot and deterministic animation
capture; the corpus/conformance table in Corpus mode.

**Timeline honesty.** "Current value" means the **end-of-tick snapshot** (the
settled value), not mid-cascade. v1 offers pause + forward-step + reset-to-t0; it
does **not** offer free backward scrub, because followers and Scripts are stateful
integrators and time is not a pure function of `t`. Free scrub is deferred unless
the design commits to full deterministic re-simulation from t0.

# Modes

Modes clarify intent (and carry layout presets), they do not hide features:

- `Explore` — navigate, select, inspect, jump viewpoints.
- `Debug` — renderer/runtime overlays, routes, sensors, unsupported nodes,
  fallback explanations.
- `Capture` — screenshots, deterministic animation dumps, viewpoint bookmarks.
- `Corpus` — virtualized batch navigation, conformance status, failure signatures
  (centre pane becomes a grid).

No top-level `Edit` mode until inspection is mature. Editing begins as local
controls inside the inspector.

# Visual Language

**Decoupled.** This doc does not specify the visual language. The separate
artifact (`2026-06-28-x3d-workbench-visual-language.md`) owns the type/density
tokens, component inventory, the status glyph+colour registry rows, and
interaction states. The constraints this doc places on it: technical
instrument-panel tone (dark neutral, compact rows, thin separators, sparse
status colour, viewport-first, no decorative chrome); and the hard requirement
that **status is encoded redundantly** (glyph + label + colour), satisfying
C3 and WCAG 1.4.1.

---

# Corpus & Conformance

- **The browser is a pure view over the conformance data**, never a second source
  of truth. It reads `report.json`/`model.json`; it surfaces `findings.yaml`
  rows (`id`, `clause`, `severity`, `status`) and lets the user copy them; it
  exports *through the existing generator* (`scripts/conformance_view.py`,
  `mise run conformance`). The generated `.md` is never hand-edited;
  `findings.yaml` is the only editable surface.
- **Reference-image comparison is a VLM semantic judgment, with a deterministic
  pre-filter — not a pixel diff.** X3D is a behavioural standard, so the primary
  question is "did we render the *same 3D subject*", which tolerates legitimate
  differences in antialiasing, lighting, tessellation, camera nuance, and
  resolution. This is already proven in-repo: `scripts/visual_conformance_sweep.py`
  renders the PoC against the Web3D/NIST front references and asks a **local
  vision model** (`gemma-4-12b-it`, OpenAI-compatible endpoint) for a
  match/minor/mismatch/render_fail/no_ref verdict. The conformance browser should
  surface those buckets plus the model's stated reason. Layer three questions,
  each answered by the cheapest tool that can:
  1. **Deterministic pre-filter** (cheap, bit-reproducible): a bounded pixel/fuzzy
     check (WPT-style `maxDifference` + `totalPixels`) catches identical/near-
     identical renders for free and is the stable **regression-pin** across
     commits.
  2. **VLM semantic judge** (the primary): for everything the pre-filter doesn't
     settle, the local vision model judges "same subject" tolerantly.
  3. **Human spot-check**: triages the VLM's `minor`/`mismatch` buckets.
- **Make the VLM verdict reproducible enough to track over time.** A vision-model
  verdict is not bit-reproducible, so pin the model id, set temperature 0, freeze
  the prompt/rubric (and its token budget — `gemma-4` is a reasoning model and
  "thinking eats the token budget"), and **store the verdict *and* the reason
  text** keyed by `PathKey` so a human can audit drift when the model or prompt
  changes. VLM judgment is a **diagnostic that ranks and triages, not a hard CI
  gate** (the existing sweep "always exits 0" for exactly this reason). Pin
  viewpoint/clock/window for the *render* side regardless, and label every
  reference by provenance.
- **Corpus scale is virtualized + indexed off-thread.** List virtualization (in
  ImGui this is manual — `ImGuiListClipper`), background/cancelable incremental
  indexing that never blocks the frame, async thumbnail loading with
  placeholders, a persisted status index so reopening does not re-scan. Anchor on
  the curated, royalty-free Web3D archive and ingest its existing metadata
  (titles, identifiers, licences, reference images) rather than re-deriving it.
- **Regression keys on stable identifiers** — `Diagnostic.code` for failure
  signatures, and a **serializable** path for object identity (raw `PathKey` is
  session-only; the serializable form is a prerequisite) — so a baseline captured
  today is still comparable next year.

# Round-trip & Export

Export honesty is a promise the moment "Export patched scene" exists:

- Export in the **source encoding** (XML/ClassicVRML/JSON); never silently
  normalize. Round-trip is **lossless for unedited subtrees**.
- Preserve `head`/`meta` provenance, **DEF-before-USE** order (H-Anim's flat
  `joints`/`segments`/`sites` lists are all USEs), ROUTE ordering, and comments.
  A patched scene is a **minimal diff**, not a full rewrite.
- **Validate against the X3D Schema before writing.** A patched scene that no
  longer validates is worse than no export.

# Architecture / Implementation Shape

Keep UI state separate from renderer state. The data model is **not** ImGui-shaped
— treat it as an **inspection protocol** (serializable, the way LSP/DAP/CDP
separate inspector backend from frontend), which also enables a headless
conformance path:

```text
WorkbenchApp
  Selection    selection      // C1: path-addressed, history, hover, multi-select
  Diagnostics  diagnostics    // C2: unified LSP-shaped stream, stable codes
  UiSettings   ui
  SceneSession scene
  Renderer     renderer
  CommandStack history        // edits are diffs/commands — decide BEFORE the editor lands
```

- **The shared core is an extracted, versioned library** that both the inspector
  and the conformance browser link — not copy-paste. State its public surface.
- **Mutation is command/diff-based**, not in-place. This is the load-bearing
  call that protects round-trip fidelity and stops the editor from corrupting the
  models the conformance browser reads. Pick it now, even unbuilt.
- **Picking resolves through the extraction layer's stable ids** (`PathKey`), not
  through any one GPU backend, so cpu_raster and the GL poc agree on what was
  selected. (The repo's differential-test discipline will otherwise catch the
  disagreement the hard way.)
- **Build/CI.** The workbench joins the **existing** examples gate
  (`scripts/validate-examples.sh`, run by `mise run validate-examples` and CI's
  examples-gate, built expressly so SDK refactors like ADR-0039 can't silently rot
  example consumers). Residual gap to close: `validate-examples` is **not** in the
  `mise run ci` aggregate's `depends`, so local `ci` is green while only GitHub
  Actions catches example breakage — add it. Apply `GLFW_INCLUDE_NONE` so the
  gate builds on runtime-only GL.

Panels are pure consumers of the shared models:

```text
SceneTreePanel(selection, scene, diagnostics)
ViewportOverlay(selection, renderer, ui)
InspectorPanel(selection, scene, renderer, diagnostics)
WarningsPanel(selection, diagnostics)
TimelinePanel(scene, ui)
CorpusPanel(selection, corpus, diagnostics)
```

# Engine Prerequisites (before building in earnest)

The design surfaces unimplemented *features* (Fog, per-light shadows, NURBS,
`Background` panorama, H-Anim skin/Motion, line-width) as honest `Unsupported` /
`Parsed (inert)` gaps, so **none of those block the workbench**. What blocks it is
the engine's ability to *report what it did*. Three items are net-new; the rest
are already done or cheap to verify. The three are tracked on GitHub Project #2
(method-attributed).

1. **Diagnostics subsystem (net-new, longest pole).** Today diagnostics are
   parse-time only (`X3DDocument.{rangeWarnings, protoWarnings, inlineWarnings}`)
   plus ad-hoc string islands (`ShaderBindingPlan.diagnostics`;
   `MaterialSystem.hpp:278` "surfaced as a diagnostic; not enforced"). The
   extract/runtime/render pipeline emits no unified, typed, navigable diagnostics
   — so contract C2 and roughly half of C3 have no data source. Build the C2
   stream and thread its sink through extraction, runtime, and render; unify the
   three parse-time `Kind` enums into it.
2. **Disposition + Extraction↔GPU reconciliation data (net-new).** No per-item
   "what I produced" manifest and no renderer "what I consumed vs dropped"
   instrumentation exist — only fragmented resolver triads
   (`TextureResolveStatus` etc.). C3/C4 are unbuildable (only fakeable) until both
   exist.
3. **Serializable selection path + `FieldRef` (partly net-new).** In-session
   selection/picking already work (`PathKey` + `PickSystem::pickClosest`). But
   `PathKey` is raw `vector<const X3DNode*>` — session-scoped, not serializable —
   so capture, bookmarks, and cross-commit regression need a serializable stable
   path (DEF + child-index), and field-level selection needs a `FieldRef`
   primitive. Neither exists.

Already done / verify-only (not blockers):

- Crash guards on deep nesting — **done** (`runtime/RecursionLimits.hpp` budgets
  `SceneExtractor` / `PickSystem` / `LightSystem`; non-throwing `budgetExceeded`).
- Sensor enrollment/culling honesty — **largely done** (the `ENV-*` wave; e.g.
  `findings.yaml` `ENV-01` closed with a wired System).
- **Verify (cheap):** that `alphaMode` MASK actually discards (reworked to
  token-parse with an `AUTO` default, `MaterialSystem.hpp:187`); and that
  `X3DEventCascade::deliver()` resolves targets via `effectiveFields()` so
  Script/PROTO author-declared eventIn ROUTEs land.

Scaffolding (parallelizable, not engine internals): extract the shared core into a
versioned library; add a workbench target to `scripts/validate-examples.sh`; add
`validate-examples` to the `mise run ci` `depends`.

# First Milestone Worth Building

```text
Selectable Scene + Explain Selected Draw
```

Required behaviours: select from tree and from viewport, converging on one
**path-addressed** `Selection`; show selected node and selected render item; show
parsed/extracted/rendered/fallback/ignored/unsupported state; show material,
mesh, shader path, texture status, culling, blend mode; toggle wireframe/bounds/
normals for the selection; click a warning and navigate to the relevant object
**or field**.

### Acceptance lenses

- **Pipeline** — a selected visible object shows source node, runtime/extraction
  identity, material, mesh, shader path, draw state.
- **Selection** — tree and viewport selection converge on the same path-addressed
  `Selection`, and a *field* is addressable.
- **Diagnostics** — at least one diagnostic carries a stable `code` and a `target`
  and navigates to the relevant object/field.
- **Fidelity** — at least one item shows the Extraction→GPU reconciliation, with a
  dropped attribute rendered as `Rendered (partial)`, not green.
- **Truth** — parsed-only, fallback, unsupported, ignored, and rendered states are
  visibly distinct (glyph + label, not colour alone). One genuine partial case is
  shown honestly (e.g. a humanoid in bind pose, or Fog parsed-inert).
- **Corpus** — the same disposition vocabulary summarizes more than one scene.
- **Embedder split** — the minimal SDK example remains free of workbench UI deps.
- **Capture** — a screenshot or textual diagnostic for the selection survives the
  session, keyed by a serializable path (not raw `PathKey`, which is session-only,
  nor `RenderItemId`). See Engine Prerequisites.

This milestone establishes the architecture and the contracts. Everything after
is additive.

# Non-Goals For The Base

- A full authoring suite before the inspection workflow is strong.
- A material/node editor before inspection is strong.
- Project files, asset libraries, or scene packaging.
- A second conformance source of truth (the browser is a *view*).
- A visual design language *in this doc* (separate artifact).
- Free backward time-scrub in v1.
- Hiding unsupported features to make demos look cleaner.

# Governance / Provenance

- **Home:** stays in `docs/plans/` as a rough working plan for now. When it
  matures, promote the durable content to `docs/wiki/subsystems/workbench.md`
  (gated by `docs-drift` / `docs-build`; add to `mkdocs.yml` nav + a `coverage.md`
  row).
- **Binding decisions → ADRs** (next is 0042+): the three-product split;
  selection-as-path (C1); diagnostics-as-unified-stream (C2); status-as-registry
  (C3); data-model-as-inspection-protocol; mutation-as-commands. Decisions that
  live only in a plan get re-litigated; decisions in ADRs get cited.
- **Lenses → PR checklist** attached to the workbench subsystem page.
- **RAG refresh** when symbols move: `mise run code-ingest`, `mise run
  docs-ingest`.

The base succeeds when it makes x3d-cpp's internal truth visible, navigable, and
trustworthy — and when the contracts above are enforced by structure (wiki gates,
a versioned shared library, ADRs, stable codes/paths) rather than by prose.

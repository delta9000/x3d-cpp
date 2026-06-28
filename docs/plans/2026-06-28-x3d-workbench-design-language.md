# X3D Workbench Design Language

## Purpose

The current `examples/poc_renderer` target should eventually split into three
related but distinct products:

1. **Minimal SDK example:** a small, readable X3D SDK consumer that proves parse,
   runtime tick, extraction, and rendering can be embedded without policy or UI
   baggage.
2. **Scene inspector/editor:** an interactive workbench for understanding and
   eventually editing X3D scenes.
3. **Conformance browser:** a corpus-facing browser that explains what x3d-cpp
   parsed, ignored, extracted, rendered, or rejected.

The design language below is for the second and third products. The minimal SDK
example should stay intentionally plain.

## Product Thesis

The workbench is not a generic 3D editor and not just a model viewer. It is an
X3D truth surface: a tool that makes the full pipeline visible and navigable.

```text
Source -> Scene Graph -> Runtime State -> Extraction -> GPU Draw
```

The core interaction is: click anything and see the chain from authored X3D node
to runtime state to extracted render item to GPU draw. The chain itself is the
central user experience.

## Design Influences

The workbench should combine a few proven interaction patterns without becoming
defined by any one existing product category:

- A viewer's directness: load a scene, navigate it, inspect what is visible, jump
  between viewpoints, and capture evidence.
- A debugger's honesty: show the exact runtime, extraction, fallback, and render
  state instead of smoothing over partial behavior.
- An editor's locality: keep structure, fields, and controls close to the thing
  the user selected.
- A conformance tool's precision: make support status visible per scene, node,
  field, asset, and render path.

Avoid these internal design risks:

- "Supports everything" vagueness.
- Full-editor sprawl before inspection is excellent.
- Dead logs that cannot navigate to the offending scene object.
- Hidden runtime behavior, especially ROUTEs, sensors, bindings, and time.
- UI features that require setup before showing value.

## Design Principles

### Inspection First, Mutation Second

The first-class action is understanding the scene. Editing should start as safe,
local mutation of visible properties, then grow into authoring workflows. Do not
make "edit mode" the center until selection, explanation, and diagnostics are
strong.

### Selection Drives Context

One global selection model should drive the whole UI:

```text
None
Scene
Node*
RenderItemId
TextureRef
LightDesc
Route
Viewpoint
Warning
CorpusScene
```

The scene tree, viewport picking, warning list, conformance rows, route events,
and texture browser should all update the same selection. Panels should be
independent consumers of that selected object.

### Evidence Near Controls

Controls should live beside the evidence they affect. Mesh controls belong near
mesh facts. Texture controls belong near texture status. Viewpoint commands
belong next to viewpoint rows. Avoid detached settings pages unless the setting
is genuinely global.

### Warnings Are Navigation

A warning is not a string; it is a typed link into the scene.

Examples:

- `ImageTexture "foo.png" failed to resolve`
- `No normals: using unlit path`
- `No authored lights: headlight fallback active`
- `ComposedShader parsed but not bound`
- `NURBS surface tessellated fallback`
- `MovieTexture frame decode failed`

Clicking a warning should select the relevant node or render item, open the
correct inspector section, and highlight the relevant field or pipeline stage.

### Honest Status Vocabulary

Use the same status chips everywhere: scene tree, inspector, warning list,
conformance browser, and corpus summary.

- `Rendered`: produced visible GPU output.
- `Extracted`: produced render data, even if not currently visible.
- `Parsed`: present in the document model but not active in runtime/render path.
- `Runtime`: affects behavior but has no direct draw item.
- `Bound`: currently bound Viewpoint, NavigationInfo, Background, etc.
- `Animated`: affected by time, ROUTEs, interpolators, sensors, or followers.
- `Fallback`: substituted behavior such as headlight, white texture, default
  material, unlit path, or generated normals.
- `Unsupported`: known node or field with no implementation.
- `Ignored`: accepted but intentionally dropped.
- `Invalid`: malformed or unusable scene data.
- `Missing Asset`: external resource resolution failed.

These labels are the UI grammar. A user should learn them once and recognize them
across every surface.

## Design Lenses

Use these lenses to review every proposed panel, control, overlay, and workflow.
A feature does not need to satisfy every lens equally, but it should have a clear
answer for the relevant ones. If the answers are weak, the feature is not ready
for the base workbench.

### Lens 1: The Pipeline

Question: does this feature reveal or improve one step of the pipeline?

```text
Source -> Scene Graph -> Runtime State -> Extraction -> GPU Draw
```

Strong features make at least one pipeline edge clearer. Weak features add UI
without explaining where the scene came from, how it changed, or why it rendered
that way.

### Lens 2: The Selected Thing

Question: what happens when the user selects something?

Every major surface should either create selection, explain selection, or act on
selection. Avoid panels that require the user to restate context already known to
the app.

### Lens 3: The Next Question

Question: after the UI answers one fact, what natural follow-up question does it
make easy?

Examples:

- `Texture failed` should lead to path, resolver, source node, and fallback.
- `Unlit path` should lead to normals, topology, material model, and shader.
- `Animated` should lead to TimeSensor, interpolator, ROUTE, and current value.
- `Bound Viewpoint` should lead to camera matrix, authored fields, and jump/copy
  commands.

Discoverability comes from this chain of adjacent questions.

### Lens 4: Truth Before Polish

Question: is the UI telling the truth even when the result is incomplete?

The workbench should prefer `Parsed`, `Ignored`, `Fallback`, `Unsupported`, and
`Invalid` over silence. A clean-looking viewport with hidden dropped behavior is
less useful than a noisy viewport with precise explanation.

### Lens 5: Locality

Question: is the control beside the evidence it changes?

Controls should sit near the rows, overlays, warnings, or status chips they
affect. Global settings are acceptable only for genuinely global behavior.

### Lens 6: Reversibility

Question: can the user poke safely?

The base workbench should make inspection and temporary overrides cheap:
wireframe, bounds, headlight, culling, material color, time pause, and viewpoint
jumps should be easy to reset. Persistent edits require a clearer save/export
boundary.

### Lens 7: Corpus Scale

Question: does this still work across hundreds of scenes?

A feature that helps one hand-picked scene but cannot summarize corpus behavior
belongs in a demo panel, not the base. Status chips, warning signatures, and
support categories should aggregate cleanly.

### Lens 8: Embedder Clarity

Question: does this clarify or obscure the SDK embedding story?

Workbench UI may be rich, but the minimal SDK example must stay small and
readable. Any feature that needs substantial UI state, policy, caching, corpus
logic, or diagnostics belongs in the workbench/conformance products, not the SDK
example.

### Lens 9: Evidence Capture

Question: can the user preserve what they learned?

Important discoveries should be capturable as screenshots, copied viewpoints,
diagnostic text, warning signatures, or conformance rows. A useful inspection
tool should produce evidence that survives after the interactive session.

### Lens 10: Implementation Gravity

Question: does this feature pull architecture in the right direction?

Good early features strengthen shared models such as `Selection`, `Diagnostics`,
`SceneSession`, and `Renderer`. Risky early features create one-off UI state,
special-case render paths, or editor policy that cannot be reused by the
conformance browser.

## Layout Grammar

Use a stable four-zone workbench:

```text
Left:   Scene structure
Center: Viewport
Right:  Inspector
Bottom: Console, timeline, corpus, event log
```

### Left: Scene Structure

The left pane is the user's map:

- Scene graph outline.
- DEF/USE indicators.
- Layers and bindable stacks.
- Viewpoints.
- Routes.
- Optional filters: rendered, unsupported, animated, warnings.

### Center: Viewport

The viewport remains primary. It should explain itself through overlays:

- Selection outline.
- Node, mesh, and scene bounds.
- Normals and tangents.
- Wireframe.
- Light vectors.
- Viewpoint frustum.
- Route/event pulses.
- Unsupported or fallback markers.

### Right: Inspector

The inspector should be layered from human summary to implementation detail:

1. **Summary:** node type, DEF/USE, status chips, short reason.
2. **X3D Fields:** actual fields grouped by semantic category and access type.
3. **Runtime:** bindings, sensors, active time state, routes, current values.
4. **Extraction:** `RenderItem`, geometry id, material descriptor, lights,
   bounds, texture refs.
5. **GPU:** shader path, draw mode, culling, blend mode, texture handles, mesh
   counts.

Users should be able to stop at the summary or drill down to exact pipeline
facts.

### Bottom: Console, Timeline, Corpus

The bottom pane is for time and batches:

- Clickable warnings and diagnostics.
- ROUTE/event log.
- Pause/step/scrub time.
- Screenshot and animation capture.
- Corpus/conformance table when in conformance-browser mode.

## Modes

Use modes to clarify intent, not to hide features:

- `Explore`: navigate, select, inspect, jump viewpoints.
- `Debug`: renderer/runtime overlays, routes, sensors, unsupported nodes,
  fallback explanations.
- `Capture`: screenshots, deterministic animation dumps, viewpoint bookmarks.
- `Corpus`: batch scene navigation, conformance status, failure signatures.

Do not add a top-level `Edit` mode until inspection is mature. Editing should
begin as local controls inside the inspector.

## Visual Tone

The UI should feel like a technical instrument panel:

- Dark neutral base.
- Compact rows.
- Thin separators.
- Small, stable controls.
- Status colors used sparingly and consistently.
- Viewport-first composition.

Color semantics:

- Green: rendered, ready, complete.
- Blue: selected, bound, current.
- Yellow: fallback, partial, warning.
- Red: invalid, error.
- Gray: parsed-only, ignored, inactive.
- Purple: reserve for ROUTE/event flow or author shader concepts; do not make it
  the dominant palette.

Avoid decorative chrome. The UI should look like a debugger, browser, and
workbench, not a consumer landing page.

## Product Split

### 1. Minimal SDK Example

Goal: be the smallest high-quality embedder example.

Characteristics:

- Separate target from the workbench.
- Minimal dependencies.
- No ImGui.
- Straight-line control flow.
- Demonstrates parse, context build, tick, extraction, and one render path.
- Good comments for SDK consumers.
- Stable enough to use in docs and examples.

This example should answer: "How do I embed x3d-cpp?"

### 2. Scene Inspector / Editor

Goal: be the interactive X3D workbench.

Initial scope:

- Scene tree.
- Viewport picking.
- Selection-driven inspector.
- Render debug toggles.
- Viewpoints and navigation modes.
- Texture/material/light inspection.
- Clickable warnings.

Later editor scope:

- Safe material edits.
- Transform edits.
- Viewpoint creation from current camera.
- Route inspection and limited route creation.
- Script/shader editing only after diagnostics are strong.
- Export patched scene.

This product should answer: "What is this scene doing, and what did x3d-cpp do
with it?"

### 3. Conformance Browser

Goal: be the corpus-facing capability and failure browser.

Initial scope:

- Scene list with parse/extract/render status.
- Support chips by component/node/field.
- Failure signatures.
- Screenshot thumbnails where available.
- Link from corpus row to scene inspector.
- Compare expected reference image, rendered image, and diagnostics.

Later scope:

- Batch rerun controls.
- Regression comparison across commits.
- Per-component coverage pages.
- Export reports into `docs/conformance`.

This product should answer: "Where are we complete, partial, missing, or
regressing?"

## First Milestone Worth Building

The first durable milestone is:

```text
Selectable Scene + Explain Selected Draw
```

Required behaviors:

- Select from scene tree.
- Select from viewport.
- Show selected node and selected render item.
- Show whether it was parsed, extracted, rendered, fallback, ignored, or
  unsupported.
- Show material, mesh, shader path, texture status, culling, blend mode.
- Toggle wireframe/bounds/normals for the selection.
- Click a warning and navigate to the relevant object.

This milestone establishes the architecture and design language. Everything
after it is additive.

### Milestone Acceptance Lenses

The milestone is not complete until these checks pass:

- **Pipeline:** a selected visible object shows source node, runtime/extraction
  identity, material, mesh, shader path, and draw state.
- **Selection:** selecting from the tree and selecting from the viewport converge
  on the same `Selection`.
- **Warnings:** at least one diagnostic can be clicked to navigate to the
  relevant object or field.
- **Truth:** parsed-only, fallback, unsupported, ignored, and rendered states are
  visibly distinct.
- **Corpus:** the same status vocabulary can summarize more than one scene.
- **Embedder split:** the minimal SDK example remains free of workbench UI
  dependencies.
- **Capture:** the user can preserve a screenshot or textual diagnostic for the
  selected object.

## Implementation Shape

Keep UI state separate from renderer state:

```text
WorkbenchApp
  Selection selection
  Diagnostics diagnostics
  UiSettings ui
  SceneSession scene
  Renderer renderer
```

Panels consume these shared models:

```text
SceneTreePanel(selection, scene, diagnostics)
ViewportOverlay(selection, renderer, ui)
InspectorPanel(selection, scene, renderer, diagnostics)
WarningsPanel(selection, diagnostics)
TimelinePanel(scene, ui)
CorpusPanel(selection, corpus, diagnostics)
```

The immediate-mode UI can stay ImGui, but the data model should not be
ImGui-shaped. That preserves the option to split products or swap UI technology
later.

## Non-Goals For The Base

- Recreating a full authoring suite before the inspection workflow is strong.
- A material/node editor before inspection is strong.
- Project files, asset libraries, or scene packaging.
- Full custom theming.
- Hiding unsupported features to make demos look cleaner.

The base succeeds when it makes x3d-cpp's internal truth visible, navigable, and
trustworthy.

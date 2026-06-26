# Future-Proof Architecture Synthesis — Codegen-Ontology + Registry-Protocol + Behavior-Projection

**Date:** 2026-06-18  **Status:** design synthesis (architecture direction; not yet an approved spec).

**Scope:** the long-horizon shape of the runtime layer — how `runtime/` (hand-written behavior) and `generated_cpp_bindings/` (codegen) interrelate, what to import from USD/Hydra, what to refuse, what to do next.

**Inputs this synthesis draws on:**
- `docs/superpowers/specs/2026-06-07-architecture-validation-and-resequencing.md` (validated architecture: node-as-truth + systems + render-consumer)
- `docs/superpowers/specs/2026-06-13-m3-conformance-design.md` (codegen-from-external-spec as the moat)
- `docs/superpowers/specs/2026-06-14-browser-level-roadmap.md` (the M25 extraction seam)
- The rot pattern empirically visible in `runtime/events/` and `runtime/scene/`
- USD schema-system docs (`openusd.org/release/tut_generating_new_schema.html`, `…/wp_schema_versioning.html`)
- Hydra 2.0 scene-index architecture (`openusd.org/dev/api/_page__hydra__getting__started__guide.html`)
- glTF extension taxonomy + ecosystem-history notes (`khronos.org/gltf`, `novedge.com/blogs/…/design-software-history-gltf-and-khronos`)

---

## 1. Problem framing

### 1.1 Architectural pitfalls in earlier VRML/X3D SDKs, and this repo's posture

Earlier C++ VRML/X3D SDKs took several architectural approaches worth contrasting against this repo's choices. Some are design patterns this repo deliberately avoids; one constraint it still shares. FreeWRL is a live, integrated browser whose design choices reflect that goal; the defunct attempts (OpenVRML, CyberX3D, the ISO/IEC 19777-4 effort) also illustrate the sustainability constraint below.

| Architectural pitfall | This repo's posture |
|---|---|
| **Hand-written node classes rot on every spec bump** | Codegen-from-X3DUOM; ontology is golden-generated, not hand-edited |
| **Browser-coupled** (a full browser pulls in OpenGL, a windowing toolkit, a JS engine, a JDK, fontconfig, etc. — a heavy build) | Header-only `INTERFACE` library; ~zero deps in core; Duktape vendored only for the optional SAI |
| **Conformance evidence is hard to demonstrate without a corpus harness** | Golden-file gate + 17,719-file corpus smoke; ctest over the full binding surface |
| **Single-author bus factor** (OpenVRML lost momentum when Braden McDaniel's employer funding evaporated; CyberX3D was a single-developer effort) | **Still exposed to this.** 260 commits, one human + an "aider" pair-programming bot. Architecture cannot cure this. |
| **Per-node-type behavior rot** (8 separate `*InterpolatorSystem.hpp` files) | **Still exposed to this.** Hand-written `System` per concrete node type in `runtime/events/`. |

### 1.2 What the empirical code review surfaced

Reading the codebase today, the rot pattern is concrete:

- **`runtime/events/`** has 8 near-identical interpolator system files (`ColorInterpolatorSystem.hpp`, `ScalarInterpolatorSystem.hpp`, `CoordinateInterpolatorSystem.hpp`, `CoordinateInterpolator2DSystem.hpp`, `NormalInterpolatorSystem.hpp`, `OrientationInterpolatorSystem.hpp`, `PositionInterpolatorSystem.hpp`, `PositionInterpolator2DSystem.hpp`). Each is ~50 LOC differing only in value type + lerp function. The math is factored into `Interpolation.hpp` (good), but the per-type `System` classes are textbook rot.
- **`runtime/scene/TransformSystem.hpp:71-76`** hard-codes the transform-bearing node list: `t == "Transform" || t == "HAnimHumanoid" || t == "HAnimJoint" || t == "CADPart"`. Add a 5th transform-bearing type in X3D 4.2 = edit this string list.
- **`runtime/scene/TransformSystem.hpp:84-93`** does field reflection by name (`for (f : n->fields()) if (f.x3dName == name)`), ignoring the typed accessors the codegen already emits.
- **`runtime/events/X3DTimeDependentSystem.hpp:47`** uses `dynamic_cast<X3DTimeDependentNode*>` to test interface membership.
- **`runtime/events/TimeSensorBehavior.hpp` AND `TimeSensorSystem.hpp`** — two files for one node (the Behavior is presumably superseded; the duplication is unresolved).

Meanwhile, the codegen layer is in better shape: virtual-base interfaces like `X3DTimeDependentNode`, `X3DSensorNode`, `X3DBindableNode`, `X3DInterpolatorNode`, `X3DGroupingNode` are *already* emitted (`generated_cpp_bindings/X3D*Node.hpp`) and concrete nodes already inherit them. `X3DNodeFactory` (`generated_cpp_bindings/X3DNodeFactory.hpp`) is the registry. **The ontology layer already has the registry-shaped data; it just doesn't expose it as a queryable runtime API.**

### 1.3 The validated architecture is already in place

Per `2026-06-07-architecture-validation-and-resequencing.md`:

> Node-as-truth + systems-update-in-place + renderer-as-consumer. The codegen moat is de-risked at the mechanism level but is still the project's own bet at the "chase the external ISO spec" level.

The architecture doc also commits to "ECS-flavored batching only at hot paths, NOT a full ECS" — meaning the consensus in shipping runtimes (USD, Castle, Godot, three.js) is *scene-graph truth with data-oriented projections on the hot paths*, not pure ECS. That's the right shape; the question is what concretely to project, and through what protocol.

### 1.4 Corrections from the 2026-06-18 grounding pass

1. **Interface count & shape.** The UOM has ~74 interface types (68 abstract
   node types + 6 mixin object types), not "~30", and forms a diamond DAG (42
   concrete nodes implement >1 interface; MovieTexture has 3 bases over a
   multi-level diamond). The membership table MUST store the FULL TRANSITIVE
   CLOSURE of a node's ancestors, not its direct bases.
2. **Registry is emitted CENTRALLY, not as per-node static arrays.** One
   generated X3DInterfaceRegistry.hpp/.cpp pair (mirroring X3DNodeFactory),
   keyed by node-type name. This keeps all ~200 existing golden headers
   byte-identical. (The doc's `static constexpr InterfaceId interfaces_[]`
   per-node sketch is superseded.)
3. **Step 1 effort is ~1–2 days, not "one day".** No NodeTypeId enum exists
   today; node identity is string-keyed via X3DNodeFactory. The registry is
   still "free" of new source-of-truth: parser.py already parses
   base_type/additional_base_types and resolve_inheritance_chain() already
   computes the transitive closure.
4. **Versioning bet — add the third evolution mode.** Field evolution is purely
   additive across 3.3→4.0→4.1 (zero changed defaults/types/removals), BUT the
   spec re-parents existing concrete nodes by inserting new abstract interfaces
   (e.g. Material→X3DOneSidedMaterialNode). So the rule is: "version bumps may
   add bases to existing classes; consumers must query capability via the
   registry/InterfaceId, NEVER via dynamic_cast to a concrete C++ base." The
   registry is exactly what makes re-parenting non-breaking.
5. **USD rationale fixes + visitor.** Refusing versioned classes is right, but
   the reason is "USD versions classes because it's a data-interchange format
   needing decades-long multi-version coexistence; a compiled runtime owns its
   definitions" — NOT "breaks SFINAE/ABI". "O(1) registry" → "constant-ish,
   table-backed". The registry replaces type *identity* checks; the *visitor*
   pattern remains the right tool for graph *traversal* — the registry makes
   the visitor open/extensible (fixing OSG's closed-apply() expression problem),
   it does not replace traversal.

---

## 2. What to borrow from USD — and what to refuse

### 2.1 Steal

| USD pattern | What it solves | How it maps here |
|---|---|---|
| **Schema registry as runtime service** (`UsdSchemaRegistry` knows every schema's family/version/type/identifier) | Replace `dynamic_cast` and string-list type detection with O(1) runtime queries | New `runtime/registry/InterfaceRegistry.hpp` + `runtime/registry/ComponentRegistry.hpp`; codegen-emitted |
| **`UsdPrim` is the truth, schemas are facades** | Single source of truth; typed views over a generic node | **Adapt:** the typed class IS the data (C++ wants this), but the registry above it is the meta-layer that behaviors query |
| **`HdSceneIndex` chain** (originating → flatten → merge → render delegate) | Composable renderer projection; one filter per job | Reframe `SceneExtractor` as a scene-index filter chain |
| **`UsdImagingPrimAdapter` plugin model** (`plugInfo.json` + `TF_REGISTRY_FUNCTION`) | Extension without recompiling | In-process `std::function` registration (no `plugInfo.json` overhead for embedder SDK) |
| **`HdOverlayContainerDataSource`** (lazy composition) | Compose data lazily; materialize on demand | Optional later optimization for filter chains; not load-bearing |
| **`HdDataSourceLocator`** for dirty propagation | Granular invalidation beyond dirty bits | Adopt for the renderer-side change tracker (`DirtyTracker` already exists) |
| **Plug-in prim adapters per prim type / per API schema** | "I handle type X" registration | Behaviors, validators, extractors all register against `NodeTypeId` and `InterfaceId` |

### 2.2 Leave — the parts that don't fit C++

| USD pattern | Why it's wrong for X3D C++ | What we do instead |
|---|---|---|
| **Schema-as-view, prim-as-truth** (`UsdGeomMesh(UsdPrim)` facade with attribute lookup per access) | Indirection on every field access. Fine for Python, terrible for C++ hot paths. | Typed class owns its data (current pattern). The registry is the protocol, not a replacement for the storage. |
| **Versioned classes** (`Sphere_0`, `Sphere_1`, `Sphere_2`) | Breaks C++ templates, SFINAE, forward decls, ABI stability. Versioning is fine for dynamic languages; it's a hazard for compiled languages. | **glTF-style:** add new node classes for new spec versions; extend existing classes with new fields (defaults match spec); never break C++ class identity. Unknown types fall through with a diagnostic (existing skip-with-record pattern). |
| **`TfToken`/`TfType` machinery** for interned strings + runtime type info | Heavy machinery for plugin discovery + Python interop. C++ has static type info. | Codegen-emitted `NodeTypeId` enum + `InterfaceId` enum; O(1) compile-time-resolved IDs. |
| **Per-schema auto-apply** (e.g., `PxrMeshLightAPI` auto-applies to `MeshLightAPI`) | Cascading versioning complexity (one version bump propagates to dozens of dependents). | X3D interfaces are concrete, not auto-applied; the registry captures the membership statically. |

### 2.3 What glTF teaches

glTF succeeded because of *restraint*, not ambition:

1. **Minimal core + extension taxonomy (KHR / EXT / VENDOR).** Core never grows to accommodate one industry's needs. X3D's equivalent: the scene-index filter chain IS the extension point. New render/projection/extraction capabilities are filters, not ontology changes.
2. **Runtime-centric, not author-centric.** Designed for the *consumer*, not the DCC tool. The repo's "headless, embedder-pulls" stance is the same discipline.
3. **One job.** glTF is for *asset delivery*. X3D tries to be VRML successor + browser tech + ISO standard + web 3D. **The repo should pick one job: headless X3D runtime for embedded consumption.** That's the empty niche — USD owns VFX pipelines, glTF owns web asset delivery, the repo can own "X3D spec, embedded, headless."

---

## 3. The proposed architecture

Two layers, both codegen-driven, with USD's *separation* as the spine but C++-native *storage*:

```
┌─────────────────────────────────────────────────────────────┐
│  Layer 1 — Ontology + Registry (codegen, golden, no behavior) │
│  generated_cpp_bindings/                                     │
│   • Typed classes that OWN data (current pattern, fast)      │
│   • Virtual-base interfaces (already emitted)                │
│   • NEW: per-node `static constexpr InterfaceId interfaces[]`│
│   • NEW: per-node `static constexpr ComponentId components[]` │
│   • NEW: InterfaceRegistry (NodeTypeId ↔ InterfaceId)        │
│   • NEW: ComponentRegistry (NodeTypeId ↔ ComponentId)        │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ protocol: registries
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  Layer 2 — Behavior + Projection (hand-written, queries     │
│            registries, iterates stores)                      │
│  runtime/                                                     │
│   • Systems register against InterfaceId                     │
│   • Systems iterate ComponentStore<T>, not the typed tree    │
│   • SceneExtractor → scene-index filter chain                │
│   • Filters register by NodeTypeId (plugin-style)            │
│   • Embedder-facing SDK API stays typed (no break)           │
└─────────────────────────────────────────────────────────────┘
```

**The design rule, pinned as the spec:**

> *The scene graph is the truth. Registries are the protocol. Component stores are the projection. Scene-index filters are the extension point. Behaviors are typed interfaces that query registries and iterate stores.*

### 3.1 The registries — what they are

```cpp
// runtime/registry/InterfaceId.hpp — codegen-emitted enum
enum class InterfaceId : uint16_t {
    X3DChildNode, X3DBindableNode, X3DTimeDependentNode,
    X3DSensorNode, X3DDragSensorNode, X3DEnvironmentalSensorNode,
    X3DGroupingNode, X3DInterpolatorNode, X3DFollowerNode,
    X3DChaserNode, X3DDamperNode, /* ... ~30 interfaces total */
};

// runtime/registry/InterfaceRegistry.hpp — codegen-emitted
class InterfaceRegistry {
    // NodeTypeId → set of InterfaceIds (cheap: static table)
    static std::span<const InterfaceId> interfacesOf(NodeTypeId t);
    // InterfaceId → set of NodeTypeIds (built once at startup)
    static const std::vector<NodeTypeId>& nodesImplementing(InterfaceId i);
    // Cheap query (no dynamic_cast)
    static bool nodeImplements(const X3DNode* n, InterfaceId i);
};

// runtime/registry/ComponentId.hpp + ComponentRegistry.hpp — same shape
// + runtime/ecs/ComponentStore<T> — SoA storage keyed by NodeId
```

Each concrete node class gets:

```cpp
// emitted by codegen from existing UOM data
class TimeSensor : public virtual X3DTimeDependentNode,
                   public virtual X3DSensorNode {
    // ...
    static constexpr std::array<InterfaceId, 2> interfaces_ = {
        InterfaceId::X3DTimeDependentNode,
        InterfaceId::X3DSensorNode,
    };
    static constexpr std::array<ComponentId, 1> components_ = {
        ComponentId::TimeDepState,
    };
};
```

The codegen template change is small: `virtual_bases` already exists and is emitted; the `interfaces_` table is a 1-line emission from the same data.

### 3.2 The behavior-side change

Systems today: `dynamic_cast<X3DTimeDependentNode*>(n)`. After:

```cpp
class X3DTimeDependentSystem : public System {
    ComponentStore<TimeDepState> states_;   // SoA, replaces unordered_map
    void attach(X3DNode* n, ctx& c) override {
        if (!InterfaceRegistry::nodeImplements(n, InterfaceId::X3DTimeDependentNode))
            return;
        states_.insert(n, TimeDepState{});
    }
    void update(double now, ctx& c) override {
        for (auto& [n, st] : states_.all()) advance(n, st, now, c);
    }
};
```

Same shape, just: (a) interface check is registry lookup, not `dynamic_cast`; (b) state is SoA, not hash-map.

### 3.3 The interpolator pilot (Step 2 below) is the proof

The 8 per-type interpolator files collapse to:

```cpp
// runtime/events/InterpolatorSystem.hpp — ONE file
template <typename T, typename LerpFn>
class InterpolatorSystem : public System {
    using NodeT = /* concrete class */;
    void attach(X3DNode* n, ctx&) override {
        if (!InterfaceRegistry::nodeImplements(n, InterfaceId::X3DInterpolatorNode))
            return;
        // ... wire set_fraction → value_changed via typed accessors
    }
    void handleFraction(X3DNode* n, const SFFloat& f, ctx& c) override {
        // call LerpFn with the typed accessors of this specialization
    }
};

// Registration (one line per concrete type):
ctx.addSystem(std::make_unique<InterpolatorSystem<SFColor, lerpColorHsv>>("Color"));
ctx.addSystem(std::make_unique<InterpolatorSystem<SFFloat, lerpf>>("Scalar"));
// ... 6 more lines
```

350 LOC of rot → 1 templated class + 8 registration lines.

### 3.4 Scene-index filter chain (the renderer seam)

Today: monolithic `SceneExtractor`. After:

```
[SceneIndex: typed-tree source]   ← reads X3DScene + execution context
        ↓
[SceneIndex: transform flatten]   ← composes world transforms, dirty-aware
        ↓
[SceneIndex: light scope filter]  ← binds active Viewpoint/Lights
        ↓
[SceneIndex: material projection] ← resolves Appearance/Material chains
        ↓
[SceneIndex: mesh build]          ← primitives → MeshData
        ↓
[SceneIndex: render item terminal]← what the renderer pulls
        ↓
    Renderer consumer
```

Each filter is independently testable, independently replaceable. The embedder adds filters without touching the source — this IS the extension point, USD-style.

---

## 4. Concrete work plan (ordered, each step independently shippable)

Each step has the 17,719-file corpus smoke + ctest as the regression gate. None of them touch the embedder-facing SDK API.

```
[1] Codegen: emit `static constexpr InterfaceId interfaces[]` per concrete node
    + emit InterfaceRegistry (NodeTypeId ↔ InterfaceId bidirectional lookup)
    + emit `bool nodeImplements(X3DNode*, InterfaceId)` helper
    →  Kills the `dynamic_cast` in X3DTimeDependentSystem, the hard-coded
       type-name list in TransformSystem::isTransform, and similar
       patterns elsewhere. One-day work. Pure additive codegen change.

[2] Refactor 8 interpolator System files → 1 templated
    InterpolatorSystem<T, LerpFn> + 8 one-line registrations
    →  Removes ~350 LOC rot, proves registry-based dispatch works.
       Corpus smoke must still pass.

[3] Add `runtime/ecs/ComponentStore<T>` (~150 LOC, no deps)
    + first user: replace X3DTimeDependentSystem::state_ map with store
    + measure on the corpus smoke. KEEP the map if SoA doesn't measurably
       win at typical scene sizes (10–1000 nodes). ECS-flavored only on
       profiling evidence (this is the rule already in
       2026-06-07-architecture-validation-and-resequencing.md).

[4] Reframe SceneExtractor as scene-index filter chain
    (source → flatten → projection → terminal). Each filter is its own
    testable unit; existing tests map to filter tests.

[5] Document the rule in a `docs/superpowers/specs/` design doc
    ("scene graph truth / registries protocol / stores projection /
    filters extension"). Make the boundary visible to contributors.

[6] Pilot custom-code preservation in codegen template
    (`// --(BEGIN CUSTOM CODE)--` block in generated headers, USD-style).
    Try regenerating after a hand-edit to one node class; verify the
    block is preserved. Proves the long-term hand-override story.

[7] Version-resilience plan, decided but NOT implemented yet:
    X3D 4.2 adds → codegen emits new node classes + extends existing
    classes with new fields (defaults match spec) + new InterfaceRegistry
    entries. Never break C++ class identity. Unknown types: existing
    "skip + record diagnostic" path. Pin in the spec so this isn't
    revisited as a versioning-model question.
```

Steps 1–6 are independent and can be done in roughly that order. Step 7 is a decision only — a paragraph in the design spec, not an implementation.

---

## 5. Decisions to pin (to prevent re-litigation)

These are the design choices that future contributors (or future-Opus, or future-self) might want to revisit. Each has a *why* that's load-bearing.

| Decision | Why it has to stay this way |
|---|---|
| **Typed classes own their data** (no schema-as-view facade) | C++ static typing + direct member access for hot paths. Indirection on every field access is the wrong tradeoff. |
| **No C++ class versioning** (no `Sphere_0` / `Sphere_1`) | Breaks templates, SFINAE, ABI. Extend existing classes with new fields + defaults; add new classes for new nodes. |
| **Hand-override preservation** (`// --(BEGIN CUSTOM CODE)--` blocks) | Same approach USD's `usdGenSchema` uses; lets embedders add behavior to generated nodes without forking codegen. |
| **Registry is the protocol, not a replacement for the typed tree** | Behaviors query the registry; the typed tree remains the storage and the API surface. Don't try to flatten the scene into entities. |
| **Component stores only on hot paths with profiling evidence** | The rule from `2026-06-07-architecture-validation-and-resequencing.md`. Don't ECS-ify speculatively. |
| **Scene-index filters as the extension point, not class inheritance** | Embedders add projection/extraction without recompiling core. Same shape as `UsdImagingPrimAdapter` registration. |
| **Headless, embedder-pulls stance is non-negotiable** | Browser coupling is the prior failure mode. Don't add OpenGL, GTK, JS engine, fontconfig to the core. Duktape stays vendored and *optional*. |
| **The "conformance moat" is still being validated** | Per `2026-06-13-m3-versioning-design.md`: fail-loud-on-restructure machinery is DESIGNED-UNBUILT. The 17k corpus smoke proves parse+extract+tick doesn't crash, not semantic correctness. M3 is the bet, not a fait accompli. |

---

## 6. Caveats (not solvable by architecture)

1. **Sustainability is the binding constraint.** USD succeeded because Pixar + Disney + Apple + Adobe + NVIDIA funded it for ~20 years (AOUSD alliance, $10K/yr member dues). glTF succeeded because Khronos had 25+ member companies. X3D's steward is the Web3D Consortium, a smaller industry body than the AOUSD and Khronos alliances behind USD and glTF. Architecture can be right and still fail without ecosystem gravity. **The only path to real adoption is finding one embedder (a game engine, a CAD tool, a web framework) that wants X3D support and doesn't want to ship a browser.** Architecture alone won't get there.

2. **The bus factor is unchanged.** 260 commits, one human + an "aider" pair-programming bot. The codegen bet doesn't fix this; nothing fixes it except more humans. The plan above is buildable by one person over weeks-to-months; it does not require a team.

3. **The conformance moat is aspirational.** Step 1 of the M3 plan (`2026-06-13-m3-versioning-design.md`) is to prove the parser is authored independently of the generated bindings. Without that attestation, the "codegen-from-spec → conformance" claim is a hypothesis, not an evidence-backed moat. Don't oversell it in the README until M3 lands.

---

## 7. Open questions (deferred)

These are design questions the repo may want to revisit, but each has a known *right default* to use until the question is reopened:

| Open question | Default for now | When to revisit |
|---|---|---|
| Pull vs push at the renderer seam | Pull (after `tick(now)` returns, the consumer pulls the changed set) | When a consumer needs change *notifications* rather than change *queries* |
| Plugin discovery format | In-process `std::function` registration | When out-of-process renderer plugins are needed (e.g., distributed build) |
| Custom-code preservation in generated headers | USD-style `// --(BEGIN CUSTOM CODE)--` markers | When the codegen template grows enough that hand-edits need a real merge story |
| Schema versioning for X3D 4.2+ | Extend existing classes + add new ones; never version | If a future spec change requires semantic break (different defaults, removed fields, type change) |
| Whether to ship a `codeless schema` mode (codegen produces only the runtime registry, no C++ class) | No — the typed class is the embedder-facing API | If embedders need to define custom node types without recompiling the SDK |
| Whether `InterfaceRegistry` should be flat-lookup or hand-coded switch | Codegen-emitted flat tables | After measuring on a representative scene |

---

## 8. File-by-file impact (the minimal first pass)

For Step 1 (the codegen + registry change), the concrete file impact:

**Generated (codegen-emitted, golden):**
- `generated_cpp_bindings/X3DNode.hpp` — no change
- `generated_cpp_bindings/X3D*Interface.hpp` (already exists) — add `static constexpr InterfaceId interfaces[]` per interface? *Probably not needed* — only concrete nodes need the table.
- `generated_cpp_bindings/*Node.hpp` (per concrete node, ~200 files) — add the `interfaces_` static array from `virtual_bases`
- **NEW generated**: `generated_cpp_bindings/InterfaceRegistry.hpp` + `InterfaceRegistry.cpp` (codegen-emitted registry)
- `generated_cpp_bindings/X3DNodeFactory.hpp` — no change (separate concern)

**Hand-written (in `runtime/`):**
- `runtime/registry/InterfaceId.hpp` — codegen-emitted enum, but lives in `runtime/` so the registry can `#include` it
- `runtime/registry/InterfaceRegistry.hpp` — codegen-emitted, but in `runtime/` for include-path consistency
- `runtime/scene/TransformSystem.hpp:71-76` — replace hard-coded list with `InterfaceRegistry::nodeImplements(n, InterfaceId::X3DGroupingNode)` plus a field-presence check
- `runtime/events/X3DTimeDependentSystem.hpp:47` — replace `dynamic_cast<X3DTimeDependentNode*>` with `InterfaceRegistry::nodeImplements(n, InterfaceId::X3DTimeDependentNode)`
- Similar small substitutions elsewhere (`BindingSystem.hpp`, `ViewDependentSystem.hpp`)

For Step 2 (interpolator pilot):
- `runtime/events/ColorInterpolatorSystem.hpp` + 7 siblings → deleted
- **NEW**: `runtime/events/InterpolatorSystem.hpp` (~120 LOC templated class)
- **NEW**: `runtime/events/InterpolatorRegistration.cpp` (~20 LOC, 8 registration lines)

For Step 3 (`ComponentStore<T>`):
- **NEW**: `runtime/ecs/ComponentStore.hpp` (~150 LOC, no deps)
- `runtime/events/X3DTimeDependentSystem.hpp:53` — replace `std::unordered_map<X3DTimeDependentNode*, State>` with `ComponentStore<TimeDepState>`

---

## 9. References (file:line and URL)

**Codebase (current state):**
- `generated_cpp_bindings/X3DNode.hpp` — codegen-emitted base class
- `generated_cpp_bindings/X3DNodeFactory.hpp` — codegen-emitted registry of node creators
- `generated_cpp_bindings/TimeSensor.hpp:27` — virtual inheritance from spec interfaces
- `generated_cpp_bindings/ColorInterpolator.hpp:25` — same
- `generated_cpp_bindings/X3DTimeDependentNode.hpp` — interface base class, codegen-emitted
- `runtime/events/X3DSystem.hpp` — the `System` interface (already interface-style)
- `runtime/events/X3DTimeDependentSystem.hpp` — bulk-update base class for time-dependent behaviors
- `runtime/events/X3DTimeDependentSystem.hpp:47` — `dynamic_cast` rot target
- `runtime/events/ColorInterpolatorSystem.hpp` + 7 siblings — rot pattern
- `runtime/events/Interpolation.hpp` — shared lerp math (good factoring)
- `runtime/events/TimeSensorSystem.hpp` — the right shape: single class deriving from `X3DTimeDependentSystem`, overriding hooks
- `runtime/scene/TransformSystem.hpp:71-76` — hard-coded type-name list rot
- `runtime/scene/TransformSystem.hpp:84-93` — field-name reflection walk rot
- `src/x3d_cpp_gen/templates/class_template.hpp.jinja` — codegen template with `virtual_bases` support

**Prior design docs (for context):**
- `docs/superpowers/specs/2026-06-07-architecture-validation-and-resequencing.md` — the validated architecture this builds on
- `docs/superpowers/specs/2026-06-13-m3-conformance-design.md` — the conformance moat as the project's own bet
- `docs/superpowers/specs/2026-06-13-m3-versioning-design.md` — fail-loud-on-restructure machinery is DESIGNED-UNBUILT
- `docs/superpowers/specs/2026-06-14-browser-level-roadmap.md` — the M25 extraction seam
- `docs/superpowers/specs/2026-06-16-v1-closure-roadmap-design.md` — v1 capability matrix
- `docs/superpowers/BACKLOG.md` — the live backlog of known gaps

**External:**
- USD schema generation tutorial: `openusd.org/release/tut_generating_new_schema.html`
- USD schema versioning whitepaper: `openusd.org/release/wp_schema_versioning.html`
- Hydra 2.0 scene-index architecture: `openusd.org/dev/api/_page__hydra__getting__started__guide.html`
- AOUSD alliance: `aousd.org/`
- glTF ecosystem history: `novedge.com/blogs/design-news/design-software-history-gltf-and-khronos-…`

---

## 10. One-line summary

**Codegen emits the typed ontology + an `InterfaceRegistry`/`ComponentRegistry`; hand-written behaviors query the registry and iterate `ComponentStore<T>` projections; the renderer seam becomes a scene-index filter chain. Adopt USD's separation, refuse USD's versioning; adopt glTF's restraint, name one job. First concrete step: emit the registry from the codegen, replace `dynamic_cast` with registry lookup, then collapse the 8 interpolator files into one templated class.**

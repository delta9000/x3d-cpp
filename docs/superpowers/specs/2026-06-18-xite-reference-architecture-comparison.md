# X_ITE as a Reference Architecture — What x3d-cpp-gen Converges With, Diverges From, and Should Borrow

**Date:** 2026-06-18
**Status:** Reference comparison. Validates the synthesis in `2026-06-18-future-proof-architecture-synthesis.md`. Identifies concrete adoption steps.

## Why X_ITE?

Of all prior X3D implementations, X_ITE is the one that has survived and grown over 15 years (17,731 commits, single primary author `create3000`, last commit 2026-06-16). It is the de-facto X3D reference. Its architecture is the shape that long-lived X3D runtimes converge to. Studying it tells us what the shape *is*.

## X_ITE's Architecture at a Glance

Top-level `src/x_ite/` directory split by concern:

| Subdir | Role |
|---|---|
| `Base/` | `X3DObject`, `X3DConstants`, `X3DNames`, `BrowserSupport`, `X3DArray`, util mixins |
| `Fields/` | Field type system (`SFNode`, `MFNode`, `SFVec3f`, etc.) |
| `InputOutput/` | I/O streams / serialization primitives |
| `Components/` | **Abstract data layer** — pure spec-defined node classes, one directory per X3D component (`Core/`, `Grouping/`, `Interpolation/`, `Shape/`, …). Example: `Components/Interpolation/ColorInterpolator.js`, `Components/Interpolation/PositionInterpolator.js`, … |
| `Browser/` | **Runtime layer** — same shape as `Components/` but each spec node has a browser-aware version that knows about scene graph, rendering, sensors, etc. Example: `Browser/Interpolation/ColorInterpolator.js` is paired with the abstract `Components/Interpolation/ColorInterpolator.js`. |
| `Execution/` | `X3DExecutionContext`, `X3DScene`, `X3DWorld`, `BindableList`, `BindableStack`, `ExportedNodesArray`, `ImportedNodesArray`, `NamedNodesArray` |
| `Routing/` | `X3DRoute`, `RouteArray`, `X3DRoutingContext` |
| `Prototype/` | `Prototype`, `ExternProto`, `ProtoDeclaration`, `ProtoInstance` |
| `Parser/` | `XMLParser`, `JSONParser`, `VRMLParser`, `GLB2Parser`, `GLTF2Parser`, `OBJParser`, `STLAParser`, `STLBParser`, `PLYAParser`, `PLYBParser`, `SVGParser`, `X3DOptimizer`, `X3DParser`, `GoldenGate` |
| `Rendering/` | `X3DRenderObject`, `RenderPass`, `TraverseType`, `VertexArray`, `TextureBuffer`, `PointingBuffer`, `MultiSampleFramebuffer`, `DependentRenderer` |

The `Components/` (abstract) vs `Browser/` (runtime) split is the central pattern. Every spec node exists in both: `Components/<X>/Foo.js` defines the data + spec-defined behaviors (e.g. interpolator math); `Browser/<X>/Foo.js` extends it with browser-awareness (scene graph interaction, event handling, render dispatch).

## X3DRenderObject — the Per-Frame State

`Rendering/X3DRenderObject.js` is X_ITE's most distinctive abstraction. It is **not** a per-item projection (x3d-cpp-gen's `RenderItem`). It is a **per-frame state container** that the renderer builds up during traversal, then consumes to issue draw calls.

Contents of an `X3DRenderObject`:

- **Matrix stacks**: `projectionMatrix`, `modelViewMatrix`, `viewMatrix`, `cameraSpaceMatrix`, current transform state
- **Lights**: `globalLights[]`, `localLights[]`, with shadow support (`shadowBuffer`, `shadowOffset`, `shadowColor`)
- **Sensors**: `Sensors[]` — list of currently-active pointer sensors
- **ViewVolumes**: `viewports[]` — stack of clipped view regions
- **Render passes**: `RenderPass.DEPTH`, `RenderPass.SHADOW`, `RenderPass.VOLUME_SCATTER`, `RenderPass.TRANSMISSION`, `RenderPass.RENDER`
- **Shape lists**: `opaqueObjects[]`, `transparentObjects[]` — built during traversal, drained by the renderer
- **Render contexts**: per-shape `{ modelViewMatrix, viewport, hAnimNode, appearance }` — a small bundle a renderer needs to draw a shape

Traversal entry points on `X3DRenderObject`:
- `bind` — set up the per-frame state, push view
- `shadow` — shadow pass
- `draw` — generic draw
- `display` — opaque + transparent draw
- `sensor` — sensor detection
- `set_camera` — set the active viewpoint

`TraverseType` enumerates the traversal reason: `POINTER`, `COLLISION`, `DEPTH`, `SHADOW`, `DISPLAY`, `FRUSTUM_CULLING`. Each traversal collects a different set of shapes/sensors.

X_ITE's `X3DRenderObject.draw()` calls `gl.*` directly. **It is tightly coupled to WebGL.** The renderer-agnostic surface is the per-frame state object, not a draw call API.

## Multi-Parser Strategy

`Parser/` is unusually rich: 7 distinct parsers (`XMLParser`, `JSONParser`, `VRMLParser`, `GLB2Parser`, `GLTF2Parser`, `OBJParser`, `STLParser`, `PLYParser`, `SVGParser`). The non-X3D parsers (glTF, OBJ, STL, PLY) **convert to X3D internally** — X_ITE doesn't load OBJ as OBJ; it parses it and constructs an equivalent X3D scene (Grouping + Shape + IndexedFaceSet).

This is a critical adoption pattern. Anyone with a 3D file is a potential user. The cost of writing an OBJ parser is small (one weekend); the value of being able to load "any 3D file" is enormous.

## What x3d-cpp-gen Already Converges On

The architecture x3d-cpp-gen has arrived at independently is the same shape X_ITE uses:

| X_ITE concept | x3d-cpp-gen equivalent | Same shape? |
|---|---|---|
| `Components/` (abstract data) | `generated_cpp_bindings/` (codegen-emitted typed classes) | ✓ |
| `Browser/` (runtime-aware versions) | `runtime/scene/`, `runtime/events/` (System classes) | ✓ |
| `Execution/X3DExecutionContext` | `runtime/events/X3DExecutionContext` | ✓ |
| `Routing/X3DRoutingContext` | `runtime/events/X3DEventCascade` | ✓ |
| `Prototype/Prototype` | `runtime/X3DProto*.hpp` | ✓ |
| `Parser/XMLParser`, `JSONParser` | `runtime/codecs/XmlReader`, `JsonWriter`, `VrmlWriter` | partial |
| `Rendering/X3DRenderObject` | `runtime/extract/SceneExtractor` (per-tick not per-frame) | similar role, different cadence |
| `Base/X3DNames` | `X3DNodeFactory` registry | ✓ |
| `Base/X3DArray` | `runtime/parse/` + std lib | ✓ |

The shape is correct. The differences are in substrate (C++ vs JS) and boundary placement (X_ITE: WebGL *inside*; x3d-cpp-gen: any renderer *outside*).

## Where x3d-cpp-gen Should NOT Copy X_ITE

1. **WebGL coupling.** `X3DRenderObject.draw()` calls `gl.*` directly. This is the exact anti-pattern x3d-cpp-gen is built to avoid. Keep the per-frame state object, but the renderer consumes it; x3d-cpp-gen never issues draw calls.

2. **Browser coupling.** X_ITE is a browser plugin: DOM, CSS, HTML integration, HTML overlays, CSS skinning. x3d-cpp-gen is headless. The embedder brings the UI; the runtime never does.

3. **Rebuild-every-frame.** X_ITE rebuilds its opaque/transparent lists every frame from scratch. x3d-cpp-gen's `DirtyTracker` + `X3DBindableStack` + incremental extract is a strict improvement for performance. Don't regress this.

4. **Dual class hierarchy per node.** X_ITE has `Components/Foo.js` *and* `Browser/Foo.js` for every spec node. The component defines pure spec semantics; the browser version is the runtime-aware version. This is two files per node for 280+ nodes — the rot source. x3d-cpp-gen avoids this by having **one** generated class per node and adding behavior via System classes that query spec interfaces. Keep this.

5. **`X3DNames` as runtime table.** X_ITE has a giant string→node lookup table. x3d-cpp-gen already has this in `X3DNodeFactory`. No change.

## What x3d-cpp-gen Should Borrow

### 1. Multi-Parser Strategy — adopt OBJ, STL, PLY, glTF as import codecs

X_ITE proves that supporting foreign formats is a small cost (a single parser per format) for large adoption benefit. The embedder's question becomes "does your library load my 3D file?" not "does your library load my X3D file?"

**Concrete proposal**: Add `runtime/codecs/{ObjReader,StlReader,PlyReader,GltfReader}.hpp` in v1.5. Each converts the foreign format to an X3D scene graph in-memory. None of them belong in core — they live in the same INTERFACE library as XMLReader today.

**Status check**: `runtime/codecs/` already exists for X3D's own formats. Adding OBJ/STL/PLY follows the same pattern. glTF is harder (PBR materials, glTF extensions) and is post-v1; OBJ/STL/PLY are easy (geometry-only) and are v1.5 candidates.

### 2. TraverseType — extend the SceneExtractor with pass-aware filtering

X_ITE's `TraverseType` makes the renderer contract explicit: each traversal pass collects different things. x3d-cpp-gen's `SceneExtractor::delta()` currently has a single implicit pass.

**Concrete proposal**: Extend `SceneExtractor` with pass filters: `delta(RenderPass::DEPTH)`, `delta(RenderPass::SHADOW)`, `delta(RenderPass::DISPLAY)`. Same per-tick output, but tagged with pass. The renderer can request the pass it needs. Picker requests `RenderPass::POINTER`.

**Status check**: This is a small extension to `runtime/extract/SceneExtractor.hpp`. No new dep. Worth doing in M25.

### 3. Multi-Pass Render State — make the per-frame renderer state an object

X_ITE's `X3DRenderObject` is a powerful pattern: the runtime hands the renderer a single state object containing everything needed to draw. x3d-cpp-gen's `SceneExtractor` already plays this role but isn't named/exposed as such.

**Concrete proposal**: Rename `SceneExtractor::delta()` output to a `RenderFrame` struct (or class). Document it as the contract: "the runtime hands the renderer a `RenderFrame` per tick; the renderer renders it." This is the surface every embedder implements against.

**Status check**: Pure refactor. No behavior change. The shape is already there.

### 4. Routing Context — already done

X_ITE has `X3DRoutingContext`; x3d-cpp-gen has `X3DEventCascade` + `X3DFieldAddress`. Same role, different name. No action.

### 5. Prototype — already done

X_ITE has `Prototype/` as its own module; x3d-cpp-gen has `runtime/X3DProto*.hpp`. Same role. No action.

## Concrete Adoption Plan

### v1.5 candidates (small, high-impact)
- Add `runtime/codecs/{ObjReader,StlReader,PlyReader}.hpp` — three codecs, one weekend, large adoption lift
- Extend `SceneExtractor::delta()` with `RenderPass` parameter (refactor + small extension)
- Rename `SceneExtractor::delta()` output to `RenderFrame` (pure refactor, makes the contract explicit)

### v2 candidates (post-v1)
- glTF import (PBR materials + glTF extension conformance is non-trivial; needs a separate conformance effort)
- SVG import (only useful if x3d-cpp-gen grows HUD/overlay support)

### v3+ candidates
- Multi-pass render state object (only if a renderer consumer needs it — likely not in v1 because no embedder has asked)

## One Concrete Sub-Adoption: OBJ/STL/PLY as v1.5 Codecs

These three are easy because they're geometry-only:
- **OBJ**: vertices + faces + normals + UVs. No materials, no animations. ~150 LOC parser.
- **STL ASCII + binary**: triangles. ~50 LOC parser.
- **PLY ASCII + binary**: vertices + faces + attributes. ~100 LOC parser.

All three convert to: `Grouping { children: [Shape { geometry: IndexedFaceSet, appearance: Appearance { material: Material { diffuseColor: gray } } }] }`.

This is a **one-weekend adoption win**. Embedders get "load any 3D file." Project gets a much larger user surface. Aligns with the "pick one job: headless X3D runtime for embedded consumption" decision — and that job now includes "loads OBJ/STL/PLY too."

## Conclusion

The architectures converge because the problem converges. x3d-cpp-gen has independently arrived at the same shape as X_ITE: spec-component data layer + runtime behavior layer + execution context + routing + render-state object + multi-parser strategy. The differences are the deliberate ones (headless, renderer-agnostic, dirty-tracked, single-class-per-node) and they're all advantages.

X_ITE is validation, not a model to copy verbatim. The "convergence" reading is the most important finding: **x3d-cpp-gen's architecture is the right shape for a long-lived X3D runtime**, because the long-lived reference runtime (X_ITE) has converged to it independently.

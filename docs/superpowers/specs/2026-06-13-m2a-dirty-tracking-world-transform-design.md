# M2a — Dirty-Tracking Layer + World-Transform Propagation

**Date:** 2026-06-13
**Branch:** `modernize-x3d-spec`
**Status:** design approved, pre-implementation
**Milestone:** M2a (first unit of the M2 scene-graph runtime). See the authoritative
ordering in `2026-06-07-architecture-validation-and-resequencing.md` §3 Step 2.

## Context

M1 (behavior/event runtime + PROTO expansion) is substantively complete. M2a is the
**connective tissue** between the M1 event cascade and the rest of M2 (bounds,
binding stacks, picking) and the M2.5 extraction seam: a per-node dirty-tracking
layer fed by the cascade, plus world-transform propagation down the Transform
hierarchy. The architecture doc validated this against USD/Hydra's
`HdChangeTracker` + dirty-list `SyncAll` and VRVis lazy-incremental scene-graph
work — **no architecture change**, this is the named foundation.

**The dirty-set quantum is spec-defined** (ISO/IEC 19775-1): all events in a cascade
share one timestamp, and a node emits at most one event per field per timestamp
(the guard that breaks ROUTE loops). So the per-tick dirty-set is exactly "every
field touched during this timestamp's cascade" — which `EventCascade` already owns.

**Runtime-only, golden byte-identical.** World transforms and dirty state live in
side tables keyed by `const X3DNode*` (the same discipline as `Scene.expandedSources`
/ `Scene.protoRedirects`). No generated node is touched; the golden header tree must
stay byte-identical (current golden sha256
`7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`).

## Existing integration surface (verified)

- `runtime/events/X3DEventCascade.hpp` — `EventCascade::deliver(addr, value)` (the
  `private static` method, ~line 87) is the single chokepoint where every routed
  field write is applied via `info.set(*node, value)`. This is where the dirty feed
  hooks in. **Note:** `deliver` is currently `static`; it becomes an instance method
  (or gains access to an observer member) so it can notify.
- `runtime/events/X3DExecutionContext.hpp` — `tick(now)` runs `systems_[*].update`
  then `cascade_.process()`. M2a adds a post-cascade propagation step here. The
  context owns the cascade and is the natural owner of the `DirtyTracker` +
  `TransformSystem`.
- `runtime/events/Interpolation.hpp` — provides `Quat` and
  `quatFromRotation(const SFRotation&)`; the mat4 rotation block builds on it.
- `generated_cpp_bindings/X3Dtypes.hpp` — `SFVec3f`, `SFRotation`, `SFMatrix4f` are
  plain data structs with no ops. No matrix multiply or TRS composition exists
  anywhere in the tree — M2a adds it.
- `runtime/X3DScene.hpp` — `Scene.rootNodes` is the traversal entry; node children
  are MFNode fields reached via reflection (`X3DNode::fields()` /
  `defaultContainerField()`).

## Design

### 1. `DirtyTracker` (new) — the HdChangeTracker analog

A side structure of **per-node category bits**:

```cpp
enum DirtyFlags : unsigned {
  DirtyNone           = 0,
  DirtyLocalTransform = 1u << 0, // a Transform's TRS field changed
  DirtyWorldTransform = 1u << 1, // world matrix recomputed this tick
  DirtyChildren       = 1u << 2, // children/addChildren/removeChildren changed
  DirtyField          = 1u << 3, // any other field changed
  // DirtyBounds (1u<<4) reserved for M2b
};
```

API:
- `markDirty(const X3DNode* n, unsigned flags)` — OR the flags into n's entry, append
  n to the changed list on first transition from clean.
- `unsigned flags(const X3DNode* n) const`
- `const std::vector<const X3DNode*>& changedNodes() const` — the pull surface.
- `void clear()` — reset for the next tick (the consumer pulls *after* tick, *before*
  the next tick clears; see §5).

Granularity decision (made): **per-node category bits, not per-(node,field)** —
matches HdChangeTracker, keeps the changed-set small; per-field is recoverable later
if profiling demands it.

Lives in `runtime/scene/DirtyTracker.hpp`, namespace `x3d::runtime`.

### 2. Cascade feed — observer hook on `EventCascade`

Add an optional observer to `EventCascade`:

```cpp
// in EventCascade
void setFieldObserver(std::function<void(const FieldAddress&)> obs);
```

`deliver` (made non-static, or given access to the observer member) calls the
observer after a successful `info.set(...)`. The execution context wires this to a
**classifier** that maps the touched field to dirty flags on its node:
- Transform TRS fields `{translation, rotation, scale, center, scaleOrientation}` →
  `DirtyLocalTransform`.
- `{children, addChildren, removeChildren}` → `DirtyChildren`.
- anything else → `DirtyField`.

The classifier keys off the node type being `Transform` for the TRS set (via
`node->nodeTypeName()` or an `is-a` check — Transform only in M2a; HAnim and other
transform-bearing nodes are a later extension).

**Scoping boundary (made):** a System's `update()` that writes a node field
*directly* (bypassing the cascade `deliver`) is **not** auto-tracked. Such a system
calls `tracker.markDirty(node, ...)` explicitly. Auto-tracking every generated
setter is explicitly out of scope for M2a (it would require touching codegen / every
setter to notify). The cascade-deliver hook covers all ROUTE-delivered changes,
which is the common animation path (interpolator → Transform via ROUTE).

### 3. `Mat4` (new) — `runtime/math/Mat4.hpp`

A header-only 4×4 `float` matrix (column-major; document the convention):
- `static Mat4 identity()`
- `Mat4 operator*(const Mat4&) const` (matrix multiply)
- `static Mat4 translation(const SFVec3f&)`, `static Mat4 scale(const SFVec3f&)`,
  `static Mat4 rotation(const SFRotation&)` (axis-angle → matrix, via the existing
  `Quat`/`quatFromRotation`).
- `static Mat4 transformLocal(const Transform& t)` (or a free function taking the 5
  fields) computing the **X3D Transform matrix**:
  `M = T · C · R · SR · S · SR⁻¹ · C⁻¹`
  where T=translation, C=center, R=rotation, SR=scaleOrientation, S=scale (ISO/IEC
  19775-1 Transform semantics). Document each factor.

No external dependency (bundled-`tinfl` ethos). `float` precision matches the
SF*f field types.

### 4. `TransformSystem` (new) — world-transform side table + incremental propagation

`runtime/scene/TransformSystem.hpp`, namespace `x3d::runtime`.

- **Index build** (once, from a `Scene`): a depth-first walk from `scene.rootNodes`
  over MFNode/SFNode child fields (via reflection) records, for each `Transform`
  node, its parent `Transform` (nearest Transform ancestor, or none = root) and its
  child `Transform`s. Also seeds the world-transform table: `world(t) = world(parent)
  · localOf(t)`, root world = identity · local.
- **World-transform table:** `std::unordered_map<const X3DNode*, Mat4>` — world
  matrix per **Transform** node (made decision: per-Transform, not per-leaf; a
  geometry leaf's world transform is a query against its nearest enclosing Transform,
  deferred to M2b/extraction).
- **Propagation pass** `propagate(DirtyTracker&)` (run post-cascade in `tick`): for
  each node flagged `DirtyLocalTransform`, recompute its local matrix, set
  `world = parentWorld · local`, flag it `DirtyWorldTransform`, and **recurse into
  its child Transforms only** (the incremental win — subtrees with no dirtied
  ancestor are skipped). A node visited once per tick (guard against a node dirtied
  directly *and* via an ancestor — recompute-from-cleanest-ancestor, or a visited set
  scoped to the pass).
- **Query:** `const Mat4& worldTransform(const X3DNode*) const` (identity for an
  unknown/un-indexed node).

### 5. `X3DExecutionContext` integration + pull API

- The context owns a `DirtyTracker dirty_` and a `TransformSystem transforms_`.
- A one-time setup (e.g. `buildSceneGraph(Scene&)` or folded into `buildFrom`) builds
  the transform index and wires the cascade's field observer to the classifier.
- `tick(now)` gains a post-cascade step:
  1. `for (s : systems_) s->update(now, *this);`
  2. `cascade_.process();` (observer populates `dirty_` during this drain)
  3. `transforms_.propagate(dirty_);` (consumes `DirtyLocalTransform`, fills world
     matrices, sets `DirtyWorldTransform`)
- **Pull API:** after `tick` returns the consumer reads `dirtyTracker().changedNodes()`
  and `worldTransform(node)`. `dirty_.clear()` is called at the **start** of the next
  `tick` (so the changed-set survives until the consumer has pulled it). Pull-vs-push
  stays the M2.5 open decision; M2a ships **pull** (every shipping comparable runtime
  is pull).

### Scope boundaries (explicit)

- **In:** DirtyTracker + cascade feed + classifier; Mat4 (TRS compose + multiply);
  TransformSystem (index, world table, incremental propagation) for `Transform`;
  tick integration; pull API; tests.
- **Out (later milestones):** bounds (M2b, reserves `DirtyBounds`); per-leaf world
  transforms + full extraction payload (M2.5); runtime structural re-indexing on
  add/remove children (M2a builds the index once and flags `DirtyChildren`, but a
  full re-index on structural mutation is deferred); non-Transform transform-bearing
  nodes (HAnim etc.); auto-tracking of system direct field writes (explicit
  `markDirty` instead).

## File structure

| File | Responsibility |
|------|----------------|
| `runtime/math/Mat4.hpp` (new) | 4×4 float matrix: multiply + X3D TRS composition |
| `runtime/scene/DirtyTracker.hpp` (new) | per-node dirty category bits + changed list |
| `runtime/scene/TransformSystem.hpp` (new) | transform index, world-transform table, incremental propagation |
| `runtime/events/X3DEventCascade.hpp` | add field observer hook; `deliver` notifies |
| `runtime/events/X3DExecutionContext.hpp` | own DirtyTracker+TransformSystem; classifier wiring; post-cascade step in tick; pull API |
| `runtime/{math,scene}/tests/*.cpp` (new) | unit tests (wired in root CMakeLists.txt) |

(New `runtime/math/` and `runtime/scene/` areas; tests wired in the ROOT
`CMakeLists.txt`, consistent with `runtime/{codecs,parse,events}/tests/`.)

## Testing

1. **Mat4 TRS composition:** a known translation+rotation+scale composes to the
   expected matrix (and identity/multiply sanity); rotation matches the quaternion
   path.
2. **Dirty feed:** a ROUTE delivering into `Transform.translation` flags that node
   `DirtyLocalTransform` and lists it in `changedNodes()`; a non-transform field
   flags `DirtyField`.
3. **Incremental propagation:** a 3-deep Transform chain (A>B>C) plus a sibling
   subtree; dirty B's local transform → B and C world transforms update to the
   expected product, the sibling subtree's world transforms are unchanged (assert the
   sibling was NOT re-marked `DirtyWorldTransform`).
4. **tick integration + pull:** drive an interpolator→Transform ROUTE through a
   `tick(now)`; after it returns, `changedNodes()` contains the Transform and
   `worldTransform()` reflects the new local value.

## Verification

- `mise run build` (pytest + ctest) all green.
- **Golden byte-identical** — runtime-only; sha256 unchanged from
  `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`.
- Corpus smoke unaffected (M2a adds no parse path); a quick sanity that the existing
  event tests still pass.

## Risks / watch-items

- **`deliver` static→instance change** must not alter cascade semantics
  (single-timestamp, per-route-once). Keep the observer purely additive; existing
  cascade/animation tests must stay green.
- **Hierarchy traversal via reflection** — must follow only node-typed fields
  (SFNode/MFNode) and respect DEF/USE sharing (a USE'd node appears under multiple
  parents; for M2a the transform index treats the first-seen parent as canonical and
  notes shared nodes as a known limitation, since a USE'd Transform under two parents
  has two world transforms — deferred, flag it).
- **Matrix storage convention** — M2a uses **column-major** (§3). Document it at the
  top of `Mat4.hpp` and keep the TRS factor order (`T · C · R · SR · S · SR⁻¹ · C⁻¹`)
  and `operator*` consistent with it; the TRS test pins the convention.
- **Double-dirty within a tick** (a node dirtied directly and via an ancestor) — the
  propagation pass must recompute each subtree from its cleanest dirtied ancestor and
  visit each node at most once.

---
title: ROUTEs
summary: Route model, field addressing, and route resolution over effective fields including dynamic Script fields.
tags: [subsystem, routes, field-addressing, dynamic-fields]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/event-cascade.md
  - ../decisions/0014-dynamic-field-foundation.md
---

# ROUTEs

## Purpose

The ROUTEs subsystem owns the data model and resolution pipeline for X3D event routes — the declarative wiring that connects one node's output field to another's input field. It covers three separable concerns that together bridge authored intent all the way to the live event graph:

1. **Route data model** (`Route` struct): encoding-neutral record of a ROUTE statement, keyed by DEF name strings with optional resolved weak-pointer endpoints.
2. **Field addressing** (`FieldAddress`): identity of a routable endpoint — a raw observer `(node*, field-name)` pair — used by the event cascade to fan out events without owning nodes.
3. **Bridge and resolution** (`buildRoutes`): translates a parsed `Scene`'s DEF-name routes into typed, validated edges in an `EventGraph` held by an `X3DExecutionContext`. This step also handles PROTO-interface redirects, pre-resolved PROTO-body-internal routes, and pre-resolved Inline-internal routes.

A critical feature of the bridge is that field lookup always consults the node's *effective* field table — the union of generated static fields and author-declared dynamic Script fields — so author `<field>` declarations on `Script` nodes are first-class ROUTE endpoints (S1). **Caveat:** this is the *resolution* path — a ROUTE *into* a Script eventIn/inputOutput is registered/validated but not yet dispatched at tick time (`SCRIPT-EVENTIN`, open); a Script's own `outputOnly`/`inputOutput` writes *do* drive the cascade.

## Key files

| File / directory | Role |
|---|---|
| `runtime/X3DRoute.hpp` | `Route` struct: the encoding-neutral ROUTE data model |
| `runtime/events/X3DFieldAddress.hpp` | `FieldAddress` struct: typed, hashable event endpoint `(node*, field)` |
| `runtime/events/X3DEventGraph.hpp` | `EventGraph`: directed ROUTE table (source→sinks), including `resolveFieldAlias` for `set_xxx`/`xxx_changed` aliases |
| `runtime/events/X3DSceneBridge.hpp` | `buildRoutes`, `BridgeResult`, `RouteError`: the Scene→EventGraph bridge and validation pipeline; also `detail::findField` using `effectiveFields()` |
| `runtime/events/DynamicField.hpp` | `DynamicFieldStore`, `AuthorFieldDecl`, `effectiveFields()`: the dynamic-field side-table that makes author Script fields visible to route resolution |
| `runtime/X3DScene.hpp` | `Scene::routes`, `Scene::resolvedProtoRoutes`, `Scene::resolvedInlineRoutes`, `Scene::protoRedirects`, `Scene::resolveRoutes()` |

## Interfaces and seams

### Exposed interface

```cpp
// runtime/X3DRoute.hpp — encoding-neutral route record
namespace x3d::runtime {
struct Route {
  std::string fromNode;              // DEF name of source node
  std::string fromField;             // source field (outputOnly / inputOutput)
  std::string toNode;                // DEF name of destination node
  std::string toField;               // destination field (inputOnly / inputOutput)
  std::weak_ptr<X3DNode> from;       // optional resolved source (filled by Scene::resolveRoutes)
  std::weak_ptr<X3DNode> to;         // optional resolved sink
};
} // namespace x3d::runtime

// runtime/events/X3DFieldAddress.hpp — routable event endpoint
namespace x3d::runtime {
struct FieldAddress {
  X3DNode *node = nullptr;           // raw observer pointer (scene graph owns lifetime)
  std::string field;                 // X3D field name (post-alias normalization)
  bool operator==(const FieldAddress &) const;
};
} // namespace x3d::runtime
// std::hash<FieldAddress> specialization also provided.

// runtime/events/X3DEventGraph.hpp — directed ROUTE table
namespace x3d::runtime {
class EventGraph {
public:
  void addRoute(const FieldAddress &from, const FieldAddress &to);
  void removeRoute(const FieldAddress &from, const FieldAddress &to);
  const std::vector<FieldAddress> &sinks(const FieldAddress &from) const;
  std::size_t routeCount() const;
  void clear();
};

// Alias resolver — ISO/IEC 19775-1 §4.4.2.2
std::string resolveFieldAlias(const X3DNode *node, const std::string &name);
} // namespace x3d::runtime

// runtime/events/X3DSceneBridge.hpp — bridge entry point
namespace x3d::runtime {
struct RouteError { std::size_t index; std::string reason; };
struct BridgeResult { std::size_t routesAdded; std::vector<RouteError> rejected; bool ok() const; };

BridgeResult buildRoutes(Scene &scene, X3DExecutionContext &ctx);
// Also exposed as a convenience method:
BridgeResult X3DExecutionContext::buildFrom(Scene &scene); // defined in X3DSceneBridge.hpp
} // namespace x3d::runtime

// runtime/events/DynamicField.hpp — effective field lookup
namespace x3d::runtime {
FieldTable effectiveFields(const X3DNode &node);  // static fields() + author fields
DynamicFieldStore &dynamicFieldStore();            // process-global store
} // namespace x3d::runtime
```

### Seam points

- **Codec readers → Scene::routes** — all four codec readers (XML, ClassicVRML, VRML97, JSON) parse `<ROUTE>` / `ROUTE` statements and push `Route` structs (DEF names only, no resolved pointers) into `Scene::routes`. Resolved pointers are populated later by `Scene::resolveRoutes()` inside `buildRoutes`.

- **PROTO expander → Scene::resolvedProtoRoutes** — when `X3DProtoExpand` clones a PROTO body it resolves body-internal ROUTEs against the cloned node set and stores them in `Scene::resolvedProtoRoutes`. `buildRoutes` registers these directly, bypassing the DEF-table lookup (clone nodes are never in `scene.defs`).

- **Inline expander → Scene::resolvedInlineRoutes** — `InlineExpand` similarly stores inlined-scene-internal ROUTEs in `Scene::resolvedInlineRoutes` (same `ResolvedProtoRoute` shape, same direct-registration path in `buildRoutes`).

- **DynamicFieldStore → effectiveFields** — `detail::findField` in `X3DSceneBridge.hpp` calls `effectiveFields(node)` which concatenates the node's generated `fields()` table with the author fields from the process-global `DynamicFieldStore`. This makes Script author `<field>` declarations first-class ROUTE endpoints (S1 dynamic-field seam).

- **EventGraph → X3DEventCascade** — after `buildRoutes` populates the `EventGraph` inside `X3DExecutionContext`, the event cascade consults `EventGraph::sinks()` on each tick to propagate field changes across edges. The cascade holds the graph by reference; nodes are never owned.

- **SaiContext → EventGraph (dynamic ROUTE operations)** — `SaiContext::addRoute` and `SaiContext::deleteRoute` call `ctx_.addRoute` / `ctx_.removeRoute` directly on the live graph (ISO/IEC 19775-1 §4.3.7, guarded by `directOutput==TRUE`).

### Validation rules enforced by buildRoutes

`buildRoutes` applies four checks in order and returns diagnostics rather than throwing:

1. **Unresolved endpoint** — `Route::from` or `Route::to` weak_ptr is expired (forward ref, IMPORT, unknown DEF): skipped silently; not counted as added or rejected.
2. **Unknown field** — field name absent from the node's effective table: rejected with reason. If the unknown name names an exposed PROTO interface field, `redirectEndpoint` rewires onto IS-mapped body endpoints instead.
3. **Direction** — source must be `outputOnly` or `inputOutput`; sink must be `inputOnly` or `inputOutput`: else rejected.
4. **Type** — `fromField.type` must equal `toField.type`; X3D performs no implicit field-type coercion across a ROUTE (ISO/IEC 19775-1 §4.4.8.2): else rejected.

## How it is tested

- `ctest --preset dev -R x3d_event_scene_bridge` — the primary bridge test: validates ROUTE resolution, field-direction enforcement, type-mismatch rejection, author-field ROUTE endpoints, and PROTO-interface redirect.
- `ctest --preset dev -R x3d_events_tests` (doctest case: `cascade_test`) — route propagation (fan-out), loop-breaking, inputOnly-sink semantics; exercises the full `EventGraph::sinks()` path.
- `ctest --preset dev -R x3d_events_tests` (doctest case: `cascade_conformance_test`) — conformance scenarios that run routes across real parsed scenes.
- `ctest --preset dev -R x3d_events_tests` (doctest case: `cascade_alias_audit_test`) — `set_xxx` / `xxx_changed` alias resolution via `resolveFieldAlias`.
- `ctest --preset dev -R x3d_events_tests` (doctest case: `cascade_dynamic_route_test`) — dynamic `addRoute`/`deleteRoute` operations through `SaiContext` (SAI §4.3.7 path).
- `ctest --preset dev -R x3d_events_tests` (doctest case: `dynamic_field_test`) — `DynamicFieldStore` unit tests: `addAuthorField`, `effectiveFields`, type-mismatch drop counter (AUD-MEM-1), `getValue`/`setValue` contract.
- `ctest --preset dev -R x3d_inline_routes` — Inline-internal ROUTE pre-resolution and direct registration path.

## Related specs and ADRs

- [Architecture](../architecture.md) — where ROUTEs sit in the runtime: resolved by `buildFrom` into the execution context's event graph, then drained by the [event cascade](../subsystems/event-cascade.md) each tick.
- [ADR-0014: Dynamic Field Foundation](../decisions/0014-dynamic-field-foundation.md) — the decision to keep author Script fields in a per-node side-table (`DynamicFieldStore`) rather than in the generated node; explains why `effectiveFields()` exists as a free function and why route resolution routes through it.
- [Event Cascade subsystem](../subsystems/event-cascade.md) — consumes the populated `EventGraph` on each tick.
- ISO/IEC 19775-1 §4.4.2.2 — inputOutput field aliases (`set_xxx`, `xxx_changed`) implemented by `resolveFieldAlias`.
- ISO/IEC 19775-1 §4.3.7 — dynamic `addRoute`/`deleteRoute` via the Script/SAI interface.
- ISO/IEC 19775-1 §4.4.8.2 — no implicit field-type coercion across a ROUTE (exact type-tag match enforced by `buildRoutes`).
- Spec: `docs/superpowers/specs/2026-06-05-dynamic-field-foundation-spec.md` — the design for author-field ROUTE wiring (S1), including the process-global store rationale and `effectiveFields` seam.
- Spec: `docs/superpowers/specs/2026-06-17-script-cdata-untabling-design.md` — the S1 file-authored Script un-tabling that made author-field ROUTEs exercise the live `DynamicFieldStore`.

---
title: LoadSensor
summary: LoadSensor as a live X3DNetworkSensorNode — a time-driven System that observes each sensor's watched X3DUrlObject children through the AssetResolver seam and emits isActive/isLoaded/loadTime/progress per §9.4.3.
tags: [subsystem, loadsensor, networking, asset-resolver, sensors]
updated: 2026-09-26
related:
  - ../../explanation/architecture.md
  - system-asset-io.md
  - inline-expand.md
  - event-cascade.md
  - execution-context.md
  - ../../explanation/decisions/0046-loadsensor-assetresolver-oracle.md
---

# LoadSensor

The LoadSensor System implements ISO/IEC 19775-1 §9.4.3 `LoadSensor` — the
Networking node that generates events as its watched `children` (any
`X3DUrlObject`) load or fail to load. It polls the [AssetResolver](system-asset-io.md) and publishes load-state edges.
Design rationale is in
[ADR-0046](../../explanation/decisions/0046-loadsensor-assetresolver-oracle.md).

- The optional `AssetResolverWithBase(url, kind, baseUrl)` callback receives
  each watched child's source directory from `Scene::nodeBaseUrls` (an
  ownership lookup, not a scan), or the configured fallback when no entry
  exists.
- `SessionOptions::assetResolverWithBase` wires it through the standard
  runtime; it takes precedence over `assetResolver`.
- Ready memoization distinguishes base, URL, and kind so equal relative URLs
  in different Inline scenes cannot mark each other loaded.
- The legacy callback is unchanged.
- See [ADR-0063](../../explanation/decisions/0063-resolver-base-urls.md).

## Purpose

`LoadSensorSystem` implements the LoadSensor node (§9.4.3). It watches the
URL loads of the sensor's children through the injected `AssetResolver` and
emits `isActive`, `progress`, `isLoaded` and `loadTime`.

## Behavior

`LoadSensorSystem` is a time-driven `System`: it does its work in `update(now, ctx)`, called every tick. Per-sensor state lives in a map owned by the system — never on the node, which stays pure data. Each tick, per enabled sensor, it refreshes the watch set from `children`, polls each still-loading child through the injected `AssetResolver`, aggregates, and emits the spec's event edges.

### Event emission table (§9.4.3)

| Transition | Events emitted |
|---|---|
| Activation (first child begins loading) | `isActive=TRUE`, `progress=ready/total` |
| Progress change while active | `progress=newValue` |
| Terminal success (all watched children Ready) | `isLoaded=TRUE`, `loadTime=now`, `progress=1`, `isActive=FALSE` |
| Terminal failure (any child Failed, or timeout) | `isLoaded=FALSE`, `isActive=FALSE` |
| First-evaluation all-Ready | `isLoaded=TRUE`, `loadTime=now`, `progress=1` — no isActive pulse |
| `enabled` → FALSE while active | `isActive=FALSE` only; state resets, re-enable starts fresh |

`progress` is `readyCount / watchedCount` over unique watched children, emitted
when it changes while active, and 1.0 is guaranteed in the success burst. A sensor
with one pre-seeded and one loading child activates at `progress=0.5`. `loadTime`
is never sent on failure.

### Per-child state machine

Each watched, non-pre-seeded child runs
`NotStarted → Loading → {Ready | Failed}`:

- `load=FALSE` holds the child in `NotStarted`; if nothing is loading, the sensor
  never activates. A runtime `FALSE→TRUE` flip starts that child loading.
- `Loading` polls the `url` MFString candidates in order, ≤1 resolver call per
  child per tick; the first `Ready` wins, all-`Failed` is terminal `Failed`.
  Embedded schemes (`ecmascript:` / `javascript:` / `data:`) and an intra-scene
  `#Name` fragment resolving to a `Viewpoint` DEF short-circuit to `Ready` with no
  resolver call.
- `Ready` is memoized system-wide per URL (ADR-0045 precedent); `Failed`
  advances the candidate index; `Pending` is re-polled next tick.

### Rulings (spec-silent edges)

- R3 — empty `url` + `load=TRUE` → vacuous `Ready`.
- R4 — a url-change reset never emits `isLoaded=FALSE` by itself.
- R5 — duplicate `USE` children dedupe by pointer; `progress` denominator
  counts unique children.
- R6 — empty watch set → vacuous first-evaluation success burst.
- R7 — first-evaluation-all-Ready emits the success burst with no `isActive`
  pulse.
- R8 — an Inline counts as loaded only when it and every nested
  sub-Inline it asks to load (`load` TRUE) have loaded. Parse-time expansion
  replaces each loaded Inline with a Group, so an `Inline` node still inside an
  expanded child's content is a sub-Inline that failed: the child is `Failed`.

For an Anchor with a same-scene `#Viewpoint` URL, readiness waits until that
Viewpoint is bound (§9.4.3). Anchor's `load` field has no effect (§9.4.1).

### Watch-set resets

- Watched-child changes are detected by poll-and-diff (the codebase idiom for
  `inputOutput` fields): a per-child `{url, load}` snapshot.
- A `url` change resets the child to `Loading` and restarts the sensor's
  timeout window (spec-explicit); a `load` toggle restarts the child;
  membership growth reopens a terminal sensor.
- `timeOut` is measured from activation, re-read live each tick, and `0` means
  monitor indefinitely.

## Key files

| File / directory | Role |
|---|---|
| `runtime/events/LoadSensorSystem.hpp` | The System: per-sensor state, the per-tick algorithm, `ChildLoadPolicy` hook, `setSensorHook`/`setChildStateHook` test seams; header-only |
| `runtime/extract/AssetResolver.hpp` | The `(url, kind) → AssetResult` seam LoadSensor resolves through (ADR-0023); `makeNullAssetResolver()` is the IO-free default stub (Failed) |
| `runtime/io/file/FileResolver.hpp` | `makeFileResolver(baseDir)` — the SEC-3-confined local-file backend (Ready/Failed, never Pending); a concrete backend, kept out of the public SDK and injected by the app layer |
| `runtime/events/X3DSceneBridge.hpp` | `attachLoadSensors(scene, ctx, resolver)` production wiring; `attachStandardRuntime` gains the defaulted trailing `assetResolver` param |
| `runtime/extract/RuntimeSession.hpp` | `SessionOptions.assetResolver` pass-through to `attachStandardRuntime` |
| `tools/x3d-cli/sim_runtime.hpp` | `attachFullRuntime` injects `io::file::makeFileResolver()` — the app-layer backend that gives `x3d sim` confined local-file load state out of the box |
| `runtime/InlineExpand.hpp` | Parse-time `Scene::expandedInlines` — the map the pre-seed reads (an Inline in `children` becomes a synthetic `Group`, keyed to the original Inline) |
| `generated_cpp_bindings/x3d/nodes/LoadSensor.hpp` | The generated node: `getChildren`/`setChildren`, `getTimeOut`, `emitIsLoaded`/`emitLoadTime`/`emitProgress` |
| `runtime/events/tests/loadsensor_test.cpp` | Behavioral conformance tests (in `x3d_events_tests`) |
| `runtime/io/tests/file_resolver_test.cpp` | `x3d_fileresolver_test` — the confined default resolver |

## Interfaces and seams

```cpp
namespace x3d::runtime {

class LoadSensorSystem : public System {
public:
  // resolver == nullptr -> IO-free null stub (Failed); the app injects a backend
  explicit LoadSensorSystem(extract::AssetResolver resolver = nullptr);
  void setScene(const Scene *);              // reads Scene::expandedInlines for pre-seed
  void setChildLoadPolicy(ChildLoadPolicy);  // headed-embedder hook
  void setSensorHook(std::function<void(X3DNode*, bool active, double now)>);
  void setChildStateHook(std::function<void(X3DNode* sensor, X3DNode* child, ChildStatus)>);
};

// ChildLoadPolicy: per-child ruling for headed embedders.
struct ChildLoadPlan { bool watch = true; bool vacuousReady = false;
                       extract::AssetKind kind = extract::AssetKind::Inline; };
using ChildLoadPolicy = std::function<ChildLoadPlan(X3DNode* child, const Scene&)>;

// Production wiring (declared in X3DSceneBridge.hpp, defined in X3DSceneBridge.cpp):
std::shared_ptr<LoadSensorSystem>
attachLoadSensors(Scene&, X3DExecutionContext&,
                  extract::AssetResolver = nullptr);

} // namespace x3d::runtime
```

### Seam points

- `extract::AssetResolver` — the byte oracle (`(url, kind) → Ready/Pending/
  Failed`). Injected by the app/embedder; the default is the IO-free
  `makeNullAssetResolver()` stub (Failed), since the SDK ships no concrete backend.
  LoadSensor is its first SDK-side caller; `Pending` is honored so a future
  async/HTTP backend drops in unchanged.
- `setChildLoadPolicy` — headed embedders override per-child watch/vacuous/kind
  rulings; combined with a `Pending`-returning resolver, this expresses
  spec-literal Anchor cases (b)/(c) (see the findings register for the default deviation).
- `ctx.postEvent(node, field, value)` — how the system emits. Post the base
  x3dName (`"isActive"`, `"isLoaded"`, `"loadTime"`, `"progress"`); the cascade
  applies the node's emit-thunk during `tick()`, so getters reflect the value and
  ROUTEs fan out in the same tick.
- `ctx.tickGeneration()` — the per-tick idempotency gate. `tick()` re-invokes
  `update` in a settle loop until the cascade quiesces; the system does its
  side-effecting work (resolver polls, state transitions, bursts) once per
  generation and short-circuits later invocations in the same tick.
- `Scene::expandedInlines` — the parse-time pre-seed source (see
  [Inline Expand](inline-expand.md)). A children entry that is a key here is
  watched as terminal-`Ready`.

## Conformance

See the [findings register](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml) for conformance status.

## Related

- [ADR-0046](../../explanation/decisions/0046-loadsensor-assetresolver-oracle.md)
- [Asset Resolver / IO Seam](system-asset-io.md)
- [Inline Expand](inline-expand.md)
- [Execution context](execution-context.md)
- [Architecture](../../explanation/architecture.md)
- [Event cascade](event-cascade.md)

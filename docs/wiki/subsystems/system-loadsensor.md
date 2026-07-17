---
title: LoadSensor System
summary: LoadSensor as a live X3DNetworkSensorNode — a time-driven System that observes each sensor's watched X3DUrlObject children through the AssetResolver seam and emits isActive/isLoaded/loadTime/progress per §9.4.3.
tags: [subsystem, loadsensor, networking, asset-resolver, sensors]
updated: 2026-07-17
related:
  - ../architecture.md
  - ../subsystems/system-asset-io.md
  - ../subsystems/inline-expand.md
  - ../subsystems/event-cascade.md
  - ../subsystems/execution-context.md
  - ../decisions/0046-loadsensor-assetresolver-oracle.md
---

# LoadSensor System

The LoadSensor System implements ISO/IEC 19775-1 §9.4.3 `LoadSensor` — the
Networking node that generates events as its watched `children` (any
`X3DUrlObject`) load or fail to load. Before this system, `LoadSensor` was a
generated data holder with no behavior: nothing observed child load state and
none of `isActive` / `isLoaded` / `loadTime` / `progress` / `timeOut` existed.
`LoadSensorSystem` closes findings **NSN-1..7 and NSN-9** (six critical) and is
the first SDK-side caller of the [AssetResolver seam](system-asset-io.md)
(ADR-0023). Design rationale is in
[ADR-0046](../decisions/0046-loadsensor-assetresolver-oracle.md).

## Purpose

`LoadSensorSystem` is a **time-driven** `System`: it does its work in
`update(now, ctx)`, called every tick. Per-sensor state lives in a map owned by
the system — never on the node, which stays pure data. Each tick, per enabled
sensor, it refreshes the watch set from `children`, polls each still-loading
child through the injected `AssetResolver`, aggregates, and emits the spec's
event edges.

### Event emission table (§9.4.3)

| Transition | Events emitted |
|---|---|
| Activation (first child begins loading) | `isActive=TRUE`, `progress=ready/total` |
| Progress change while active | `progress=newValue` |
| Terminal success (all watched children Ready) | `isLoaded=TRUE`, `loadTime=now`, `progress=1`, `isActive=FALSE` |
| Terminal failure (any child Failed, or timeout) | `isLoaded=FALSE`, `isActive=FALSE` |
| First-evaluation all-Ready (NSN-9 / pre-seed) | `isLoaded=TRUE`, `loadTime=now`, `progress=1` — no isActive pulse |
| `enabled` → FALSE while active | `isActive=FALSE` only; state resets, re-enable starts fresh |

`progress` is `readyCount / watchedCount` over unique watched children, emitted
when it changes while active, and 1.0 is guaranteed in the success burst. A sensor
with one pre-seeded and one loading child activates at `progress=0.5`. `loadTime`
is **never** sent on failure.

### Per-child state machine

Each watched, non-pre-seeded child runs
`NotStarted → Loading → {Ready | Failed}`:

- `load=FALSE` holds the child in `NotStarted`; if nothing is loading, the sensor
  never activates. A runtime `FALSE→TRUE` flip starts that child loading.
- `Loading` polls the `url` MFString candidates in order, **≤1 resolver call per
  child per tick**; the first `Ready` wins, all-`Failed` is terminal `Failed`.
  Embedded schemes (`ecmascript:` / `javascript:` / `data:`) and an intra-scene
  `#Name` fragment resolving to a `Viewpoint` DEF short-circuit to `Ready` with no
  resolver call.
- **`Ready` is memoized system-wide per URL** (ADR-0045 precedent); `Failed`
  advances the candidate index; `Pending` is re-polled next tick.

### Rulings (spec-silent edges)

- **R3** — empty `url` + `load=TRUE` → vacuous `Ready`.
- **R4** — an NSN-7 reset never emits `isLoaded=FALSE` by itself.
- **R5** — duplicate `USE` children dedupe by pointer; `progress` denominator
  counts unique children.
- **R6** — empty watch set → vacuous first-evaluation success burst.
- **R7** — first-evaluation-all-Ready emits the NSN-9 burst with **no** `isActive`
  pulse.

### NSN-7 resets

Watched-child changes are detected by poll-and-diff (the codebase idiom for
`inputOutput` fields): a per-child `{url, load}` snapshot. A `url` change resets
the child to `Loading` and restarts the sensor's timeout window (spec-explicit); a
`load` toggle restarts the child; membership growth reopens a terminal sensor.
`timeOut` is measured from activation, re-read live each tick, and `0` means
monitor indefinitely.

## Key files

| File / directory | Role |
|---|---|
| `runtime/events/LoadSensorSystem.hpp` | The System: per-sensor state, the per-tick algorithm, `ChildLoadPolicy` hook, `setSensorHook`/`setChildStateHook` test seams; header-only |
| `runtime/extract/AssetResolver.hpp` | The `(url, kind) → AssetResult` seam LoadSensor resolves through (ADR-0023); `makeNullAssetResolver()` is the IO-free default stub (Failed) |
| `runtime/io/file/FileResolver.hpp` | `makeFileResolver(baseDir)` — the SEC-3-confined local-file backend (Ready/Failed, never Pending); a **concrete backend**, kept out of the public SDK and injected by the app layer |
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

// Production wiring (X3DSceneBridge.hpp):
std::shared_ptr<LoadSensorSystem>
attachLoadSensors(Scene&, X3DExecutionContext&,
                  extract::AssetResolver = nullptr);

} // namespace x3d::runtime
```

### Seam points

- **`extract::AssetResolver`** — the byte oracle (`(url, kind) → Ready/Pending/
  Failed`). Injected by the app/embedder; the default is the IO-free
  `makeNullAssetResolver()` stub (Failed), since the SDK ships no concrete backend.
  LoadSensor is its first SDK-side caller; `Pending` is honored so a future
  async/HTTP backend drops in unchanged.
- **`setChildLoadPolicy`** — headed embedders override per-child watch/vacuous/kind
  rulings; combined with a `Pending`-returning resolver, this expresses
  spec-literal Anchor cases (b)/(c) (the default deviation is recorded as `NSN-11`).
- **`ctx.postEvent(node, field, value)`** — how the system emits. Post the base
  x3dName (`"isActive"`, `"isLoaded"`, `"loadTime"`, `"progress"`); the cascade
  applies the node's emit-thunk during `tick`, so getters reflect the value and
  ROUTEs fan out in the same tick.
- **`ctx.tickGeneration()`** — the per-tick idempotency gate. `tick()` re-invokes
  `update` in a settle loop until the cascade quiesces; the system does its
  side-effecting work (resolver polls, state transitions, bursts) once per
  generation and short-circuits later invocations in the same tick.
- **`Scene::expandedInlines`** — the parse-time pre-seed source (see
  [Inline Expand](inline-expand.md)). A children entry that is a key here is
  watched as terminal-`Ready`.

## How it is tested

- **`ctest --preset dev -R x3d_events_tests`** (`runtime/events/tests/loadsensor_test.cpp`)
  — the primary behavioral suite: every emission-table row, the MFString
  fallback, all-candidates-failed, `timeOut` expiry vs. `timeOut=0`, the NSN-7
  url/load/membership resets with timeout restart, USE dedup (R5), the R3/R6
  vacuous cases, the embedded-scheme and Anchor `#Name` short-circuits, the
  `setChildLoadPolicy` hook, and a ROUTE `loadTime` end-to-end through
  `attachStandardRuntime`.
- **`ctest --preset dev -R x3d_fileresolver_test`** (`runtime/io/tests/file_resolver_test.cpp`)
  — the confined local-file backend (app-injected): an existing confined file is `Ready`; missing,
  escaping (`../`), absolute, and scheme-bearing paths are `Failed`; fragments are
  stripped; never `Pending`.
- **`ctest --preset dev -R x3d_extract_tests`** (`runtime/extract/tests/runtime_session_test.cpp`)
  — pins `SessionOptions.assetResolver` threading through to the LoadSensorSystem.

## Open findings

- **NSN-10** — DIS PDU nodes (`EspduTransform`/`ReceiverPdu`/`SignalPdu`/
  `TransmitterPdu`) remain inert, blocked on a DIS/networking transport seam
  (post-v1). Unrelated to LoadSensor beyond sharing the `X3DNetworkSensorNode`
  base.
- **NSN-11** — spec-literal Anchor children cases (b) replacement-world / (c)
  separate-window are not the SDK default (by design — the runtime is headless).
  Headed embedders express them via `setChildLoadPolicy` + a `Pending`-returning
  resolver. Deferred.

## Related specs and ADRs

- [ADR-0046](../decisions/0046-loadsensor-assetresolver-oracle.md) — the binding
  decisions: first SDK-side AssetResolver caller, resolver + pre-seed oracle,
  Ready-only memo, tick-generation idempotency, rulings R3–R7, Anchor policy hook.
- [Asset Resolver / IO Seam](system-asset-io.md) — the `AssetResolver` seam
  (ADR-0023) LoadSensor resolves through.
- [Inline Expand](inline-expand.md) — parse-time `Scene::expandedInlines`, the
  pre-seed source.
- [Execution Context](execution-context.md) — `postEvent` / `addSystem` / `tick`
  / `tickGeneration` seams.
- Design spec: `docs/superpowers/specs/2026-07-17-loadsensor-design.md`.
- Conformance source of truth: `docs/conformance/findings.yaml` (NSN-1..7, NSN-9
  closed; NSN-10, NSN-11 deferred).

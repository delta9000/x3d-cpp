---
title: "ADR-0046: LoadSensor as a System over the AssetResolver Seam"
summary: LoadSensor becomes a live X3DNetworkSensorNode via a time-driven LoadSensorSystem — the first SDK-side caller of the AssetResolver seam (ADR-0023). Per-sensor state lives in the system (never on the node); load truth comes from an injected resolver plus a parse-time pre-seed of already-expanded Inlines. A Ready-only system-wide URL memo, a ≤1-resolver-call-per-child-per-tick bound made idempotent against the cascade settle loop, a SEC-3-confined local-file default resolver, and rulings R3–R7 for the spec-silent edges. Closes NSN-1..7 and NSN-9.
tags: [adr, loadsensor, networking, asset-resolver, events, conformance]
updated: 2026-07-17
related:
  - ../subsystems/system-loadsensor.md
  - ../subsystems/system-asset-io.md
  - 0023-asset-resolver-seam.md
  - 0038-local-resolver-path-confinement.md
  - 0045-shared-mesh-instancing.md
---

# ADR-0046: LoadSensor as a System over the AssetResolver Seam

## Status

Accepted

## Context

`LoadSensor` (ISO/IEC 19775-1 §9.4.3) shipped as a generated data holder with
zero runtime behavior: nothing observed its `children`' load state, and none of
`isActive` / `isLoaded` / `loadTime` / `progress` / `timeOut` semantics existed.
Eight conformance findings (`NSN-1..7`, `NSN-9`), six critical, were all blocked
on one missing piece — a runtime that knows, per tick, whether each watched child
has loaded.

The `AssetResolver` seam (`runtime/extract/AssetResolver.hpp`, ADR-0023) was built
for exactly this shape of question — `Ready` / `Pending` / `Failed` with
retry-next-frame `Pending` — but nothing in the SDK called it. The texture path
holds the resolver on the *consumer* side (contract A, render-time). LoadSensor is
the first place the SDK itself needs a byte oracle.

## Decision

Introduce a time-driven **`LoadSensorSystem`** (`runtime/events/LoadSensorSystem.hpp`)
following the `ViewDependentSystem` pattern: per-sensor mutable state in an
`unordered_map<LoadSensor*, SensorState>` owned by the system, never on the node;
events via `ctx.postEvent(node, field, value)` so ROUTEs fire in-timestamp.

### 1. Load-truth oracle: injected resolver + parse-time pre-seed

Each watched child runs a per-child state machine driven by an injected
`extract::AssetResolver`. Children already expanded at parse time (found as keys
in `Scene::expandedInlines` — the synthetic `Group` that replaced an `Inline`) are
**pre-seeded terminal-Ready**, so the NSN-9 immediate burst reports a fact rather
than re-fetching. Rejected: resolver-only (re-fetches every expanded Inline;
spurious failures for HTTP-loaded Inlines under a file-only resolver) and
parse-time-truth-primary (parse-failure vs. skipped-scheme is indistinguishable at
runtime, and NSN-7 still needs the resolver path).

### 2. Default resolver: SEC-3-confined local file

When the embedder injects nothing, the system installs a new local-file
`AssetResolver` (`runtime/io/file/FileResolver.hpp`) that answers `Ready` when a
URL resolves inside the SEC-3 confinement root (ADR-0038,
`confineLocalIncludePath`) and is readable, `Failed` otherwise, **never
`Pending`** (v1 local I/O is synchronous). This matches Networking level 1
(`file:` protocol) and means a default CLI/session reports load state for local
files out of the box, without silently swallowing the NSN-9 burst.

### 3. One resolver call per child per tick, made idempotent

Blocking honesty: v1 backends are synchronous, so a resolver call can block a
tick. The system bounds this to **at most one resolver call per still-loading
child per tick**, and asks each URL candidate at most once ever — `Ready` is
memoized **system-wide per URL** (the ADR-0045 Ready-only-memo precedent),
`Failed` is cached per-child by advancing a candidate index, `Pending` is
re-polled next tick. Embedded schemes (`ecmascript:` / `javascript:` / `data:`)
and intra-scene `#Name` fragments short-circuit to `Ready` with no resolver call
at all.

The one non-obvious hazard: `X3DExecutionContext::tick()` re-invokes every
`System::update` in a settle loop *until the cascade quiesces*. A system that
polls a side-effecting resolver on each invocation would drain a scripted queue
(and violate the one-call bound) two or more times per tick. `TimeSensorSystem`
stays idempotent by recomputing purely from `now`; LoadSensor cannot (the
resolver has side effects and `Failed` advances a candidate). So the per-tick work
is **gated on `ctx.tickGeneration()`** — the first invocation does the work and
emits; later invocations in the same tick short-circuit, emitting nothing new,
which quiesces the loop.

### 4. Rulings for the spec-silent edges (R3–R7)

The spec is silent or environment-dependent on several cases; the system commits
to explicit, documented rulings:

- **R3** — empty `url` with `load=TRUE` → vacuous `Ready`.
- **R4** — an NSN-7 reset never emits `isLoaded=FALSE` by itself; `isLoaded` fires
  only on terminal transitions.
- **R5** — duplicate `USE` children dedupe by node pointer; the `progress`
  denominator counts unique watched children.
- **R6** — an empty watch set is a vacuous first-evaluation success burst
  (consistent with the already-loaded clause, which quantifies over all elements).
- **R7** — first-evaluation-all-`Ready` (including pre-seeded and vacuous cases)
  produces the NSN-9 `isLoaded`+`loadTime`+`progress=1` burst and emits **no**
  `isActive` pulse — the spec mandates only `isLoaded`+`progress` for the
  already-loaded case, and a same-update TRUE-then-FALSE would collapse to FALSE
  under last-writer-wins anyway.

### 5. Anchor and other environment-dependent kinds: default policy + hook

The runtime is headless, but its consumers may not be. The default policy is
uniform: an Anchor `url="#Name"` (no file part) is `Ready` iff a `Viewpoint` DEF'd
`Name` exists in the scene (the viewpoint-bind case); other URLs go to the
resolver, where `Ready` means "load request acknowledged". Headed embedders
override `LoadSensorSystem::setChildLoadPolicy` (returning a `ChildLoadPlan` —
`watch` / `vacuousReady` / `kind`) and express spec-literal Anchor cases (b)
replacement-world / (c) separate-window with a `Pending`-returning resolver —
"loaded when the OS acknowledges" is a resolver answer, not an SDK thread. That
default deviation is recorded as a deferred finding (`NSN-11`), not left silent.

## Consequences

- **Closes `NSN-1..7`, `NSN-9`** (six critical). `LoadSensorSystem` is wired by
  `attachStandardRuntime` / `attachFullRuntime` and threaded through
  `SessionOptions.assetResolver` + `baseUrl`, so an authored LoadSensor reports
  load state with no embedder wiring.
- **First SDK-side `AssetResolver` caller.** The seam existed for a milestone with
  no in-SDK consumer; this is it. The `Pending` contract — inert for the
  synchronous local-file default — is honored so a future async/HTTP backend
  drops in without touching the system.
- **State stays off the node.** The generated `LoadSensor` remains pure data; all
  per-sensor bookkeeping (watch set, activation, timeout window, url/load
  snapshots) lives in `LoadSensorSystem`, keeping the node model codec-clean.
- **Regression guard.** `runtime/events/tests/loadsensor_test.cpp` (in
  `x3d_events_tests`) pins every emission-table row, the MFString fallback,
  all-candidates-failed, timeout vs. `timeOut=0`, the NSN-7 url/load/membership
  resets, USE dedup, the R3/R6 vacuous cases, the embedded-scheme and Anchor
  `#Name` short-circuits, the policy hook, and a ROUTE `loadTime` end-to-end;
  `runtime/io/tests/file_resolver_test.cpp` pins the confined default resolver;
  `runtime/extract/tests/runtime_session_test.cpp` pins the `SessionOptions`
  pass-through.
- **Deferred (recorded, not silent).** Runtime re-expansion of an already-expanded
  Inline whose `url` changes at runtime (no runtime Inline expansion exists;
  pre-seeded children stay terminal-Ready), spec-literal Anchor cases (b)/(c) as
  the SDK default (`NSN-11`, available via the policy hook), and diagnostics for
  non-`X3DUrlObject` `children` entries (deferred to WB-1 unified diagnostics).

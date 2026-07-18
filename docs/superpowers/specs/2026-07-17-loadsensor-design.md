# LoadSensor runtime — a System over the AssetResolver seam (NSN-1..7, NSN-9)

- **Date:** 2026-07-17
- **Findings:** NSN-1 (minor), NSN-2/3/4/5/6 (critical), NSN-7 (major), NSN-9 (critical)
- **Spec:** ISO/IEC 19775-1 §9.4.3 LoadSensor, §9.3.2 X3DUrlObject
- **Status:** Design approved (brainstorming 2026-07-17); implementation plan follows.

## Problem

`LoadSensor` is a generated data holder with zero runtime behavior: nothing observes
the load state of its `children`, and none of `isActive` / `isLoaded` / `loadTime` /
`progress` / `timeOut` semantics exist. Eight findings in
`docs/conformance/findings.yaml` (NSN-1..7, NSN-9), six of them critical, all blocked
on one missing piece: a System that knows, per tick, whether each watched child has
loaded. The AssetResolver seam (`runtime/extract/AssetResolver.hpp`, ADR-0023) was
built for exactly this — `Ready/Pending/Failed` with retry-next-frame Pending — and
nothing calls it yet. This design makes `LoadSensorSystem` its first SDK-side caller.

## Spec contract (§9.4.3)

The clauses the implementation must satisfy, quoted at design time from the V4.0 text:

- Only `X3DUrlObject` descendants may appear in `children`; multiple children per
  sensor, and "output events are generated only when all of the children have loaded
  or at least one has failed."
- `isActive` TRUE "when the first element begins loading"; FALSE when loading
  completes (all loaded, or one failed), or the `timeOut` period is reached.
- `isLoaded` TRUE when all elements loaded; FALSE when one or more failed, or the
  timeout period is reached. "If all elements in the children are already loaded by
  the time the LoadSensor is processed, the LoadSensor shall generate an isLoaded
  event with value TRUE and a progress event with value 1 at the next event cascade."
- `loadTime` is generated only when loading has *successfully* completed.
- `progress` is in [0,1]; intermediate meaning is browser-dependent, but "the X3D
  browser shall in all cases guarantee that a progress value of 1 is generated upon
  successful load of all URL objects."
- `timeOut` is "the maximum time for which the LoadSensor will monitor loading,
  starting from when the sensor becomes active"; 0 means indefinite.
- On child reload (changed `url`, `load` toggled), the LoadSensor "resets … so that
  it monitors those elements based on the new values and resets its timeout period."
- Streamed media: first frame of data available counts as loaded.
- Anchor children have three environment-dependent cases (viewpoint bind /
  replacement world / separate window); the latter two are consumer behaviors.

## Decisions (locked during brainstorming)

1. **Load-truth oracle: resolver + parse pre-seed.** Each watched child runs a
   per-child state machine in the system, driven by an injected
   `extract::AssetResolver`. Children already expanded at parse time (found in
   `Scene::expandedInlines`) are pre-seeded terminal-Ready, so the NSN-9 immediate
   burst reports a fact instead of a re-fetch. Alternatives rejected: parse-time
   truth primary (parse-failure vs. skipped-scheme indistinguishable at runtime;
   NSN-7 still needs the resolver path) and resolver-only (re-fetches every expanded
   Inline; spurious failures for HTTP-loaded Inlines under a file-only resolver).
2. **Default resolver: local-file.** When the embedder injects nothing, the system
   uses a new SEC-3-confined local-file AssetResolver — file exists and is readable
   within the confinement root → Ready, else Failed, never Pending. Matches
   Networking level 1 (file: protocol). Rejected: fail-fast (default CLI sessions
   would report load failure for good local files) and inert-without-resolver
   (silently swallows even the pre-seeded NSN-9 burst).
3. **Anchor and other environment-dependent kinds: pragmatic default policy +
   consumer-overridable hook.** The runtime is headless, but its consumers may not
   be. The default policy is uniform (`url="#Name"` → Ready iff a Viewpoint with
   that DEF exists in the scene; other URLs → resolver, Ready = "load request
   acknowledged"). Headed embedders override `setChildLoadPolicy` and use Pending
   to express spec-literal Anchor cases (b)/(c) — "loaded when the OS acknowledges"
   is a resolver answer, not an SDK thread.

## §1 Architecture and components

New time-driven system `runtime/events/LoadSensorSystem.hpp`, following the
`X3DTimeDependentSystem` / `ViewDependentSystem` pattern: per-sensor mutable state
in an `unordered_map<LoadSensor*, SensorState>` owned by the system, never on the
node; events via `ctx.postEvent(node, field, value)` so ROUTEs fire in-timestamp.

```cpp
class LoadSensorSystem : public System {
public:
  // resolver == nullptr -> local-file default (SEC-3-confined)
  // baseUrl           -> base for relative child urls (Scene does not store one)
  explicit LoadSensorSystem(extract::AssetResolver resolver = nullptr,
                            std::string baseUrl = "");
  void attach(X3DNode *node, X3DExecutionContext &ctx) override; // dynamic_cast<LoadSensor*> gate
  void update(double now, X3DExecutionContext &ctx) override;

  void setChildLoadPolicy(ChildLoadPolicy);  // headed-embedder hook, see §2
  // Test seams, ViewDependentSystem precedent:
  void setSensorHook(std::function<void(X3DNode*, bool active, double now)>);
  void setChildStateHook(std::function<void(X3DNode* sensor, X3DNode* child, ChildStatus)>);
};
```

New backend `runtime/io/file/FileResolver.hpp`: `makeFileResolver()` returns an
AssetResolver answering Ready/Failed for confined local paths (never Pending).
Reuses the parse path's SEC-3 confinement root (`detail::activeConfineRoot()`,
ADR-0038) so runtime reads are confined exactly like parse-time reads.

Blocking honesty: v1 backends are synchronous, so a resolver call can block a tick.
Bounded to **at most one resolver call per still-loading child per tick**, and each
URL candidate is asked at most once ever (Ready memoized system-wide per URL,
ADR-0045 precedent; Failed cached per-child; Pending re-polled next tick). No
threads, no async machinery; Pending is honored contractually for future backends.

Registration: `attachLoadSensors(Scene&, X3DExecutionContext&,
extract::AssetResolver = nullptr, std::string baseUrl = "")` in
`runtime/events/X3DSceneBridge.hpp`; `attachStandardRuntime` gains the same two
defaulted trailing parameters; CLI `attachFullRuntime` and
`RuntimeSession`/`SessionOptions` pass them through. Watch filtering uses
`X3DInterfaceRegistry::nodeImplements(…, InterfaceId::X3DUrlObject)` (BindingSystem
precedent).

Inline identity note (verified in `expandInlines`): the expansion walk is blind over
all SFNode/MFNode fields, so an Inline inside `LoadSensor.children` is replaced by
its synthetic Group there. Pre-seed lookup is therefore: children entry ∈
`scene.expandedInlines` keys → watched as the map's original Inline, terminal-Ready.
An Inline left un-expanded (load=FALSE or unresolved at parse) stays an Inline in
`children` and takes the normal resolver path.

## §2 Child evaluation semantics

Watch set is built at `attach` and refreshed every tick. Entries: pre-seeded Group
(above) → terminal-Ready; `nodeImplements(X3DUrlObject)` → watched; anything else →
ignored per spec, no diagnostic channel invented here (WB-1 territory; documented on
the wiki page instead).

Per-child state machine (non-pre-seeded):

```
NotStarted --load=TRUE, url=[u1..un]--> Loading --first candidate Ready--> Ready (terminal)
                                             \--all candidates Failed---> Failed (terminal)
```

- `load=FALSE` holds the child in NotStarted; if no child is loading the sensor
  never activates (isActive TRUE only "when the first element begins loading"). A
  runtime FALSE→TRUE flip starts that child loading.
- Loading polls candidates in MFString order, ≤1 resolver call per tick; first Ready
  wins. Embedded schemes (`ecmascript:`, `javascript:`, `data:`) short-circuit to
  Ready — the bytes are in the string. Relative URLs are joined against the system's
  `baseUrl` before the call. AssetKind mapping: ImageTexture→Texture,
  MovieTexture/AudioClip→Movie (streamed-media first-frame rule), Inline→Inline,
  other URL-objects→Inline as generic bytes (kind is advisory to current backends).
- **Ruling R3 (spec-silent):** empty `url` with `load=TRUE` → vacuous Ready.

Anchor under the default policy: `url="#Name"` (no file part) → Ready iff a
Viewpoint with that DEF exists in the scene; other URLs → resolver. The policy hook:

```cpp
struct ChildLoadPlan { bool watch = true; bool vacuousReady = false;
                       extract::AssetKind kind = extract::AssetKind::Inline; };
using ChildLoadPolicy = std::function<ChildLoadPlan(X3DNode* child, const Scene&)>;
```

NSN-7 resets (poll-and-diff, the codebase idiom for inputOutput fields): per child,
snapshot `{url, load}`. `url` change → child resets to Loading and the sensor's
timeout window restarts (spec-explicit). `load` TRUE→FALSE → child back to
NotStarted. Membership change → added children evaluate fresh, removed drop out;
the timeout window restarts only if a child (re)entered Loading. **Ruling R4:** a
reset never emits `isLoaded=FALSE` by itself — isLoaded fires only on terminal
transitions. **Ruling R5:** duplicate USE entries dedupe by node pointer; the
progress denominator counts unique watched children.

## §3 Aggregate evaluation and event emission

Per tick, per enabled sensor: refresh watch set → poll Loading children → aggregate
→ emit edges. Timeout check: `active && timeOut>0 && now-activatedAt > timeOut` →
terminal failure; `timeOut` is re-read live; the window starts at activation.

| Transition | Events emitted |
|---|---|
| Activation (first child begins loading) | `isActive=TRUE`, `progress=ready/total` |
| Progress change while active | `progress=newValue` |
| Terminal success (all watched children Ready) | `isLoaded=TRUE`, `loadTime=now`, `progress=1`, `isActive=FALSE` |
| Terminal failure (any child Failed, timeout, no network) | `isLoaded=FALSE`, `isActive=FALSE` |
| First-evaluation all-Ready (NSN-9) | `isLoaded=TRUE`, `loadTime=now`, `progress=1` — no isActive pulse |
| `enabled` → FALSE while active | `isActive=FALSE` only; state resets, re-enable starts fresh (ENV-07 precedent) |

Progress uses the spec's browser latitude: `readyCount / watchedCount` over unique
watched children, emitted when the value changes while active, 1.0 guaranteed in the
success burst. A sensor with one pre-seeded and one loading child activates at
progress 0.5.

**Ruling R6 (spec-silent):** empty watch set → vacuous success burst (consistent
with the already-loaded clause, which quantifies over all elements).

**Ruling R7:** first-evaluation-all-Ready produces the NSN-9 burst even when
resolution happened synchronously inside that first update, and emits no isActive
pulse — the spec mandates only `isLoaded`+`progress` for the already-loaded case
(NSN-9 adds `loadTime`), and a same-update TRUE-then-FALSE would collapse to FALSE
under last-writer-wins anyway.

## Testing

New `runtime/events/tests/loadsensor_test.cpp` in the `x3d_events_tests` binary
(doctest, timesensor_test rig pattern): scripted lambda resolvers returning
Ready/Pending/Failed sequences per URL; drive `ctx.tick(t)`; assert node getters
(cascade applies emit-thunks during tick) and the test hooks. Cases: every emission
table row; MFString fallback (first Failed, second Ready); all-candidates-Failed;
timeout expiry with timeOut=5 vs. timeOut=0 indefinite; NSN-7 url-change reset with
timeout restart; load=FALSE gating then flip; enabled=FALSE mid-load; NSN-9 pre-seed
via real `parseDocument` with an Inline fixture (burst, no isActive pulse); vacuous
empty-children burst; USE-dedup denominator; Anchor `#Name` ruling; ecmascript:
short-circuit. Plus a ROUTE end-to-end test (standard_runtime_test pattern:
LoadSensor.loadTime routed to a target, assert delivery) and a
`runtime/io/tests/` FileResolver test (existing file → Ready, nonexistent and
outside-confine-root → Failed, never Pending).

## Docs / Definition of Done

- Findings NSN-1,2,3,4,5,6,7,9 flipped to `closed` (with shipping commit) +
  `mise run conformance`.
- New `docs/wiki/subsystems/system-loadsensor.md`; mkdocs nav entry; `coverage.md`
  row; `docs/wiki/seam-status.md` NSN-* line updated.
- One ADR capturing the binding decisions: first SDK-side AssetResolver caller,
  Ready-only memo, rulings R3–R7.
- `mise run docs-drift` reviewed before commit; `mise run ci` green.

## Out of scope (recorded, not silent)

- Runtime re-expansion of an already-expanded Inline whose `url` changes at runtime
  (no runtime Inline expansion exists at all; pre-seeded children stay
  terminal-Ready). Separate finding territory.
- Spec-literal Anchor cases (b)/(c) as SDK default — available through the policy
  hook; the default deviation is recorded in findings.yaml on ship.
- Diagnostics for non-X3DUrlObject `children` entries — deferred to WB-1 (unified
  diagnostics).

## References

- `docs/conformance/findings.yaml` NSN-1..7, NSN-9 (clause 9.4.3).
- ADR-0023 (AssetResolver seam; names LoadSensor as its consumer),
  ADR-0038 (SEC-3 confinement), ADR-0045 (Ready-only memo precedent),
  ADR-0017 (parse-time Inline expansion).
- `docs/wiki/subsystems/system-asset-io.md`, `sensors.md`, `system-viewdependent.md`.
- ISO/IEC 19775-1 V4.0 §9.3.2, §9.4.3.

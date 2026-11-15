---
title: RuntimeSession and SessionOptions
summary: Session construction, configuration defaults, tick overloads, accessors, diagnostics and ownership.
tags: [reference, sdk, session, embedding]
updated: 2026-11-15
related:
  - native-host-contract.md
  - subsystems/sdk-facade.md
  - ../how-to/integrate-a-native-host.md
---

# RuntimeSession and SessionOptions

Header: `x3d/sdk.hpp`; namespace: `x3d::sdk`; target: `x3d_cpp::sdk`.
The definitions are in `runtime/extract/RuntimeSession.hpp`.

`RuntimeSession` owns a document, execution context and extractor together.
It builds scene indices, resolves ROUTEs and attaches the configured systems
before `create` returns. The host supplies the renderer, thread and event loop.
See [Integrate a native host](../how-to/integrate-a-native-host.md) for the
steps, or the [native host contract](native-host-contract.md) for mutation and
render-feed rules.

## Construction and frame methods

Member signatures of `sdk::RuntimeSession`:

```cpp
static std::unique_ptr<RuntimeSession> create(X3DDocument doc,
                                            SessionOptions options = {});

void tick(double now);
void tick(double now,
          const std::vector<X3DExecutionContext::RetainedInput> &ingress);
void tick(double now, X3DExecutionContext::TickTimings *timings,
          const std::vector<X3DExecutionContext::RetainedInput> &ingress = {});

RenderDelta fullSnapshot();
RenderDelta delta();
```

| Method | Contract |
|---|---|
| `create` | Takes the document and options by value and returns an owning `unique_ptr`. The session itself is neither copyable nor movable |
| `tick` | Advances the context using host time in seconds; repeated timestamps are allowed |
| `tick` with `ingress` | Supplies a retained batch before systems run; admission and value rules are in [retained batch ingress](subsystems/execution-context.md#retained-batch-ingress) |
| `tick` with `timings` | Fills phase timings when the pointer is non-null. A null pointer disables those clock reads; see [Measure tick phases](../how-to/measure-tick-phases.md) |
| `fullSnapshot` | Returns every current placement in `added` and replaces the extraction baseline |
| `delta` | Returns changes for the latest tick. Before a baseline exists it returns a full snapshot; a second call without a tick returns an empty delta |

Every `tick` overload appends pending runtime Inline warnings to
`document().inlineWarnings` and drains the scene's pending warning vector.
Calling `context().tick()` directly bypasses this drain.

Extraction is separate from ticking: the host requests it with `fullSnapshot`
or `delta`. Delta and descriptor lifetimes follow
[Extraction pipeline](subsystems/extract.md#one-delta-per-tick-contract).

## SessionOptions

Options are consumed at construction. Changing the caller's options object
after `create` does not reconfigure the session.

The `runtime::` qualifications on this page mean `x3d::runtime`; those types
are not aliases in `x3d::sdk`. The other type names are available through the
[SDK facade](subsystems/sdk-facade.md).

### Behavior and input

| Member and type | Default | Effect |
|---|---|---|
| `standardRuntime` (`bool`) | `true` | Attaches the standard behavior systems. `false` leaves their installation to the host; scene indexing, ROUTE resolution and extraction still run |
| `interactive` (`bool`) | `false` | Installs pointing-device sensors, Anchors and SDK navigation; `navigation()` exposes the navigation system |
| `pointingSensors` (`bool`) | `false` | Installs pointing-device sensors when `interactive` is false, without Anchors or SDK navigation |
| `anchorHandler` (`runtime::AnchorSystem::AnchorHandler`) | Empty | With `interactive`, receives activated Anchors with external URLs and their parameters. An empty handler ignores these activations; fragment Viewpoint binding remains internal |

### Extraction and coordinates

| Member and type | Default | Effect |
|---|---|---|
| `geoProjection` (`std::shared_ptr<const runtime::geo::GeoProjection>`) | Null | Selects the built-in projection when null; otherwise retains the supplied projection for the context's lifetime. See [Geospatial](subsystems/geospatial.md#world-ownership-and-migration) |
| `meshOptions` (`MeshBuildOptions`) | `{}` | Passed unchanged to the extractor for tessellation, font metrics, external geometry and shader selection. Its shader options also configure the standard runtime's ShaderSystem. See [extraction seams](subsystems/extract.md#seam-points) |

### Resource resolution

| Member and type | Default | Effect |
|---|---|---|
| `textureResolver` (`TextureResolver`) | `makeNullTextureResolver()` | Supplies decoded URL texture pixels to extraction; the default returns `Failed` |
| `textureResolverWithBase` (`TextureResolverWithBase`) | Empty | Contextual texture callback; takes precedence over `textureResolver` |
| `assetResolver` (`AssetResolver`) | Null | Supplies the standard runtime's LoadSensor byte requests; null selects a failed-loading stub. Parsing, texture decoding and runtime Inline loading use separate callbacks |
| `assetResolverWithBase` (`AssetResolverWithBase`) | Empty | Contextual LoadSensor callback; takes precedence over `assetResolver` |
| `inlineResolver` (`runtime::InlineResolver`) | Empty | A nonempty value enables the standard runtime's deferred Inline system and supplies its fallback loader |
| `baseUrl` (`std::string`) | Empty | Fallback source directory for Inline loading, contextual texture resolution and contextual LoadSensor requests |

### Precedence and resource boundaries

- `interactive` includes pointing sensors. Setting both flags installs them
  once. Both flags are independent of `standardRuntime`.
- With `standardRuntime = false`, the session does not pass the asset, Inline,
  base URL or shader options to standard systems. Extraction still receives
  `meshOptions`, the texture callbacks and `baseUrl`.
- Contextual callbacks receive the authored URL and its recorded node source
  base, falling back to `baseUrl` when that base is absent. A failure from a
  contextual callback does not retry the plain callback.
- Asset-loaded Inline nodes can retain their original loader and source
  ancestry. When the runtime Inline system is enabled, that retained callback
  takes precedence over `inlineResolver` for its node. Supplying a fallback
  does not replace retained policies. See [Inline expansion](subsystems/inline-expand.md#seam-points).
- Parsing happens before `create`. Session resolvers do not control earlier
  `parseFile` or `parseDocument` reads. See [Supply resources through the seams](../how-to/supply-resources.md).
- Script, physics and audio backends require separate wiring; no
  `SessionOptions` member installs them.

## Accessors

| Member | Return type | Meaning |
|---|---|---|
| `context()` | `X3DExecutionContext &` | Input, systems, events and runtime state |
| `extractor()` | `SceneExtractor &` | Render descriptors, diagnostics and scene-level extraction |
| `scene()` | `Scene &` | The owned document's scene |
| `document()` | `X3DDocument &` | The owned document and parse/runtime warning collections |
| `routes() const` | `const BridgeResult &` | ROUTE-resolution results from construction, including `routesAdded` and `rejected`; not a live route inventory |
| `navigation() const` | `const std::shared_ptr<runtime::NavigationSystem> &` | Null unless `interactive` was enabled |
| `pointingSensors(const PickResult &) const` | `std::vector<X3DNode *>` | Enabled sensors at a current pick's lowest applicable scope; a geometric query that does not require installed sensor systems or alter pointer/grab state |

`context`, `extractor`, `scene` and `document` also have const overloads
returning const references. Accessors borrow session-owned state; sensor
pointers borrow node lifetime. A pick and its results must be discarded before
removing their nodes. Sensor-query details are in
[Host-owned navigation](subsystems/system-pointing.md#host-owned-navigation).

## Diagnostics, errors and lifetime

### Scene acceptance and exceptions

Parse failures can throw before session construction. Warning collections,
ROUTE rejections, skipped geometry and partial extraction are separate
[scene acceptance checks](../how-to/integrate-a-native-host.md#2-check-the-scene-before-you-accept-it).

Session methods propagate allocation, backend and callback exceptions rather
than translating them into a result type. Exceptions can leave partial effects;
rollback is not promised. `InlineCycleError` has the specific warning
conversion described in
[Inline expansion](subsystems/inline-expand.md#fetched-and-runtime-diagnostics).

### Destruction and borrowed state

One thread drives the context and extractor. Every native call and node
callback must finish before destruction. Destroying the session while its
callbacks cannot retire calls `std::terminate`.

The document, context and extractor have dependency-ordered lifetimes.
References returned by accessors cannot outlive the session. Copying a document
with shared nodes does not create an independent mutable graph for another
live activation; see [Activation ownership](subsystems/activation-ownership.md).

Resource callbacks are retained by value and may retain host objects through
their captures. Host state borrowed by those captures must remain valid while
the callbacks can run, including loaders retained by an Inline scene.

## Related

- [Native host contract](native-host-contract.md)
- [SDK facade](subsystems/sdk-facade.md)
- [Integrate a native host](../how-to/integrate-a-native-host.md)

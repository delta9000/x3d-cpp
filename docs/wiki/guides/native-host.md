---
title: Native host integration
summary: Own session timing, input, resources, and render-feed consumption.
tags: [guide, embedding]
updated: 2026-11-05
related: []
---

# Native host integration

Include `x3d/sdk.hpp` and link `x3d_cpp::sdk` from
`find_package(x3d_cpp CONFIG REQUIRED)`. The boundary is an in-process C++20
library. `RuntimeSession` does not create a window, graphics device, or renderer.
The host owns presentation, input mapping, frame pacing, and GPU lifetime.

`examples/embed_minimal/native_host.cpp` exercises a CPU mirror of the render
feed with static meshes and Phong materials. It runs in-tree as
`x3d_native_host_contract`; `scripts/verify_install_embed.sh` also builds and
runs it against installed and relocated packages. These are library-boundary
checks, not Vulkan or CAVEOS renderer validation.

To build an installed consumer from the repository root:

```sh
cmake -S . -B build -G Ninja -DX3D_CPP_PER_HEADER_CHECKS=OFF
cmake --build build --target x3d_cpp_runtime -j3
cmake --install build --prefix "$PWD/install"
cmake -S examples/embed_minimal -B build-host -G Ninja -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build build-host --target x3d_embed_native_host -j3
./build-host/x3d_embed_native_host
```

## Session and tick ownership

`RuntimeSession::create(document, options)` owns the document, context, and
extractor in dependency order. The session is heap-pinned and cannot move;
its `unique_ptr` can. Standard behaviors attach by default. Pointer navigation
requires `SessionOptions::interactive`. A host that owns navigation can set
`SessionOptions::pointingSensors` to install pointing-device sensors alone and
query them with `session->pointingSensors(hit)`; see
[host-owned navigation](../subsystems/system-pointing.md#host-owned-navigation).
Scripts, physics, and audio need explicit backend wiring.

Drive the session from one host thread:

1. Supply mapped input and tracking pose.
2. Call `tick(now)` with simulation time in seconds.
3. Consume and apply `delta()` before the next tick.
4. Submit renderer work with host-owned view/projection and synchronization.

The dirty set covers one tick. Consume a delta after every simulation substep,
or recover a missed delta with a full snapshot. Callback execution is synchronous;
avoid concurrent mutation, extraction, and reentrant calls.

`setPointer` takes a world-space ray. `setHeadPose` composes tracking after the
bound Viewpoint and navigation offset; with no bound Viewpoint it composes at
the world origin. The host owns unprojection, device mapping, per-eye
projection, and graphics API clip conventions.

A host that owns the camera absolutely passes its rigid camera-to-world matrix:

```cpp
session->context().setViewerWorldPose(worldFromHead);
session->tick(now);
auto authored = session->context().boundViewpointWorldPose();
// Return camera control to the bound Viewpoint and stored relative inputs:
session->context().clearViewerWorldPose();
```

The override works without a Viewpoint and stays active across binds and
`setHeadPose` calls until cleared. `setViewerWorldPose` returns `false` and
keeps the previous pose for a non-finite matrix or one with scale, shear, or
reflection beyond a 1e-3 tolerance; small float drift is re-orthonormalized.
Set it before ticking view-dependent systems. `boundViewpointWorldPose()`
excludes navigation, head tracking, and the override, and returns identity when
no Viewpoint is bound. See
[ADR-0065](../decisions/0065-absolute-viewer-world-pose.md).

## Mutation

Raw generated setters bypass context dirty tracking. The example queues host
writes and applies `context.writeField` from `System::update`, checking each
`FieldWriteResult`. Consume that queue once: settlement can run a System more
than once during one tick. A write immediately before `tick` loses its dirty
mark when the tick clears the previous set. X3D input events can instead use
`postEvent` for delivery in the next cascade.

A field write does not enroll arbitrary behavior nodes, resolve new ROUTEs, or
repair every scene index. For whole-scene changes, prepare a new session and
swap it at a frame boundary. `RuntimeSession` has no transactional reload API.

Structural SFNode/MFNode edits and `visible` changes produce a replacement
extraction snapshot. Scoped descriptors and visibility-hint changes can also
require that path. Ordinary scalar geometry/material/TRS edits use incremental
channels where possible. See [Extraction](../subsystems/extract.md) for the
dependency and replacement rules; don't assume subtree-cost updates.

Switch `whichChoice` and per-placement LOD selection changes keep unaffected
`RenderItemId`s and immutable mesh payloads. Only placements entering or
leaving the active selection appear in `removed`/`added`. A level that leaves
and re-enters the selection without an edit returns with the same `GeomId` and
mesh, so a `GeomId`-keyed upload cache can keep its buffers across LOD boundary
crossings if it retains them for removed placements. An edit while the level
was inactive gives it a new `GeomId`. Surviving placements whose material,
shader, or skin binding changed are listed in `updatedMaterial` or
`updatedSkinPose`. Records whose scoped light, fog, or clip-plane state changed
appear in both `removed` and `added` with the same ID. Surviving external
packed placements receive `updatedGeometry`. Node-reference edits in the same
tick still take the replacement snapshot. Selection refresh walks the active
scene within a budget, so its cost is not proportional to the changed subtree
alone. See [ADR-0069](../decisions/0069-selection-deltas-and-cheap-ticks.md).

`lightsChanged` compares the complete world-resolved light list with the last
published one, including scope, attenuation, radius, and shadow controls, and
covers lights the geometry walk does not reach (`visible=FALSE`,
`Collision.proxy`). An idle tick or unrelated animation leaves it false; full
snapshots set it true. Camera, background, and fog flags are set on every
delta. `snapshotLights()` owns the indices that items reference; advancing
`delta()` calls in scenes with lights re-collect it and re-tag live items.

## Delta consumption and lifetime

`RenderDelta` identifies placements. Read `extractor.item(id)` and copy needed
descriptor fields before the next mutation or extraction. Apply removals before
additions because replacement can reuse dense IDs. Apply every supported update
channel and camera/light/background/fog flag. A `fullSnapshot()` replaces the
host's placement set rather than appending to it.

Track the live set from deltas; `itemCount()` can include old slots. Clear
ID-indexed state on unload and use a host-owned session/baseline epoch when
buffering frames. `RenderItemId`, pointer-based `PathKey`, and `GeomId` are
local identities, not persistent IDs or a transport format.

`MeshRef` and `TexturePixelsRef` retain immutable CPU payloads. Copying an
entire descriptor does not make its raw node/path/light/skin identities owning.
For skin items, corner remaps live on the shared mesh; the skin descriptor
carries binding and pose state. Defer GPU resource retirement using the host's
own fences or timeline values. Distributed rendering needs a separate,
versioned host protocol.

## Measure runtime phases

With `X3D_CPP_BUILD_EXAMPLES=ON`, the `x3d_runtime_tick_profile` target
(`tools/runtime_tick_profile.cpp`) takes a scene filename and a positive tick
count:

```sh
./build/x3d_runtime_tick_profile /path/to/scene.x3d 91
```

It parses the scene, creates a standard `RuntimeSession`, takes a full
snapshot, and advances from zero at 30 Hz with a fixed authored viewer and no
pointer. It prints parse, session, and snapshot milliseconds, then CSV rows for
systems/cascade, post-cascade hooks, transforms, bounds, total tick, and delta,
plus placement/update counts and `lightsChanged`. It fails if the extraction
walk budget is exhausted. Compare runs built with identical settings; the
timings are not a real-time guarantee.

In your own host, pass a `sdk::X3DExecutionContext::TickTimings` to
`session.context().tick(now, &timings)` to receive the same per-phase
milliseconds. Plain `tick(now)` reads no clocks.

## Resources and errors are explicit

For complete resource control, parse memory with explicit EXTERNPROTO and
Inline callbacks. Defaults can read local files, even through `parseDocument`.
Use callable deny resolvers to reject access; empty callbacks are not a global
resource policy. Parse-time resolution must complete synchronously or fail.

Session options expose separate asset, texture, runtime Inline, base URL, and
shader configuration. One callback does not configure every resource path.
`textureResolverWithBase` and `assetResolverWithBase` are optional variants
that receive the authored URL and the requesting node's source base (see
[ADR-0063](../decisions/0063-resolver-base-urls.md)).
`Pending` creates no background job; the host owns loading, timeout, and
cancellation. Advancing deltas retry pending texture and geometry resources.
Ready textures are memoized; changing bytes behind an unchanged Ready URL
requires a full snapshot. Failed textures are not automatically polled.

Inspect parse warnings, rejected routes, skipped geometry, and extraction
budgets before accepting a scene. Unresolved DEF/IMPORT endpoints can be
silently skipped, so an empty rejection list is insufficient; the example
checks an expected `routesAdded` count.

Reject a budget-exceeded snapshot before publishing partial authoritative
state. After fixing the content or budget, require a successful full snapshot
before resuming deltas. Exceptions do not imply rollback; preserve the host's
last valid state or recreate the session according to host policy.

The [seam matrix](../seam-status.md) records the experimental render-feed
boundary. Pin an SDK revision during integration and test the actual host;
the example does not establish full X3D conformance, thread safety, a stable
C ABI, hard real-time bounds, or hardware rendering correctness.

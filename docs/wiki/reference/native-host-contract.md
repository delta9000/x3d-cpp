---
title: Native host contract
summary: The rules a host follows for session ownership, input, mutation, deltas, lifetimes and resources.
tags: [reference, embedding, extraction]
updated: 2026-11-14
related:
  - ../how-to/integrate-a-native-host.md
  - ../explanation/tick-model.md
  - seam-matrix.md
---

# Native host contract

The boundary is an in-process C++20 library: include `x3d/sdk.hpp`, link
`x3d_cpp::sdk`. `RuntimeSession` creates no window, graphics device or
renderer. The host owns presentation, input mapping, frame pacing and GPU
lifetime.

For the steps, see [Integrate a native host](../how-to/integrate-a-native-host.md).
For the reasoning, see [The tick model](../explanation/tick-model.md).

## Session

| Rule | Detail |
|---|---|
| Ownership | `RuntimeSession::create(document, options)` owns the document, context and extractor in dependency order |
| Movement | The session is heap-pinned and cannot move; its `unique_ptr` can |
| Standard behaviors | Attached by default |
| Pointer navigation | Needs `SessionOptions::interactive` |
| Pointing sensors without navigation | `SessionOptions::pointingSensors`, queried with `session->pointingSensors(hit)`; see [host-owned navigation](subsystems/system-pointing.md#host-owned-navigation) |
| Scripts, physics, audio | Need explicit backend wiring |
| Reload | No transactional reload API. Prepare a new session and swap it at a frame boundary |

## Tick order

One host thread drives the session:

1. Supply mapped input and tracking pose.
2. Call `tick(now)` with simulation time in seconds.
3. Consume and apply `delta()` before the next tick.
4. Submit renderer work with host-owned view, projection and synchronization.

- The dirty set covers one tick. Consume a delta after every simulation
  substep, or recover a missed delta with a full snapshot.
- Callbacks run synchronously. Avoid concurrent mutation, extraction and
  reentrant calls.

## Input and view conventions

| Call | Convention |
|---|---|
| `setPointer` | Takes a world-space ray |
| `setHeadPose` | Composes tracking after the bound Viewpoint and navigation offset; with no bound Viewpoint it composes at the world origin |
| `context().setViewerWorldPose(m)` | Rigid camera-to-world override. Works without a Viewpoint; stays active across binds and `setHeadPose` calls until cleared. Set it before ticking view-dependent systems |
| `context().clearViewerWorldPose()` | Returns camera control to the bound Viewpoint and the stored relative inputs |
| `context().boundViewpointWorldPose()` | The authored bound viewpoint only: excludes navigation, head tracking and the override; identity when no Viewpoint is bound |

`setViewerWorldPose` returns `false` and keeps the previous pose for a
non-finite matrix or one with scale, shear or reflection beyond a 1e-3
tolerance. Small float drift is re-orthonormalized. See
[ADR-0065](../explanation/decisions/0065-absolute-viewer-world-pose.md).

The host owns unprojection, device mapping, per-eye projection and graphics
API clip conventions.

## Mutation

| Way to change the scene | Behavior |
|---|---|
| Raw generated setter | Bypasses context dirty tracking |
| `context.writeField` from `System::update` | Dirty-aware. Returns a `FieldWriteResult`; check it |
| `context.writeField` immediately before `tick` | Loses its dirty mark when the tick clears the previous set |
| `postEvent` | Delivers an X3D input event in the next cascade |

- Settlement can run a System more than once during one tick. A System that
  drains a host queue must consume it once.
- A field write does not enroll arbitrary behavior nodes, resolve new ROUTEs
  or repair every scene index.

### What an edit costs in extraction

| Edit | Extraction path |
|---|---|
| Structural SFNode/MFNode edit, `visible` change | Replacement snapshot |
| Scoped descriptor or visibility-hint change | Can require a replacement snapshot |
| Ordinary scalar geometry, material or TRS edit | Incremental channels where possible |
| Node-reference edit in the same tick as a selection change | Replacement snapshot |

Don't assume subtree-cost updates. The dependency and replacement rules are in
[Extraction](subsystems/extract.md).

### Switch and LOD selection changes

- Unaffected placements keep their `RenderItemId`s and mesh payloads; only
  placements entering or leaving the active selection appear in `removed` and
  `added`.
- An unedited level that leaves and re-enters comes back under the same
  `GeomId`, so a `GeomId`-keyed upload cache can keep its buffers across LOD
  boundary crossings.

The full rules, including which update channels a surviving placement gets,
are in [view-dependent placements](subsystems/extract-deltas.md#view-dependent-placements)
and [ADR-0069](../explanation/decisions/0069-selection-deltas-and-cheap-ticks.md).

### Light and environment flags

| Flag | Set when |
|---|---|
| `lightsChanged` | The complete world-resolved light list differs from the last published one, including scope, attenuation, radius and shadow controls. Covers lights the geometry walk does not reach (`visible=FALSE`, `Collision.proxy`). False on an idle tick or unrelated animation; true on a full snapshot |
| Camera, background and fog flags | Set on every delta |

`snapshotLights()` owns the indices that items reference. Advancing `delta()`
calls in scenes with lights re-collect it and re-tag live items.

## Delta consumption

- `RenderDelta` identifies placements. Read `extractor.item(id)` and copy the
  descriptor fields you need before the next mutation or extraction.
- Apply removals before additions; replacement can reuse dense IDs.
- Apply every supported update channel and the camera, light, background and
  fog flags.
- A `fullSnapshot()` replaces the host's placement set. It does not append.
- Track the live set from deltas. `itemCount()` can include old slots.

## Identity and lifetime

| Thing | Rule |
|---|---|
| `RenderItemId`, pointer-based `PathKey`, `GeomId` | Local identities. Not persistent IDs and not a transport format |
| ID-indexed host state | Clear it on unload. Use a host-owned session or baseline epoch when buffering frames |
| `MeshRef`, `TexturePixelsRef` | Retain immutable CPU payloads |
| A copied descriptor | Its raw node, path, light and skin identities stay non-owning |
| Skin items | Corner remaps live on the shared mesh; the skin descriptor carries binding and pose state |
| GPU resource retirement | Defer with the host's own fences or timeline values |
| Distributed rendering | Needs a separate, versioned host protocol |

## Resources

| Topic | Rule |
|---|---|
| Parse-time loading | Defaults can read local files, even through `parseDocument`. For complete control, parse memory with explicit EXTERNPROTO and Inline callbacks |
| Denying access | Use callable deny resolvers. Empty callbacks are not a global resource policy |
| Parse-time resolution | Must complete synchronously or fail |
| Session options | Separate asset, texture, runtime Inline, base URL and shader configuration. One callback does not configure every resource path |
| `textureResolverWithBase`, `assetResolverWithBase` | Optional variants that receive the authored URL and the requesting node's source base; see [ADR-0063](../explanation/decisions/0063-resolver-base-urls.md) |
| `Pending` | Creates no background job. The host owns loading, timeout and cancellation. Advancing deltas retry pending texture and geometry resources |
| Ready textures | Memoized. Changing bytes behind an unchanged Ready URL requires a full snapshot |
| Failed textures | Not polled again automatically |

## Errors

- Parse warnings, rejected routes, skipped geometry and extraction budgets are
  reported, not thrown. Inspect them before accepting a scene.
- Unresolved DEF or IMPORT endpoints can be skipped silently, so an empty
  rejection list is not sufficient evidence. The example checks an expected
  `routesAdded` count.
- A budget-exceeded snapshot is partial. Reject it before publishing it as
  authoritative state, and require a successful full snapshot before resuming
  deltas.
- Exceptions do not imply rollback. Preserve the host's last valid state or
  recreate the session according to host policy.

## Evidence and limits

`examples/embed_minimal/native_host.cpp` exercises a CPU mirror of the render
feed with static meshes and Phong materials. It runs in-tree as
`x3d_native_host_contract`, and `scripts/verify_install_embed.sh` builds and
runs it against installed and relocated packages. These are library-boundary
checks, not Vulkan or CAVEOS renderer validation.

The render feed is experimental; the [seam matrix](seam-matrix.md) records its
state. Pin an SDK revision during integration and test the actual host. The
example does not establish full X3D conformance, thread safety, a stable C
ABI, hard real-time bounds or hardware rendering correctness.

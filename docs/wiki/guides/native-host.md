---
title: Native Host Integration
summary: A renderer-free embedding contract for host-owned frame timing, input, resources, scene lifetime, and render-feed consumption.
tags: [guide, embedding, native, runtime, extraction]
updated: 2026-09-30
related:
  - ../architecture.md
  - ../seam-status.md
  - ../subsystems/execution-context.md
  - ../subsystems/extract.md
---

# Native host integration

The boundary is an **in-process C++20 library**: include `x3d/sdk.hpp` and link
`x3d_cpp::sdk` from `find_package(x3d_cpp CONFIG REQUIRED)`. Java, a browser,
windowing, a display server, and a graphics device are not required. The host
owns its Vulkan instance/device/queues, swapchains or direct-display outputs,
tracking devices, frame pacing, per-eye projection, synchronization, and GPU
resource retirement. None is created or driven by `RuntimeSession`.

`examples/embed_minimal/native_host.cpp` is the executable contract. It runs a
CPU mirror of the actual render delta stream through the **installed package**,
including a moved install prefix. The named host configuration is a single-threaded
C++ runtime with standard behaviors, host-controlled resources, static AoS meshes
and Phong materials in the fixture; it is not a Full-profile browser claim. It also runs as `x3d_native_host_contract` in
the in-tree behavior/sanitizer suite. This demonstrates the library boundary;
it is not a Vulkan renderer, a CAVEOS adapter, or hardware validation.

Build and run the installed consumer from the repository root:

```sh
cmake -S . -B build -G Ninja -DX3D_CPP_PER_HEADER_CHECKS=OFF
cmake --build build --target x3d_cpp_runtime
cmake --install build --prefix "$PWD/install"
cmake -S examples/embed_minimal -B build-host -G Ninja -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build build-host --target x3d_embed_native_host
./build-host/x3d_embed_native_host
```

## One runtime owner, one tick owner

`RuntimeSession::create(document, options)` owns the document, execution context,
and extractor, in lifetime-safe destruction order. Its `unique_ptr` can move;
the session itself cannot. Construction performs `buildSceneGraph`, `buildFrom`,
and (by default) `attachStandardRuntime`. That last step is required to drive
authored TimeSensor/interpolator ROUTEs; resolving ROUTEs alone is insufficient.
`interactive` defaults to false so pointer navigation is not silently installed.
Scripts, physics, audio, and other optional backends require explicit wiring.
A host that runs scene-authored shaders sets `meshOptions.shaders`: a
`validator` that compiles each candidate with the host's own compiler (so an
invalid program falls through to the next shader or the material) and a
`resolver` for ShaderPart urls. The session gives both the extractor and the
shader event system the same options.

Drive a session from one host thread:

1. Feed the host's current inputs and tracking pose
2. Tick with simulation time in seconds
3. Consume `delta()` before the next tick, copying needed descriptor fields
4. Submit the host's own renderer work, with its own per-eye view/projection

A paused clock may repeat a timestamp; `tickGeneration()` still advances. The
SDK neither sleeps nor acquires a display. The host chooses simulation time and
its relationship to presentation time. It must avoid concurrent mutation,
extraction, or reentrant calls through callbacks. Callback execution is
synchronous on the calling thread; a slow callback can block it.

If simulation takes several substeps per presented frame, consume and apply a
delta after **each** tick. The dirty set holds one tick only. If a delta was
missed, recover with an authoritative full snapshot rather than expecting a
later delta to replay history.

### Input and view conventions

`context().setHeadPose(position, orientation)` composes relative tracking after
the bound Viewpoint's authored pose and navigation offset. Without a bound
Viewpoint, this relative mode returns an identity view matrix.

When your host owns the camera absolutely, pass its rigid camera-to-world
matrix directly:

```cpp
session->context().setViewerWorldPose(worldFromHead);
session->tick(now);
auto authored = session->context().boundViewpointWorldPose();
// Return camera control to the bound viewpoint and stored relative inputs:
session->context().clearViewerWorldPose();
```

The override works without a viewpoint and stays active across binds and
`setHeadPose` calls until cleared. Supply it before ticking view-dependent
systems. The authored query excludes navigation, head tracking, and the
absolute override; it returns identity if no viewpoint is bound. See
[the pose contract](../decisions/0065-absolute-viewer-world-pose.md).

`setPointer` accepts a **world-space ray**. Unprojection and device mapping belong
to the host. `setPointerButton`, `setPointerPresent`, `setPointerScreen`, `setKey`,
and the `pushKeyCharacter`/action/modifier/string methods accept mapped input;
setting a pointer alone does not install pointing or navigation systems. An
application that wants those can opt into `interactive`, or explicitly wire
its chosen systems. The runtime models one effective viewer/input state per
context; multi-user arbitration belongs to the host.

Meshes are in local X3D coordinates; each placement carries a column-major
`worldTransform`. Upload a shared mesh once and apply the placement transform.
The host owns Vulkan clip-depth conventions, viewport orientation, calibrated
screen geometry and asymmetric per-eye projections. Do not assume an example
OpenGL projection is a Vulkan/direct-display integration contract.

## Mutation is not ownership

Owning the scene safely does not make every mutation safe. `scene()`,
`document()`, `context()`, and `extractor()` remain low-level escape hatches.
Raw generated setters bypass the context's dirty tracking and runtime wiring.

The example queues host writes and applies them with `context.writeField`
inside a host `System::update`. It checks every `FieldWriteResult`. A System can
run more than once during cascade settlement, so consume its command queue
once. Calling `writeField` immediately before `tick` is not this protocol:
`tick` clears the previous dirty set first. For actual X3D input events,
`postEvent(node, "set_translation", value)` between ticks queues delivery into
the next cascade. It is not a checked generic write API.

A field write does not automatically enroll an arbitrary newly attached
behavior node, resolve new ROUTEs, repair DEF tables, or rebuild every subsystem.
`RuntimeSession` has no transactional edit/reload method. For whole-scene or
behavior-topology changes, prepare a **new session**, validate it, then swap at a
host frame boundary. Do not replace `session.scene()` underneath live systems.
If a callback throws midway through tick/extraction, do not assume rollback;
discard/recreate the candidate or recover according to a host-owned policy.

Structural SFNode/MFNode edits and `visible` changes take an authoritative
remove-all/add-all extraction baseline in the next delta. Ordinary scalar
geometry/material/TRS changes remain incremental.

Switch `whichChoice` and per-placement LOD selection changes retain unaffected
`RenderItemId`s and immutable mesh payloads. Keep calling
`session.tick(now); auto changes = session.delta();` and apply removals before
additions. Only placements entering or leaving the active selection appear in
those lists. If selection changes scoped light/fog indices, records whose indices
change also appear in both lists, with the same ID. External packed payloads are
re-resolved and surviving packed placements receive `updatedGeometry`.
Node-reference edits in the same tick still require the structural baseline.
Selection refresh uses a bounded active-scene walk; it does not promise CPU cost
proportional only to the changed subtree. See [ADR-0069](../decisions/0069-selection-deltas-and-cheap-ticks.md).

`lightsChanged` compares the complete world-resolved light list with the last
published list, including scope, attenuation, radius, and shadow controls. An
idle tick or unrelated animation leaves it false. Full snapshots set it true for
initialization. Camera, background, and fog notifications remain conservative.
You can remove a host value-comparison workaround used solely to suppress
unchanged light uploads. `snapshotLights()` still owns the indices referenced by
items; scalar light edits require a new snapshot if you need refreshed indices.

### Measure runtime phases

Build the `x3d_runtime_tick_profile` target with `X3D_CPP_BUILD_EXAMPLES=ON`.
Run it with a scene filename and a positive tick count:

```sh
./build/x3d_runtime_tick_profile /path/to/scene.x3d 91
```

The probe parses, creates a standard `RuntimeSession`, takes a full snapshot,
and advances from zero at 30 Hz with a fixed authored viewer and no pointer.
It prints parse, session, and snapshot milliseconds, then CSV rows separating
systems/cascade, post-cascade hooks, transforms, bounds, total tick, and delta.
It also prints placement/update counts and `lightsChanged`, and fails if the
extraction walk budget is exhausted. Use identical build settings for comparisons;
phase timing adds clock reads and is not a hard real-time guarantee. The optional
`sdk::X3DExecutionContext::TickTimings` overload is used as
`session.context().tick(now, &timings)`; ordinary `tick(now)` reads no clocks.

For the CAVEOS LOD probe scene, run:

```sh
./build/x3d_runtime_tick_profile /home/ben/code/godot-x3d/thirdparty/x3d-examples/www.web3d.org/x3d/content/examples/ConformanceNist/SpecialGroups/LOD/bound_500.x3d 91
```

## Consume changes and own lifetime

`RenderDelta` names placements; it does not own full render payloads. Read
`extractor.item(id)` while the extractor is current and copy what is needed.
References into the extractor are borrowed: conservatively invalidate them at
any tick, extraction, rebuild, or session destruction.

- Apply `removed` before `added`; a rebuild can use the same integer in both.
  Evict a content-cache entry when its last live placement is removed before
  accepting new content, even if its old GPU allocation awaits retirement
- Apply all update channels to live records, including skin pose if supported
- Honor camera, light, background and fog flags; a static test is not full
  renderer coverage
- A fresh `fullSnapshot()` means **replace** the host's live placement set,
  not append to it. It rebuilds extraction caches and restarts dense IDs
- Do not iterate `itemCount()` as the live set; incremental removals can leave
  old slots. Track additions/removals in the host
- On unload, drop borrowed references and all ID-indexed lookup tables

`MeshRef` (`shared_ptr<const MeshData>`) and `TexturePixelsRef` own immutable CPU
payloads. Retaining these handles preserves those bytes across replacement or
session destruction. Copying an entire descriptor does **not** make its raw
node/path/light/skin identities owning or portable. Keeping a node `shared_ptr`
alive also does not keep its former session or runtime callbacks valid: do not
write through retained runtime-bound nodes after unload. A GPU upload or draw may
outlive the CPU frame: hold required payloads and defer freeing GPU resources
until the host's own fences/timeline values permit it.

### Identity is local, not a transport

`RenderItemId` is an extractor-local dense integer. `PathKey` is a vector of raw
node pointers. `GeomId` combines a raw node pointer and a content version.
These are not UUIDs, persistent asset IDs, globally unique handles, or a wire
format. Separate placements can share geometry while having distinct paths and
transforms. Pointer reuse, session replacement and snapshot/rebuild boundaries
must not alias stale host resources.

Keep a **host-owned session/baseline epoch** and frame serial if buffering work
across frames. The example increments its epoch on replacement/unload. Scope
all SDK identities to that baseline; clear caches conservatively on rebuild.
A mesh cache may use immutable payload ownership to distinguish extracted mesh
variants. Do not rely on a bare geometry-node address as an eternal GPU key.

For out-of-process or distributed CAVE rendering, explicitly translate copied
values/payloads into a separately versioned host protocol with its own IDs,
ordering, recovery, and ownership. This example deliberately defines none.
CAVEOS headers/source are needed before deciding whether any public SDK
identity addition is necessary.

## Resources and errors are explicit

For host-controlled resources, parse **memory** with both EXTERNPROTO and Inline
resolver callbacks supplied explicitly. The default callbacks of
`parseDocument`, as well as `parseFile`, can read local files. Empty callbacks
are not a blanket permission policy; the example supplies callable deny
resolvers that return null. A real host can inject its own bounded, confined
resource cache, preserving URL fallback order and base-URL meaning.

Parse-time callbacks must resolve synchronously or fail. Session options expose
separate `assetResolver` (for LoadSensor), `textureResolver`, runtime
`inlineResolver`, and `baseUrl`; configuring one is not a universal fetch policy
for every resource type. Defaults do not fetch images or open devices. A
texture callback receives authored URL strings; resolve their base/path and
allowlisted schemes in the host.

`Pending` does not create a background job. Ready texture results are memoized;
Pending/Failed results are not, but an unchanged material is not necessarily
re-extracted by a later delta. Arrange an explicit material refresh or use a
full snapshot after asynchronous completion, and replace host state accordingly.
Likewise, changing bytes behind an unchanged Ready URL needs explicit cache
invalidation/rebaselining. The host owns timeouts, cancellation and diagnostics.

Catch parse/create exceptions before replacing an active session. Inspect
`rangeWarnings`, `protoWarnings`, `session.routes().rejected`,
`extractor.skippedGeometryCounts()`, and `extractor.budgetExceeded()`; a parsed
scene is not proof that every feature is executable or renderable. Check
`FieldWriteResult` rather than discarding it. `routes().rejected` reports known
endpoint validation errors, but unresolved DEF/IMPORT endpoints are silently
skipped by `buildFrom`; an empty rejection list is not proof that every authored
ROUTE was bound. The example pins its expected `routesAdded` count; production
hosts need a corresponding expected-feature/route validation policy.

The example rejects
`budgetExceeded()` before installing partial authoritative state, invalidates
its delta baseline, and requires a successful fresh snapshot (after fixing the
budget/content policy) before it resumes deltas. Do not accept the next empty
delta as recovery: extraction has already advanced its own baseline. The host
mirror also marks itself invalid before mutation, so a copy/allocation/check
exception cannot publish a partial frame as valid. The caller must not present
invalid state, and must accept a fresh successful snapshot before resuming.
Unsupported or denied resources
need a visible host policy (fallback, rejection, diagnostics), not a silent
claim of complete X3D support.

## Validation and limits

The render-feed surface is **EXPERIMENTAL**, consistently with the seam-status
matrix. The executable checks only its stated static-geometry/material/input
fixture and lifecycle paths. It does not establish full X3D conformance,
thread safety, a stable C ABI, binary serialization, a device backend, hard
real-time bounds, Vulkan rendering correctness, display timing, or CAVEOS
integration. Pin an exact SDK revision while integrating, and add application
fixtures against the actual host boundary.

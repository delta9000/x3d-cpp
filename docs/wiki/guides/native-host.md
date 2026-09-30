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

`context().setHeadPose(position, orientation)` composes the tracked pose after
the bound Viewpoint's authored pose and per-viewpoint navigation offset. A bound
Viewpoint is required; without one, `viewMatrix()` is identity. The tracked pose
is relative to that composed frame, not an arbitrary absolute world-camera
matrix. Supply a consistent viewer pose before ticking view-dependent systems.

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

Structural SFNode/MFNode and active-child edits now take an authoritative
remove-all/add-all extraction baseline in the next delta. This is a bounded
full-scene walk, not an incremental subtree-cost promise. Ordinary scalar
geometry/material/TRS changes remain incremental. **Switch/LOD activation changes
also use this path**: head movement around an LOD boundary can cause full
extraction and upload churn on successive frames. The example verifies TRS-only
updates retain their mesh payload, whereas structural replacement reconstructs
even unchanged meshes. Measure representative scenes on the actual host before
setting frame budgets; no hardware throughput or hard real-time bound is claimed.
The host does not need to
mark extraction topology itself for these supported writes.

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

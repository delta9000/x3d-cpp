# Experimental native SAI provider

This optional companion implements the same final
`x3d::sai::experimental::provider::service` used by the reference backend.
`native::make_service()` composes that service with an owned native backend.
Setup and live execution use identical scene/context/node/field handles.

The backend owns one native document per scene and transfers that document into
an owned `RuntimeSession` at activation. Values, containment, roots, DEF names,
ROUTEs, interpolation, world transforms and render extraction are native state.
The real native interpolator systems attach to the owned node registry, including
unnamed detached nodes; root occurrence lists stay unchanged. For setup-authored
scenes, other standard runtime systems are not enabled because their types are
outside this slice. Adopted sessions retain the systems the host prepared.
Activation closes setup for every provider-owned node in this bounded contract.
Detached nodes remain absent from render roots while supported live inputs and
ROUTEs stay valid. A future per-node realization model is outside this slice.
There is no mirrored reference scene, hidden event loop, clock, renderer or I/O.

## Build and use

Use the current x3d-sai provider contract with targets `x3d::sai` and
`x3d::sai_reference_provider`. Older checkouts exposing only `x3d::sai_metadata`
or separate offline/hosted frontends are incompatible. Keep both repository
revisions with your build; this experimental source contract has no stable ABI.

From the x3d-cpp root, build each package into one prefix with the same compiler:

```sh
sai_source=/path/to/x3d-sai
sai_prefix="$PWD/install-sai"
cmake -S "$sai_source" -B build/sai-package -G Ninja \
  -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX="$sai_prefix"
cmake --build build/sai-package --parallel 2
cmake --install build/sai-package
cmake -S . -B build/native-package -G Ninja \
  -DX3D_CPP_BUILD_TESTS=OFF -DX3D_CPP_ENABLE_DEV_TOOLING=OFF \
  -DCMAKE_INSTALL_PREFIX="$sai_prefix"
cmake --build build/native-package --parallel 2
cmake --install build/native-package
cmake -S adapters/sai_provider -B build/sai-adapter -G Ninja \
  -DCMAKE_PREFIX_PATH="$sai_prefix" -DCMAKE_INSTALL_PREFIX="$sai_prefix"
cmake --build build/sai-adapter --parallel 2
cmake --install build/sai-adapter
```

An installed application links one companion target:

```cmake
find_package(x3d_cpp_sai CONFIG REQUIRED)
target_link_libraries(my_host PRIVATE x3d_cpp::sai_provider)
```

Include `<x3d/sai_provider.hpp>` and call `native::make_service()`; ordinary work
then uses `provider::service`. The [host example](../../examples/sai_host/README.md)
shows a complete run and an alternative source build using explicit paths to
both checkouts. It also demonstrates the optional native presentation feed.
For backend-independent lifecycle rules, use the SAI
[portable contract](https://github.com/delta9000/x3d-sai/blob/main/docs/portable-contract.md).

## Adopt a host session

`native::adopt_session(std::move(session), limits)` returns an owning `service`,
`scene`, `active` activation, and checked `host` access to your existing session.
The session must be exclusively owned, quiescent, and unticked; relinquish mutable
node/store aliases. Construct the service on its eventual owner thread.

Use ordinary named-node lookup, field read, enqueue, and pump on the loaded graph.
`host.session(service)` borrows the same native session for host tracking inputs
and extraction between pumps. The host owns the sole `fullSnapshot`/`delta`
consumer; the provider performs neither extraction operation for adopted scenes.
Don't retain native borrows across pumps or retirement, mutate fields, or tick
outside the provider. Retire/close the service on its owner thread.

Adopted scalar access extends beyond the creation whitelist. SFNode/MFNode reads
preserve identity; node-valued writes remain refused. PROTO/Inline/IMPORT/Script
interfaces remain opaque while native content continues to execute and render.
Portable META projection is unsupported. Live ROUTE addition is available
through the optional `live_routes` capability: enqueue a route with
`service.enqueue_route(client, from, to)` and process its ordinary FIFO ticket.
Its pump installs the route before evaluating that turn; update gates install
all their routes before inputs. Removal and PROTO interface redirection remain
unsupported. See the
[adoption contract](../../docs/wiki/subsystems/sai-provider.md#adopt-a-prepared-session)
for the exact kind set and restrictions, and
[ADR-0065](../../docs/wiki/decisions/0074-sai-adopt-session.md) for ownership and
ingress decisions. `sai_provider_adopt_session` is registered in the source and
relocated provider harness, including same-pump TimeSensor and live ROUTE
assertions. The new live ROUTE assertions have not been run for this slice.

## Bounded capabilities

- Transform: seven inputOutput scalars, setup bounding-box fields, children and
  metadata; addChildren/removeChildren are explicitly unsupported
- PositionInterpolator: key, keyValue, set_fraction, value_changed and metadata
- Shape: geometry and metadata; Box: size, solid and metadata
- Setup field kinds: SFBool, SFFloat, SFVec3f, SFRotation, SFNode, MFNode,
  MFFloat and MFVec3f
- Generic setup creation, ordered root occurrences, names and ROUTEs; live host
  turns, owner-bearing node values, indexed writes, user data and notifications

The capability node list describes constructible types. Field descriptors
report actual access; the field-kind list also includes adopted scalar services. A type
unknown to the native factory returns unknown_type; a known type beyond this
slice returns unsupported_operation. Public SFNode/MFNode values contain
owner-bearing handles. Numeric IDs exist only inside backend dispatch.

The coupled PositionInterpolator policy requires nonempty finite, strictly
increasing keys with matching finite positions and finite adjacent float
differences. It is an explicit bounded proof restriction, not a complete
component/profile or ISO conformance claim.

Graph admission reconstructs supported node-valued fields from all registered
native nodes, including unnamed detached components. It rejects foreign native
pointers, repeated non-NULL nodes in one MFNode list and containment cycles.
Accepted native node interfaces are also enforced: metadata requires
X3DMetadataObject, Shape.geometry requires X3DGeometryNode and Transform.children
requires X3DChildNode. Candidate replacement may repair its own invalid native field but cannot hide
another invalid component. Traversal is iterative.

A released update gate produces one host submission. Stateful writes coalesce;
ordered inputOnly occurrences, including identical repeats, remain in that
submission. The execution context installs the complete retained inputOutput batch before
systems, input handlers, and observations. A valid final graph can therefore
repair a temporarily cyclic prefix. Later routed and inputOnly occurrences
retain per-occurrence delivery. All inputs run in one native tick. Output-event and ROUTE
cardinality follows the runtime's timestamp rules, independently of external
input occurrence counts. No synthetic epsilon time or per-input event reset is
introduced by the adapter.

API-authored ROUTEs use explicit weak native endpoint identity in Scene.routes,
including unnamed nodes. They do not fabricate DEF names. Name-bound routes
created by parsers retain their original resolution behavior. Direct unnamed
routes are runtime authoring objects: codec serialization and PROTO-clone
endpoint remapping are not supported by this provider extension. An expired
weak direct endpoint is never rebound through a reused DEF name.

Setup can propagate allocation failures. Native node registration rolls back an
incomplete identity entry, and name registration prepares its owning string
before publishing the lookup binding. A retry cannot inherit a failed node's
identity or give one node two canonical DEF names. This is consistency of those
registrations, not a general rollback guarantee for all setup operations. The
isolated `sai_provider_allocation_failure` executable injects allocation failures
through the ordinary service interface.

## Native extension and validation

`native::backend::native_scene(storage_id)` is an explicit setup-only native
inspection extension, with owner-thread checks. Retained native storage cannot
retain portable handle authority. A retained setup Scene view is moved-from at
activation and is not a view of the live session. `inspect(storage_id)` is also
owner-thread-affine and returns copied runtime,
interpolation, world-transform and render-delta evidence; neither extension is
part of normative SAI. Applications use the final portable service for ordinary
work, not an additional native general-purpose service API.

Run `mise run sai-provider /path/to/x3d-sai /empty/work/directory` from the
x3d-cpp root (the task invokes `scripts/verify_sai_provider.sh`).
Set X3D_CPP_SHARED_NODES=ON or OFF for shared/static native node packages.
The harness checks common authoring/live conformance, native storage/containment
and runtime evidence, cross-backend report parity, reference-free native link
commands, source builds and relocated installed consumers.

## Owned presentation feed

`<x3d/sai_presentation.hpp>` adds the native-only `make_presented_scene()`
factory. It returns the ordinary portable service, one ordinary setup scene,
and a feed bound to that exact scene and service. Continue to create nodes,
author fields, activate, enqueue, pump, drain callbacks and close through the
portable service. Other scenes created on that service do not acquire a feed.
The portable contract and existing `make_service()` factory are unchanged.

`feed.snapshot(service)` explicitly pulls the latest complete immutable frame.
It checks the exact service, lifetime and active frontend state, including a
frontend fault after native execution completed. Setup, wrong-thread, retired,
closed and foreign-service access fail. No presentation callback runs in a
native turn. The activation frame has no host time; later frames record the
native tick and supplied time. A rejected submission that never enters the
backend does not create a native frame.

A frame owns its item vector, matrices, extracted `MaterialDesc` values, and
shared immutable mesh data. With no Appearance in the supported node slice,
the material is the extractor's unlit-white fallback. You no longer need to
invent a host material for these placements. The frame
contains no native node pointers, storage IDs, scene/session storage or callback
captures. Its opaque scene identity namespaces placement and mesh keys. Host
code may retain and render an old frame after the service is destroyed; that
does not retain authority to operate on its expired scene handle.

Placement keys survive unchanged paths across extractor rebuilds. Removed paths
are forgotten, and a later re-addition receives a new key. Mesh keys identify
currently live immutable mesh ownership, so shared placements share a key;
equivalent freshly rebuilt mesh content may receive another key. Neither key
space is a portable node handle. Identity exhaustion fails explicitly.

Use `changes_between(previous.get(), *current)` to compare two retained frames
from the same scene. Pass `nullptr` for the first upload. The result contains
`added`, `removed`, `updated_transform`, `updated_geometry`, and
`updated_material` placement keys. Read added and updated values from
`current.items`, and discard removed keys. Update buckets can overlap, but
exclude added and removed placements. Ordering within a bucket is unspecified.
The function rejects foreign scenes and decreasing native ticks; equal host
times and skipped ticks are supported. It needs no live service.

Transform changes compare exact matrix values; geometry changes compare mesh
identity. Each item's opaque `material_revision` advances on extractor material
invalidation or placement rebuild, so `updated_material` can conservatively
request a refresh of an equal value. Revisions survive unchanged turns and
cannot be compared across scene identities. This is a net frame comparison,
not a log of intermediate edits. See
[ADR-0066](../../docs/wiki/decisions/0066-sai-presentation-materials-and-deltas.md).

This retains the latest-full-snapshot policy: the backend keeps one current frame and
current identity maps, with no historical frame queue. Slow hosts deliberately
skip intermediate presentation frames and replace their displayed scene from
the next complete frame. Portable event/receipt queues retain their separate
policies. Producer retention scales with the current scene; consumer-retained
frames and backend scene storage are outside portable queue byte budgets.

The feed exposes geometry, materials, and world transforms for the four-node slice.
Its configuration preflight conservatively limits every registered node path,
including detached nodes, to the native nesting bound (currently 1,000 nodes).
This prevents the extractor's silent depth truncation from becoming an alleged
complete frame. If extraction exhausts its separate visit budget after native
execution starts, capture fails, the activation faults, and the feed rejects
that candidate; previously retained complete frames remain valid.
Camera, flat shading, rasterization and presentation belong to the host. It does
not represent a complete renderer, profile, material system or CAVEOS binding.
The existing extractor distinguishes shared shapes under different Transform
paths, but collapses repeated identical root/path occurrences. This extension
preserves that behavior and does not claim occurrence-distinct rendering there.

## Scene information

The adapter advertises `scene_units` and `scene_metadata` for the canonical
provider. Effective unit discovery includes SI defaults. Setup declarations are
stored in native `X3DDocument::head.units` and `Scene::sourceUnits` together, before
node creation. Ordered scene META entries live in the document head and survive
activation through `RuntimeSession::document()`.

Provider field inputs and outputs always use canonical SI values. Authored scalar
fields are marked already normalized, including detached nodes; declarations do
not trigger a second conversion on activation. Generated defaults and live writes
remain canonical. The shared provider fixture and native-runtime fixture cover
nonidentity length/angle declarations, metadata, defaults, interpolation and
live values. This introduces no parsed-document importer or serializer: a future
writer must inverse-convert canonical values or adjust UNIT declarations before
serialization. Metadata writes are immediate and setup-only, not the standard
live per-client buffered service. Duplicate document META projection, optional
XML META attributes and complete ISO unit/codec fidelity remain outside the slice.

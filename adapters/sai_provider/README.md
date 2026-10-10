# Experimental native SAI provider

This optional companion implements the same final
`x3d::sai::experimental::provider::service` used by the reference backend.
`native::make_service()` composes that service with an owned native backend.
Setup and live execution use identical scene/context/node/field handles.

The backend owns one native document per scene and transfers that document into
an owned `RuntimeSession` at activation. Values, containment, roots, DEF names,
ROUTEs, interpolation, world transforms and render extraction are native state.
The real native interpolator systems attach to the owned node registry, including
unnamed detached nodes; root occurrence lists stay unchanged. Other standard
runtime systems are not enabled because their node types are outside this slice.
Activation closes setup for every provider-owned node in this bounded contract.
Detached nodes remain absent from render roots while supported live inputs and
ROUTEs stay valid. A future per-node realization model is outside this slice.
There is no mirrored reference scene, hidden event loop, clock, renderer or I/O.

## Bounded capabilities

- Transform: seven inputOutput scalars, setup bounding-box fields, children and
  metadata; addChildren/removeChildren are explicitly unsupported
- PositionInterpolator: key, keyValue, set_fraction, value_changed and metadata
- Shape: geometry and metadata; Box: size, solid and metadata
- Eight field kinds: SFBool, SFFloat, SFVec3f, SFRotation, SFNode, MFNode,
  MFFloat and MFVec3f
- Generic setup creation, ordered root occurrences, names and ROUTEs; live host
  turns, owner-bearing node values, indexed writes, user data and notifications

`service.supported()` and field descriptors report these restrictions. A type
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
submission. The complete retained inputOutput seed prefix is stored before
its source notifications are read or published, so a valid final graph can
repair a temporarily cyclic prefix. Later routed and inputOnly occurrences
retain per-occurrence validation and delivery. This relies on the bounded
backend installing only inputOnly interpolation handlers; it is not a blanket
guarantee for other native systems. All inputs run in one native tick. Output-event and ROUTE
cardinality follows the runtime's timestamp rules, independently of external
input occurrence counts. No synthetic epsilon time or per-input event reset is
introduced by the adapter.

API-authored ROUTEs use explicit weak native endpoint identity in Scene.routes,
including unnamed nodes. They do not fabricate DEF names. Name-bound routes
created by parsers retain their original resolution behavior. Direct unnamed
routes are runtime authoring objects: codec serialization and PROTO-clone
endpoint remapping are not supported by this provider extension. An expired
weak direct endpoint is never rebound through a reused DEF name.

## Native extension and validation

`native::backend::native_scene(storage_id)` is an explicit setup-only native
inspection extension, with owner-thread checks. Retained native storage cannot
retain portable handle authority. A retained setup Scene view is moved-from at
activation and is not a view of the live session. `inspect(storage_id)` is also
owner-thread-affine and returns copied runtime,
interpolation, world-transform and render-delta evidence; neither extension is
part of normative SAI. Applications use the final portable service for ordinary
work, not an additional native general-purpose service API.

Run `scripts/verify_sai_provider.sh /path/to/x3d-sai /empty/work/directory`.
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

A frame owns its item vector, matrices and shared immutable mesh data. It
contains no native node pointers, storage IDs, scene/session storage or callback
captures. Its opaque scene identity namespaces placement and mesh keys. Host
code may retain and render an old frame after the service is destroyed; that
does not retain authority to operate on its expired scene handle.

Placement keys survive unchanged paths across extractor rebuilds. Removed paths
are forgotten, and a later re-addition receives a new key. Mesh keys identify
currently live immutable mesh ownership, so shared placements share a key;
equivalent freshly rebuilt mesh content may receive another key. Neither key
space is a portable node handle. Identity exhaustion fails explicitly.

This is a latest-full-snapshot policy: the backend keeps one current frame and
current identity maps, with no historical frame queue. Slow hosts deliberately
skip intermediate presentation frames and replace their displayed scene from
the next complete frame. Portable event/receipt queues retain their separate
policies. Producer retention scales with the current scene; consumer-retained
frames and backend scene storage are outside portable queue byte budgets.

The feed exposes only geometry and world transforms for the four-node slice.
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

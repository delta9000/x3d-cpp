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
commands, source builds and relocated installed consumers. See
`tests/coverage-ledger.md` for assertion migration details.

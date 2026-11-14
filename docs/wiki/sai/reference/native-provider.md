---
title: Native SAI provider
summary: Reference for the native backend in adapters/sai_provider, including its supported slice, adopted-scene rules and native extensions.
tags: [sai, reference, provider, native]
updated: 2026-11-14
related:
  - ../how-to/use-the-native-backend.md
  - ../how-to/adopt-a-session.md
  - presentation-feed.md
  - ../../decisions/0074-sai-adopt-session.md
---

# Native SAI provider

The optional `adapters/sai_provider` package implements `provider::service`
over native scene storage. The backend owns a native `X3DDocument` during setup
and a `RuntimeSession` after activation. There is no second reference scene to
keep synchronized.

Guides: [Build and link the native backend](../how-to/use-the-native-backend.md),
[Adopt a loaded session](../how-to/adopt-a-session.md).

## Entry points

| Call | Header | Returns |
|---|---|---|
| `native::make_service(limits = {})` | `<x3d/sai_provider.hpp>` | The ordinary portable service |
| `native::make_presented_scene(limits = {})` | `<x3d/sai_presentation.hpp>` | Service, one setup scene and a [presentation feed](presentation-feed.md) |
| `native::adopt_session(std::move(session), limits = {})` | `<x3d/sai_provider.hpp>` | `adopted_scene { service, scene, active, host }` |
| `native::backend::native_scene(storage_id)` | `<x3d/sai_provider.hpp>` | Setup-only native inspection (see below) |

All names are in `x3d::sai::experimental::native`. The contract is
experimental with no stable ABI, so build the SDK, runtime and adapter from the
same x3d-cpp revision.

## Constructible slice

| Aspect | Support |
|---|---|
| Node types | Transform, Shape, Box, PositionInterpolator |
| Field kinds | `sf_bool`, `sf_float`, `sf_double`, `sf_time`, `sf_int32`, `sf_string`, `sf_color`, `sf_vec3f`, `sf_rotation`, `sf_node`, `mf_node`, `mf_float`, `mf_vec3f` |
| Backend name | `x3d-cpp native` |
| Services | Authoring, host turns, notifications, node values, indexed writes, user data, `scene_units`, `scene_metadata`, `live_routes` |

The capability node list describes *constructible* types. It does not limit
discovery in an adopted scene.

No portable PROTO, IMPORT, Inline, Script, geospatial, URL-loading or codec
service exists. Native runtime support for those features does not expose them
through this adapter.

## Execution model

- Each `pump` performs one native tick through
  `RuntimeSession::tick(now, ingress)`. Adopted and bounded scenes use the same
  call, so deferred Inline diagnostics drain into `document().inlineWarnings`.
- Retained candidates enter the execution context together before systems run.
- Input occurrences, routed changes and standard-system outputs share one
  tick's timestamp guards.
- Successful native field occurrences become owning observations. Application
  callbacks run only through explicit dispatch.
- A native execution failure can leave partial effects and faults the
  activation. No rollback is promised.
- Live runtime assertions use portable reads and notifications; the backend
  carries no fixture-specific inspection record.

## Adopted scenes

`adopt_session` transfers a prepared `RuntimeSession` into an owning service.
The decision record is
[ADR-0074](../../decisions/0074-sai-adopt-session.md).

### Preconditions

- The session is exclusively owned, quiescent and has never ticked.
- Mutable graph and author-store aliases have been relinquished. Moving a
  shallow document copy does not establish exclusive ownership.
- The service is constructed on its eventual owner thread, after the loader's
  exclusive session handoff.

### What survives

The original session, options, custom resolvers, native ROUTEs, document
annotations and extraction baseline.

### Host access

`host.session(service)` borrows the native session. It requires the exact
service, the owner thread, active frontend state and idle native callbacks.

| The host may | The host must not |
|---|---|
| Supply native tracking inputs between pumps | Tick the session itself |
| Own the sole `fullSnapshot()`/`delta()` extraction cursor | Mutate fields or change topology natively |
| Read `extractor()` results after a pump | Keep native borrows across a pump or retirement |

The provider never calls `fullSnapshot()` or `delta()` for an adopted scene.
Retire or close the service on its owner thread; moving an active service to a
retirement worker is unsupported. Retirement revokes both portable handles and
checked host access.

### Identity registration

- Preserved: root order and repeated occurrences, DEF/USE sharing, detached
  root-context DEF bindings, resolved root ROUTE endpoints.
- Nodes that native code makes reachable after adoption (a Script ROUTE into
  `addChildren`, a host-side children edit, a new root) are registered on first
  discovery through a supported read, root list or DEF lookup.
- The registry retains each registered node for the scene's lifetime, so a
  native SFNode/MFNode write never faults a pump as a "foreign node".
- The field-write listener reads and records a write only when a live observer
  watches that exact field. Unobserved native writes are not read back.
- Adoption does not run the bounded authoring graph validator; its
  repeated-reference and containment rules would reject valid loaded graphs.

### Field access in an adopted scene

| Kind | Read | Write |
|---|---|---|
| SFBool, SFFloat, SFDouble, SFTime, SFInt32, SFString, SFColor, SFVec3f, SFRotation | Yes | Yes |
| SFEnum | As a string | Exact enum tokens only |
| SFNode, MFNode | Yes; NULL slots and sharing preserved | Refused |
| Other kinds | Unsupported | Unsupported |

| Access type | Rule |
|---|---|
| inputOnly | Unreadable |
| outputOnly | Unwritable |
| initializeOnly | Fixed at adoption |

Before scalar ingress, disposable native storage checks generated range
diagnostics and exact enum tokens. A custom type without a factory is refused
for writes. This is a bounded value service, not complete reflection or
component support.

### Opaque content

PROTO instances, Inline contexts, IMPORT endpoints and Script fields keep
executing and rendering, but are opaque to SAI.

| Query | Result on opaque content |
|---|---|
| `type_name` | The real type: the PROTO name, `Inline` or `Script` |
| `fields`, `get_field` | `unsupported_operation` |
| `named_nodes` | IMPORT aliases excluded |
| `named(alias)` | Refused |

Nested names and declaration templates are never flattened into the root
context.

### Metadata in an adopted scene

Adopted META stays in the native document. The adopted scene refuses portable
metadata projection with `unsupported_operation`. The service still advertises
`scene_metadata`, so other scenes created in the same service keep it.

## Live ROUTE restrictions

On top of the portable [live ROUTE](live-routes.md) rules, the native
`do_check_live_route` refuses, at `enqueue_route`:

- opaque PROTO, Inline, IMPORT and Script endpoints
- PROTO interface redirection
- cross-context routes
- native kind mismatches. SFEnum and SFString both map to portable
  `sf_string`, but an SFEnum to SFString route fails with `invalid_route`
- node-valued destinations, in every native scene. Adopted scenes keep the
  topology-write restriction, and bounded scenes would otherwise skip the
  containment and cycle preflight that admitted writes get

Installation repeats the check. The installer adds a direct weak-identity entry
to `Scene::routes` with canonical field names and installs the executable edge.
It prepares document storage before native graph mutation and removes that
entry if graph insertion throws or refuses a detached endpoint. No extraction
call is needed.

## Named nodes

The adapter reads the native scene's DEF bindings during setup, active and
faulted states. Hosts need no parallel name table. See
[Named nodes](named-nodes.md).

## Native inspection extension

`native::backend::native_scene(storage_id)` is an explicit setup-only
inspection extension with owner-thread checks.

- Retained native storage cannot retain portable handle authority.
- A retained setup `Scene` view is moved-from at activation and is not a view
  of the live session.
- Applications read live fields and observe notifications through the portable
  service.

## Evidence

| Test | Covers |
|---|---|
| `sai_provider_native` | Shared authoring and live oracle plus native scalar, metadata and graph evidence |
| `sai_provider_native_runtime` | Shared live oracle, named-node oracle, and native session, interpolation, ROUTE, world-transform, RenderDelta and failure-effect proofs |
| `sai_provider_parity` | Exact comparison of complete backend-blind reports from both backends |
| `sai_provider_adopt_session` | Shared Sphere placements, DEF identity, detached names, routed interpolation, deferred observation, Material extraction, access, range and enum refusals, opaque content and real type names, per-scene metadata refusal, owner-thread access, retirement |
| `native_node_writes_after_adoption` | Nodes added natively (observed and unobserved `addChildren`) and read back through SAI |
| `inline_diagnostics_drain_through_pump` | Inline warning drain |
| `same_pump_time_sensor_ingress` | A new cycle interval governs the first fraction emitted in that same pump |
| `sai_provider_allocation_failure` | Native registration stays consistent after allocation failure |

How these run is in [Test suites and gates](test-suites.md).

## Key sources

`adapters/sai_provider/src/sai_provider.cpp`,
`adapters/sai_provider/include/x3d/sai_provider.hpp`,
`adapters/sai_provider/include/x3d/sai_presentation.hpp`,
`adapters/sai_provider/tests/`, `tests/cmake/sai_provider/`,
`tests/cmake/sai_coexistence/`, `scripts/verify_sai_provider.sh` and
`scripts/verify_sai_coexistence.sh`.

---
title: Native SAI provider
summary: Compose the portable SAI service with owned native scene state and an explicit host loop.
tags: [subsystem, sai, provider, ownership]
updated: 2026-10-18
related:
  - system-script-sai.md
  - execution-context.md
  - ../decisions/0047-sai-sister-repo-split.md
---

# Native SAI provider

The optional `adapters/sai_provider` package implements x3d-sai's portable
`provider::service` using native scene storage. The service owns handle authority,
admission, receipts and notification delivery. Its backend owns a native
`X3DDocument` during setup and a `RuntimeSession` after activation. There is no
second reference scene to keep synchronized.

## Choose the package

Both backends use the same ordinary service and handles. Link the backend you
need; reference-only editing mechanisms are optional extensions.

| Consumer need | CMake target | Role |
|---|---|---|
| Implement or use the portable contract | `x3d::sai` | Frontend from the independent x3d-sai package |
| Use the reference backend | `x3d::sai_reference_provider` | Reference storage behind that frontend |
| Use native runtime execution | `x3d_cpp::sai_provider` | This adapter; `find_package(x3d_cpp_sai CONFIG REQUIRED)` |
| Use reference snapshots/edit history | `x3d::sai_reference_kernel`, `x3d::sai_reference_metadata` | Explicit reference extensions, not required by the native backend |

`native::make_service()` returns the ordinary portable service.
`native::make_presented_scene()` additionally creates one scene with an immutable
mesh/material/transform feed. The [ordinary host example](https://github.com/delta9000/x3d-cpp/tree/main/examples/sai_host)
shows source and installed consumption, backpressure, explicit time, callbacks
and shutdown without a test-fixture API. The native provider target supplies its
own dependencies; ordinary SDK builds remain independent of x3d-sai.

## Adopt a prepared session

`native::adopt_session(std::move(session), limits)` transfers a prepared native
`RuntimeSession` into an owning portable service. It returns `service`, `scene`,
`active`, and checked `host` access; no setup state escapes. Include
`<x3d/sdk.hpp>` for native runtime operations and `<x3d/sai_provider.hpp>` for
adoption. You must relinquish mutable graph and author-store aliases and supply
a quiescent session that has never ticked. Moving a shallow document copy does
not establish exclusive ownership.

```cpp
auto adopted = take(native::adopt_session(std::move(session)));
auto &service = *adopted.service;
auto context = take(service.root_context(adopted.scene));
auto paint = take(service.named(context, "Paint"));
auto color = take(service.get_field(paint, "diffuseColor"));
auto client = take(service.connect(adopted.active));
take(service.enqueue(client, {{color, sai::color3f{0.8f, 0.1f, 0.3f}}}));
auto report = take(service.pump(adopted.active, {now}));
// Check report.error, report.state, and request receipts before rendering.
auto *runtime = take(adopted.host.session(service));
auto changes = runtime->delta();
// Consume changes through runtime->extractor(), then drain SAI observations.
take(service.dispatch_notifications());
```

Here `take` unwraps a result or reports its error. The original session, options,
custom resolvers, native ROUTEs, document annotations, and extraction baseline
survive. The provider never calls `fullSnapshot()` or `delta()` for this scene.
You own the sole extraction cursor, including any loader-thread baseline. Each
pump performs one native tick. Supply native tracking inputs between pumps;
don't tick, mutate fields, change topology, or keep native borrows across a pump
or retirement. Checked access requires the exact service, owner thread, active
frontend state, and idle native callbacks. Construct the service on the eventual
owner thread after the loader's exclusive session handoff. Retire or close it
on that thread; moving an active service to a retirement worker is unsupported.

Identity registration preserves root order and repeated occurrences, DEF/USE
sharing, detached root-context DEF bindings, and resolved root ROUTE endpoints.
Nodes that native code makes reachable after adoption (a Script ROUTE into
`addChildren`, a host-side children edit, a new root) are registered on first
discovery through a supported read, root list, or DEF lookup, with the same
opacity rules. The registry retains each registered node for the scene's
lifetime. A native SFNode/MFNode write therefore never faults a pump as a
"foreign node". The field-write listener reads and records a write only when a
live observer watches that exact field.
Node-valued reads preserve NULL slots and sharing. Adoption does not run the
bounded authoring graph validator, whose repeated-reference and containment
rules would reject valid loaded graphs. PROTO instances, Inline contexts,
IMPORT endpoints, and Script fields are opaque: their native execution and
rendering remain intact, but SAI field discovery refuses them explicitly.
`type_name` still reports the real type: the PROTO name for a PROTO instance,
`Inline` and `Script` otherwise. Opacity shows up as the `unsupported_operation`
refusal from `fields`/`get_field`, not as the type.
IMPORT aliases are excluded from named-node enumeration and lookup refuses an
alias by name. Nested names and declaration templates are never flattened into
the root context. Adopted META remains in the native document; the adopted
scene refuses portable metadata projection with `unsupported_operation`. The
service still advertises `scene_metadata`, so other scenes created in the same
service keep it.

The capability node list describes **constructible** types: Transform, Shape,
Box, and PositionInterpolator. It does not limit adopted node discovery.
Per-field descriptors govern adopted access: SFBool, SFFloat, SFDouble, SFTime,
SFInt32, SFString, SFEnum (as a string), SFColor, SFVec3f, and SFRotation, plus
SFNode/MFNode reads. Other kinds are unsupported. Node-valued writes are refused;
inputOnly is unreadable, outputOnly is unwritable, and initializeOnly is fixed
at adoption. Before scalar ingress, disposable native storage checks generated
range diagnostics and exact enum tokens; a custom type without a factory is
refused for writes. This is a bounded value service, not complete reflection or
component support. Live ROUTE addition uses the separate optional capability below.

Retained candidates enter the execution context together before systems run.
Input occurrences, routed changes, and standard-system outputs share one tick's
timestamp guards. Native successful field occurrences become owning observations;
application callbacks run only through explicit dispatch. A native execution
failure can leave partial effects and faults the activation; no rollback is
promised. Retirement revokes both portable handles and checked host access.
Each pump ticks through `RuntimeSession::tick(now, ingress)`, so deferred Inline
diagnostics drain into `document().inlineWarnings` as they do for a host-driven
session. The bounded (non-adopted) backend uses the same call.

`sai_provider_adopt_session` covers shared Sphere placements, DEF identity,
detached names, routed interpolation, deferred observation, Material extraction,
access/range/enum refusals, opaque content and its real type names, per-scene
metadata refusal, owner-thread access, and retirement.
`native_node_writes_after_adoption` adds nodes natively (observed and
unobserved `addChildren`) and reads them back through SAI.
`inline_diagnostics_drain_through_pump` checks the Inline warning drain.
Its `same_pump_time_sensor_ingress` case checks that a new cycle interval governs
the first fraction emitted in that same pump. The separate provider harness
registers this test for source and relocated package consumers, and it passes
there against the x3d-sai checkout. See
[ADR-0074](../decisions/0074-sai-adopt-session.md).

## Add a live ROUTE

Check `service.supported().supports(provider::service_id::live_routes)`, then
submit `service.enqueue_route(client, from, to)`. It returns the ordinary ticket;
check enqueue errors, the pump report, and the terminal receipt. The original
setup-only `add_route` call is unchanged. The queued operation validates owner
thread, client and endpoint authority, lexical context, supported fields, exact
kind, and output-to-input direction.

The native provider installs the route before evaluating that FIFO turn and
performs one ordinary tick. Earlier queued events cannot traverse it, and
installation does not replay stored output. For a released update gate, routes
install in their relative order before all writes and input occurrences in the
gate, including writes admitted before the route call. A route ticket shares
cancellation, capacity accounting, and grouped outcomes with write tickets.

The native backend's `do_check_live_route` refuses opaque
PROTO/Inline/IMPORT/Script endpoints, PROTO interface redirection, cross-context
routes, native kind mismatches, and node-valued destinations. The frontend calls
it at `enqueue_route`, so these fail at admission rather than at pump. For
example, SFEnum and SFString both map to portable `sf_string`, but an SFEnum to
SFString route fails at enqueue with `invalid_route`. Installation repeats the
check. Node-valued destinations are refused in every native scene: adopted
scenes keep the topology-write restriction, and bounded scenes would otherwise
skip the containment and cycle preflight that admitted writes get.

An exact duplicate (same source node/field, same sink node/field) is a
successful no-op (ISO 19775-1 4.4.8.2). That covers a duplicate of an
installed route and a copy held in the same update gate. The frontend keeps
only the first copy of a route when it releases a gate, and the native installer
returns success without effect for a route already in the executable graph.
Setup-time `add_route` still refuses a duplicate with `invalid_route`. It adds a direct
weak-identity entry to `Scene::routes` and installs
the executable edge; field names are canonical. It prepares document storage
before native graph mutation and removes that entry if graph insertion throws
or refuses a detached endpoint. No extraction call is needed. The reference
backend implements the portable capability through its scene-edit transaction.

A refused route leaves existing topology and the activation intact. If a
submission already installed a route before a later operation fails, its effects
are partial: the activation faults and later work is cancelled. Route removal,
nested-context exposure, and PROTO redirection remain unsupported.

`live_routes_on_adopted_session` in `sai_provider_adopt_session` checks earlier
versus later FIFO events, same-submission installation, deferred observation,
admission-time refusals (including SFEnum to SFString), redundant duplicates
alone and in a gate, and document/executable agreement. The existing source and
relocated provider harness runs this test.
See [ADR-0075](../decisions/0075-sai-live-route-addition.md) and the portable
contract's queued live ROUTE section in the x3d-sai checkout.

## Discover named nodes

Use `service.named_nodes(context)` to list owned `name`/`target` pairs and
`service.names(node)` to format a node reference. Results use ascending,
case-sensitive name order. Enumeration includes detached named nodes and omits
unnamed nodes; reverse lookup succeeds with an empty vector for an unnamed node.
The existing one-DEF-name-per-node authoring rule is unchanged.

The adapter reads the native scene's DEF bindings during setup, active, and
faulted states. The portable frontend supplies authority checks and wraps native
IDs; hosts need no parallel name table. Retained results own their strings, but
node handles lose authority on retirement. These owner-thread operations are
covered by the shared `check_provider_named_nodes` oracle in
`sai_provider_native_runtime`; this proposal has not been built or tested.
See [ADR-0067](../decisions/0067-provider-named-node-discovery.md).

## Own the host loop

Create supported nodes, author fields and ROUTEs, then activate the scene. Live
clients enqueue work; the host calls `pump` with finite nondecreasing time and
separately drains notifications. Enqueue acceptance is not execution success:
inspect the owning receipt and the turn report's error, state and failure effect.
A failure after partial or unknown native effects faults the activation; do not
replay it automatically. Keep service mutation and shutdown on its owner thread.

Retained receipts and immutable presentation frames can outlive service closure.
They retain their results, not authority to mutate a retired scene. The full
[portable contract](https://github.com/delta9000/x3d-sai/blob/main/docs/portable-contract.md)
is the source for lifecycle and resource-budget rules. Script nodes still use
the separate [in-process SaiContext](system-script-sai.md); its runtime pointers
are not portable service handles.

## Supported scope and verification

Discover `service.supported()` and each field's supported flag. The native slice
covers Transform, PositionInterpolator, Shape and Box, with eight field kinds.
It has no portable PROTO, IMPORT, Inline, Script, geospatial, URL-loading or codec
service. Native runtime support for those features does not expose them through
this adapter. The contract is experimental and has no stable binary ABI claim.

The native feed keeps `snapshot(service)` for complete frames.
`changes_between(previous.get(), *current)` compares retained frames by placement
key, including skipped ticks; `nullptr` requests an initial upload. It returns
added, removed, transform, geometry, and material update buckets. The additive
`delta.items` map holds complete current payloads by placement key for the union
of added and updated placements. Each changed key appears once, even when its
geometry and material both change; unchanged and removed keys have no payload.
For example, upload `delta.items.at(key)` for an added or geometry/material
update key, or move using its `world_transform` for a transform-only update.
These are owned values with shared immutable meshes, so you can release the
input frames after comparison. `keyed_delta_payloads` in the existing
`sai_provider_presentation` test checks these contracts; it has not been run. Material values
come directly from extraction; the current slice has no Appearance authoring,
so it carries the SDK's unlit-white fallback. Material revisions track extractor
invalidations and can report equal values after a rebuild. Comparisons require
the same scene and nondecreasing native ticks, and remain valid after service
destruction. See [ADR-0066](../decisions/0066-sai-presentation-materials-and-deltas.md)
and the [feed contract](https://github.com/delta9000/x3d-cpp/tree/main/adapters/sai_provider#owned-presentation-feed).
The `sai_provider_presentation` test includes `materials_and_frame_deltas` for
this proposal; it requires the separate provider harness below.

From this checkout, pass an explicit x3d-sai source directory:

```sh
mise run sai-coexistence /path/to/x3d-sai
mise run sai-provider /path/to/x3d-sai
```

These opt-in checks fetch nothing. They exercise source consumers, move an
installed prefix, and rebuild consumers with package registries disabled.
`X3D_CPP_SHARED_NODES=OFF` selects static native nodes. The provider check also
compares reference/native behavior and runs the ordinary host. Builds and logs
remain in the printed work directory. Record both repository revisions with a
result; one repository's ordinary CI is not cross-repository evidence.

Key implementation and evidence: `adapters/sai_provider/src/sai_provider.cpp`,
`adapters/sai_provider/include/x3d/sai_provider.hpp`,
`adapters/sai_provider/include/x3d/sai_presentation.hpp`,
`adapters/sai_provider/tests/`, `tests/cmake/sai_provider/`,
`tests/cmake/sai_coexistence/`, `scripts/verify_sai_provider.sh` and
`scripts/verify_sai_coexistence.sh`.

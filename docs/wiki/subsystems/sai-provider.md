---
title: Native SAI provider
summary: Compose the portable SAI service with owned native scene state and an explicit host loop.
tags: [subsystem, sai, provider, ownership]
updated: 2026-10-10
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
See [ADR-0063](../decisions/0067-provider-named-node-discovery.md).

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
destruction. See [ADR-0063](../decisions/0066-sai-presentation-materials-and-deltas.md)
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

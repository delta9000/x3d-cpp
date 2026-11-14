---
title: SDK Façade
summary: The single public header consumers include — x3d::sdk — which re-exports the curated v1 embedder surface from namespace x3d::sdk.
tags: [subsystem, sdk, facade, public-api]
updated: 2026-10-18
related:
  - ../architecture.md
  - ../subsystems/cli-suite.md
  - ../decisions/0001-ext-firewall.md
  - ../guides/gate-system.md
---

# SDK façade

Include `x3d/sdk.hpp` and link `x3d_cpp::sdk`. The façade re-exports selected
runtime types into `x3d::sdk` and adds the header-only
[node inspection](#node-inspection) templates; it adds no compiled
implementation of its own.
The linked node and runtime libraries supply compiled definitions.

## Loading and execution

Use `parseFile` for local-file loading or `parseDocument` with explicit
resolvers for host-controlled loading. See [parse readers](parse-readers.md).

`RuntimeSession::create` owns a document, context, and extractor and attaches
the standard runtime. Call `fullSnapshot()` once, then `tick(now)` and
`delta()` from the host loop. Low-level `X3DExecutionContext` and
`SceneExtractor` remain available. See [execution context](execution-context.md)
and [extraction](extract.md) for lifetime, mutation, and delta contracts.

## Resource interfaces

Applications supply resource policy through these interfaces:

| Interface | Contract reference |
|---|---|
| AssetResolver | [Byte loading](system-asset-io.md) |
| TextureResolver | [Image decoding](system-texture-decode.md) |
| FontMetrics | [Glyph metrics](system-font-metrics.md) |
| ScriptEngine | [Script execution](system-script-sai.md) |
| GeoProjection | [Context-owned geospatial conversion](geospatial.md) |
| AudioBackend | [Sound](sound.md) |
| MovieDecoder | [Movie frames](extract-textures.md#movie-frame-decoding) |

`AssetResolverWithBase` and `TextureResolverWithBase` are experimental
variants that receive the requesting node's source base URL with the authored
URL. See [ADR-0063](../decisions/0063-resolver-base-urls.md).

Not every resource implementation is re-exported by `sdk.hpp`; optional
backend factories have their own headers and build options.

## Package boundaries

`x3d_cpp::authoring` and `x3d/authoring.hpp` provide nodes, the document model,
`SceneBuilder`, and writers without simulation or extraction. See
[asset import](asset-import.md#scenebuilder). `sdk.hpp` re-exports the builder as
`sdk::SceneBuilder`.

Extensions use `x3d/ext.hpp` and the separate opt-in extension target.
See [the extension firewall](ext-firewall.md).

## Node inspection

The experimental `sdk::forEachNode(scene, visit)` and
`sdk::readField<T>(node, name)` let a host discover nodes and read field values
without naming runtime helpers. See
[ADR-0071](../decisions/0071-public-node-inspection.md).

```cpp
sdk::forEachNode(session->scene(), [&](sdk::X3DNode *node) {
    if (node->nodeTypeName() == "Viewpoint") {
        auto label = sdk::readField<x3d::core::SFString>(*node, "description")
                         .value_or("");
        // Add node and label to the host's viewpoint menu.
    }
});
```

The walk visits each node identity once, before its children, starting with
scene roots and then PROTO peers. It follows generated SFNode/MFNode fields in
reflection order, including inactive branches and loaded Inline children.
Shared nodes and cycles are guarded; nulls are skipped. It does not fetch
resources or visit uninstantiated PROTO declarations or author-declared Script
fields. Callback pointers are borrowed: don't change the graph during the walk
or keep pointers after their nodes are removed. This enumerates node
identities, not rendered placements.

`readField<T>` returns an owned `std::optional<T>` copy through the generated
reflected getter. Missing or unreadable fields and exact C++ type mismatches
return `nullopt`. Empty strings and false booleans are present values. Names
are exact X3D field names, without ROUTE aliases, and no numeric or enum-token
conversion is applied. Getter and copy exceptions propagate. Author-declared
Script fields stay on the context-aware Script/SAI APIs. The
`x3d_sdk_node_inspection` test (`include/x3d/tests/sdk_node_inspection_test.cpp`)
covers both functions.

The public aliases are declared in `include/x3d/sdk.hpp`; their CMake wiring
is in `cmake/x3d/libraries.cmake`. The [native host contract](../guides/native-host.md)
describes the experimental render-feed boundary.

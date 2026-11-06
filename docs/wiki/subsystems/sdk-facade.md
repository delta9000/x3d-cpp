---
title: SDK Façade
summary: The single public header consumers include — x3d::sdk — which re-exports the curated v1 embedder surface from namespace x3d::sdk.
tags: [subsystem, sdk, facade, public-api]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/cli-suite.md
  - ../decisions/0001-ext-firewall.md
  - ../guides/gate-system.md
---

# SDK façade

Include `x3d/sdk.hpp` and link `x3d_cpp::sdk`. The façade re-exports selected
runtime types into `x3d::sdk`; it adds no compiled implementation of its own.
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

Not every resource implementation is re-exported by `sdk.hpp`; optional
backend factories have their own headers and build options.

## Package boundaries

`x3d_cpp::authoring` and `x3d/authoring.hpp` provide nodes, the document model,
and writers without simulation or extraction. See [asset import](asset-import.md).

Extensions use `x3d/ext.hpp` and the separate opt-in extension target.
See [the extension firewall](ext-firewall.md).

The public aliases are declared in `include/x3d/sdk.hpp`; their CMake wiring
is in `cmake/x3d/libraries.cmake`. The [native host contract](../guides/native-host.md)
describes the experimental render-feed boundary.

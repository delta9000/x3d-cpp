---
title: "ADR-0051: Deferred Inline expansion uses the parse resolver and live runtime enrollment"
summary: A runtime Inline load uses the injected InlineResolver and the parse expansion path, then enrolls the new subtree in the scene indices, systems, routes, bindings, and extraction dirty tracker. Unload and replacement require a separate detach contract.
tags: [adr, inline, networking, immersive]
updated: 2026-09-26
related:
  - ../subsystems/inline-expand.md
  - 0017-inline-expansion-parse-time.md
---

# ADR-0051: Deferred Inline expansion uses the parse resolver and live runtime enrollment

## Status

Accepted

## Context

Parse-time Inline expansion already resolves a URL list through an injected
`InlineResolver`, isolates the child DEF namespace, and retains the child scene
for IMPORT. A runtime `load=TRUE` event must produce the same content. The
execution context also owns indices, routes, systems, and extraction dirty
state built before the event.

## Decision

`attachStandardRuntime` accepts the same resolver and base URL used by parse.
The optional `InlineRuntimeSystem` calls `expandInlines` for a deferred Inline,
then wires IMPORT aliases and child routes. It refreshes the scene indices,
attaches the new subtree to registered systems and the binding registry, and
marks its parent dirty so the next extraction delta contains the new content.
The resolver is an application dependency supplied to the runtime session;
the Scene remains a data model.

This change implements one-way loading. A loaded Inline is represented by a
synthetic Group, while the original Inline remains retained for serialization.
The current system interface has `attach` but no corresponding `detach`, and
the event graph stores raw node endpoints. Unload and URL replacement must
first define coordinated removal of system state, routes, IMPORT aliases,
binding and pick state, and extracted render items. AUD-NET-2 remains open
until those transitions are implemented and tested.

## Consequences

- Embedders that need runtime Inline loading pass an `InlineResolver` and the
  document base URL when attaching the standard runtime.
- Nested deferred Inlines use the same resolver when their parent loads.
- The loaded subtree participates in full snapshots and incremental deltas.

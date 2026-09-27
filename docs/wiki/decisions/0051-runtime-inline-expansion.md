---
title: "ADR-0051: Deferred Inline expansion uses the parse resolver and live runtime enrollment"
summary: Runtime Inline loading uses the parse resolver; unload and URL replacement purge routes, queued events, and system state before releasing the old subtree, then report render item changes through a topology revision.
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
advances the topology revision so the next extraction delta contains the new content.
The resolver is an application dependency supplied to the runtime session;
the Scene remains a data model.

The synthetic Group retains the loaded content while the original Inline stays
available for serialization and later reload. A live load record owns the
original Inline, Group, parent slot, and loaded URL value. On `load=FALSE` or a
different URL value, unload proceeds while the old nodes are still owned:

1. Remove every route touching the old subtree and discard pending deliveries
   and per-timestamp route guards for those endpoints. Block later events to
   detached nodes, including nodes an embedder still owns.
2. Call `System::detach` for each node, remove bindables from their stacks, and
   clear pick and transform indices. Descendant loaded Inlines detach first.
3. Remove IMPORT aliases and scene-side Inline expansion records, then replace
   the Group in its parent slot with the original Inline. A URL change can now
   expand through the same resolver path used on first load.

The extractor diffs the scene's old and new render items on a topology
revision. A full scene rewalk on an Inline transition is deliberate: it avoids
dereferencing an old root or reusing a stale path identity when an allocator
recycles a removed node address. Ordinary field changes retain the incremental
subtree path. Retired content is released after context and system references
are purged. Sound and physics systems remove detached nodes from their active
registries, so the old backend graphs are no longer rendered or stepped;
backend-owned handles remain allocated until the backend is destroyed.

## Consequences

- Embedders that need runtime Inline loading pass an `InlineResolver` and the
  document base URL when attaching the standard runtime.
- Nested deferred Inlines use the same resolver when their parent loads.
- The loaded subtree participates in full snapshots and incremental deltas;
  unload and replacement report removed RenderItems in the delta.

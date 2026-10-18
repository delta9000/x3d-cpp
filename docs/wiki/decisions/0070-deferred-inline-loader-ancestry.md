---
title: "ADR-0070: Deferred Inline loader ancestry"
summary: Asset-loaded Inline nodes retain their confined loader and active source URLs across runtime ticks.
tags: [adr, inline, assets, runtime]
updated: 2026-10-18
related:
  - ../subsystems/parse-readers.md
---

# ADR-0064: Deferred Inline loader ancestry

## Status

Proposed. Implementation and regression tests await maintainer execution.

## Problem

The paired asset loaders detect cycles during synchronous parsing. Runtime
Inline activation starts another loader call, losing its active source URLs.
A file with a deferred Inline of itself can therefore add another scene every
tick. CAVEOS caps all runtime loads at 64 to contain this growth, also limiting
legitimate independent loads.

## Decision

Retain value-owned asset loader callbacks on Inline nodes of fetched documents,
keyed by weak node identity in `Scene::deferredInlineResolvers`. Each callback
captures the existing loading policy and a snapshot of active normalized source
URLs. Each invocation gets its own mutable Context; no callback retains a
reference to the original parsing Context. Inline expansion transfers these
callbacks alongside node base URLs when splicing child scenes.

The retained callback takes precedence over the runtime fallback resolver for
that node. A host chooses confinement and recursive PROTO decoration when it
loads a world, then uses the paired Inline resolver in SessionOptions. Newly
authored nodes without retained state still use that fallback resolver.
Independent siblings and unload/reload use independent invocation state.
Ancestry is a path property, never a session-wide visited set or load count.

## Consequences

The existing URL normalization, nesting ceiling, candidate fallback, and
Pending rules remain the authority. Core stores and invokes an opaque callback;
it gains no dependency on codecs or the extension layer. Loaded scenes retain
the asset backend and decorator captures until their loader state is released.
Expired weak keys cannot attach policy to a reused node address.

This change addresses factory-loaded deferred Inline nodes. Arbitrary custom
resolvers and local-file resolvers still supply their own cycle semantics;
there is no universal way to infer which source an opaque callback selected.
PROTO declarations cloned outside a fetched Scene don't inherit this per-node
state. This does not add asynchronous loading or a cycle diagnostic channel.

## Alternatives

A global visited set rejects independent placements and later reloads. A fixed
load count merely delays recursive growth and bounds valid scenes too. Keeping
the parsing Context by reference would leave dangling callbacks after return.

---
title: SAI named-node discovery
summary: Reference for named, named_nodes and names in the provider contract.
tags: [sai, reference, provider]
updated: 2026-11-14
related:
  - ../how-to/find-named-nodes.md
  - ../../explanation/decisions/0067-provider-named-node-discovery.md
---

# SAI named-node discovery

Decision record:
[ADR-0067](../../explanation/decisions/0067-provider-named-node-discovery.md). Task guide:
[Find named nodes](../how-to/find-named-nodes.md).

## Operations

| Operation | Returns |
|---|---|
| `named(context, name)` | The node bound to `name` |
| `named_nodes(context)` | Owning vector of `named_node { name, target }` |
| `names(node)` | Owning vector of names bound to that node in its lexical context |
| `define_name(node, name)` | Setup only; binds the node's one canonical DEF name |

## Ordering and contents

- Results are in ascending, case-sensitive name order.
- `named_nodes` includes every name in that lexical context, including named
  nodes detached from roots. Unnamed nodes do not appear.
- `named(context, entry.name)` resolves to `entry.target`.
- `names` on an unnamed node succeeds with an empty vector.

## Scope

- There is one root lexical context per scene. Discovery does not flatten
  nested contexts or enumerate IMPORT aliases.
- Setup still permits only one canonical DEF name per node. These queries add
  no alias authoring and no rename operation.

## States, authority and lifetime

- Both queries work in setup, active and faulted states.
- They follow the discovery authority and owner-thread rules. Disposed, foreign
  and retired handles fail as they do for `named` and `type_name`.
- Returned strings and vectors survive later edits, retirement and service
  destruction. Retained node handles lose authority on retirement.

## Backend seam

A backend implements `do_named_nodes(scene)` from its existing name bindings.
The frontend:

- sorts the result
- rejects null identities, invalid names and duplicate names with
  `invalid_descriptor`
- wraps IDs in provider-owned handles
- implements reverse lookup by filtering that same enumeration, with no second
  name table

The default virtual returns `unsupported_operation`.

## Evidence

The shared `check_provider_named_nodes` oracle runs against both backends. For
the native backend it runs in `sai_provider_native_runtime`.

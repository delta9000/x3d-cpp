---
title: Find named nodes through the SAI
summary: Look up a node by DEF name, list every named node in a context, and get the names of a node.
tags: [sai, how-to, provider]
updated: 2026-11-14
related:
  - ../reference/named-nodes.md
---

# Find named nodes through the SAI

These calls work in setup, active and faulted scenes, on either backend. Rules
and ordering are in [Named nodes](../reference/named-nodes.md).

## Get the context

```cpp
auto context = service.root_context(scene);
```

## Look up one node by name

```cpp
auto paint = service.named(*context, "Paint");
if (!paint) { /* no such name, or no authority */ }
```

## List every named node

```cpp
auto entries = service.named_nodes(*context);
if (entries) {
  for (const auto &entry : *entries) {
    // entry.name is the DEF name; entry.target is a usable node handle.
    auto type = service.type_name(entry.target);
  }
}
```

Entries come back in ascending, case-sensitive name order. Named nodes that are
not attached to a root are included.

## Get the names of a node

```cpp
auto names = service.names(node);
const std::string label =
    names && !names->empty() ? names->front() : std::string{"(unnamed)"};
```

An unnamed node gives a successful empty vector, so this is safe for formatting
any node reference.

## Name a node

Naming is a setup operation:

```cpp
service.define_name(node, "Paint");
```

A node has one DEF name. There is no rename or alias call.

## In an adopted native scene

IMPORT aliases are not listed, and looking one up by name is refused. See
[Native provider](../reference/native-provider.md#opaque-content).

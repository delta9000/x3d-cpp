---
title: "ADR-0057: Scene-Owned Author Fields and Scoped Clone Services"
summary: Explicit Scene and declaration owners replace the process-global author-field store and clone factory.
tags: [adr, ownership, script, proto, dynamic-fields]
updated: 2026-10-07
related:
  - 0014-dynamic-field-foundation.md
  - ../subsystems/execution-context.md
  - ../subsystems/system-script-sai.md
  - ../subsystems/proto-expand.md
---

# ADR-0057: Scene-Owned Author Fields and Scoped Clone Services

## Status

Accepted. Supersedes the process-global ownership portion of ADR-0014; the
hand-written side table and generated-node boundary remain unchanged.

## Context

Distinct nodes with identical author-field names did not collide in the old
pointer-keyed table. The ownership defects were more specific: global cleanup
removed unrelated scene data; Inline detachment erased author fields on retained
child nodes; PROTO cloning dropped Script author fields; and the extension
resolver installed a sticky process-wide clone factory. Copied FieldInfo thunks
also remained usable after their tracked node expired until another lookup
swept the entry. These are unsuitable defaults for independently owned worlds.

## Decision

- Every fresh `Scene` owns `shared_ptr<DynamicFieldStore> authorFields`.
  Shallow Scene/document copies intentionally alias both graph and store.
  Independent worlds need independent nodes and field entries.
- `ProtoDeclaration::authorFields` retains template/default storage, including
  when the declaration outlives an EXTERN resolver's temporary document.
- Readers register into their current Scene. Writers receive the current Scene
  or declaration owner. The event cascade and both Script backends use the
  execution context's explicit owner. No singleton or thread-local fallback
  remains.
- Inline adoption shares entries for the same imported nodes. It does not copy
  their values or share the whole parent/child store. A conflicting live entry
  for the same node is rejected. Removing a parent's view preserves a retained
  child's data; cached GeoLOD tiles import their entries again on redisplay. Authored
  GeoLOD rootNode content uses a per-LOD entry view while inactive, because it
  has no separate child Scene; redisplay returns those views to the active Scene.
- Entry ownership stays weak toward its node. Copied tracked FieldInfo thunks
  check node lifetime and identity before accessing values. The store/entry
  lock order remains consistent. Reference-based registration remains available
  for caller-managed nodes, with its existing explicit lifetime obligation.
- `CloneContext` carries source/destination stores and an optional fallback
  factory for a synchronous clone. Author declarations and stored values get
  fresh entries; SFNode/MFNode values use the same clone map as generated fields.
  `ProtoDeclaration::createNode` scopes an extension factory to that declaration.
  Each `ext::install()` result owns a fresh declaration and changes no global
  configuration. Generated-fields-only `deepClone(node)` remains available.

## Migration

This changes concrete runtime layouts and removes source APIs. Rebuild consumers;
this is not a patch-level ABI compatibility claim. The abstract `ScriptEngine`
virtual interface remains unchanged. Concrete context and author-field APIs are
experimental rather than covered by the former blanket Script/SAI freeze claim.

```cpp
auto doc = x3d::codec::parseDocument(text);
auto &fields = *doc.scene.authorFields;
x3d::runtime::X3DExecutionContext ctx(doc.scene.authorFields);
ctx.buildSceneGraph(doc.scene);
ctx.buildFrom(doc.scene);
auto visible = x3d::runtime::effectiveFields(*node, fields);
```

For standalone programmatic nodes, `ctx.authorFields()` exposes that context's
fresh store. If fields must be registered before context construction, create a
shared store explicitly and pass the same owner to the context. An untouched
default context may bind once through its first scene build. Taking its field
accessor first fixes its standalone owner; different-owner rebinding, pending
events or an already-used clock are rejected before scene changes. Do not replace
a Scene's owner while it is active.

```cpp
auto text = x3d::codec::XmlWriter{}.writeNode(node, doc.scene.authorFields.get());
x3d::runtime::Scene target;
auto copy = x3d::runtime::deepClone(node,
    x3d::runtime::CloneContext{doc.scene.authorFields.get(),
                               target.authorFields.get(), {}});
```

Without a store, bare-node XML serialization includes generated fields and
Script source only. Full document/Scene and declaration serialization selects
the appropriate owner automatically. `erase`/`clear` drop a store's own views;
copied FieldInfo remains usable while another store retains that same live
entry. It becomes inert after the last owner drops it or its tracked node dies.
A retained node alone does not retain the Scene field store; retain that shared
owner explicitly when author fields must outlive world teardown.

## Verification and limits

`scene_author_field_ownership_test.cpp` covers distinct fresh owners, deliberate
shallow aliases, Inline shared-value lifecycle, copied-thunk lifetime and address
reuse, conflicting imports, Script/PROTO clone values, node-valued clone aliases,
declaration lifetime, context binding and resolver-local extension factories.
Existing reader/writer, memory, cascade and Script tests use explicit stores.

The host still serializes each mutable world's operations. Sharing the same
mutable native graph across active worlds is not independent-world isolation.
This change does not replace the separate process-wide Geo projection selector,
remove local-file resolver thread-local operation state, supply missing
SFNode/MFNode author-default reader support, or establish complete Script, PROTO
or SAI conformance.

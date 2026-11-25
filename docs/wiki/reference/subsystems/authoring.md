---
title: Authoring and SceneBuilder
summary: The footprint-gated x3d_cpp::authoring target and SceneBuilder, the checked way to build an X3D document in code.
tags: [subsystem, authoring, scenebuilder, cmake]
updated: 2026-11-25
related:
  - ../../explanation/decisions/0042-authoring-target.md
  - ../../how-to/build-a-scene-in-code.md
  - sdk-facade.md
---

# Authoring and SceneBuilder

## Authoring target

`x3d_cpp::authoring` and `x3d/authoring.hpp` provide node construction, document
containers, range checks, and writers. They exclude parser, execution, and
extraction libraries. `scripts/authoring-footprint.sh` builds
`x3d_authoring_link_contract` (`examples/embed_minimal/authoring.cpp`, linked
against `x3d_cpp::authoring` only) and rejects excluded subsystem symbols in
it; it has no byte-size budget. [Mesh import](asset-import.md) builds on the
same runtime and links no parser either.

## SceneBuilder

`runtime/SceneBuilder.hpp` (header-only, namespace `x3d::authoring`) builds an
`X3DDocument` with checks at the call site. It works through reflection, so it
covers every generated node type.

| Call | Does | Throws `std::invalid_argument` when |
|---|---|---|
| `b.add<T>()`, `b.add("Type")`, `b.create<T>()` | Creates a root node, or a detached one. | The type name is unknown. |
| `node.add<C>(field = "")`, `node.add("Type", field)`, `node.add(child, field)` | Attaches a child; an empty `field` means the child's default containerField. SFNode fields are replaced, MFNode fields appended. Attaching one node twice writes a USE. | The parent has no such field, or it is not a node field. |
| `node.set(field, "text")` | Parses X3D attribute text for the field's type. | The field is unknown or a node field, the text has the wrong arity or a bad number, or an enum token is unknown (SFEnum and MFEnum). |
| `node.set(field, value)` | Stores a typed value. | `value` is not the field's exact C++ type. |
| `node.with(fn)` | Runs `fn(T&)` for the typed setters. | |
| `node.def(name)` | Sets the DEF and registers it; a rename follows into routes that name the node, including a parsed document's. | The name is bound to another node. |
| `b.route(from, f, to, g)` | Adds a ROUTE; an endpoint with no DEF gets a generated `_Type<n>` name. `set_x` and `x_changed` resolve to inputOutput `x`. | A field is unknown, the source is not an output, the destination is not an input, or the field types differ. |
| `b.routeUnchecked(...)` | Adds a ROUTE without field checks, for Script and shader author fields and ProtoInstance interfaces. | An endpoint is null. |
| `b.find(def)`, `b.document()`, `b.take()` | Looks up a node, borrows the document, or moves it out. | `find`: the DEF is unknown. |

Not checked: whether a child's node type is legal in the slot (node fields are
untyped in the bindings) and value ranges (`validateRanges` covers those). A
call that throws from `b.add<T>().def(...)` has already added the root.
`SceneBuilder(doc)` takes the document by value and continues it; its existing
content is not re-checked. Test: `include/x3d/tests/authoring_builder_test.cpp`.

## Key files

| Source | Role |
|---|---|
| `include/x3d/authoring.hpp` | Authoring façade |
| `runtime/SceneBuilder.hpp` | Checked scene construction |
| `scripts/authoring-footprint.sh` | Symbol gate |

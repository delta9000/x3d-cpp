---
title: Generated Bindings
summary: The committed, golden-locked generated C++ node bindings that are the output artifact of the generator pipeline.
tags: [subsystem, generated, bindings, golden, namespace]
updated: 2026-07-17
related:
  - ../../explanation/architecture.md
  - generator.md
  - reflection.md
  - scene-graph.md
  - ../../explanation/decisions/0005-golden-files-in-git.md
  - ../../explanation/decisions/0006-compiled-static-lib.md
  - ../../explanation/decisions/0039-generated-binding-namespaces.md
  - ../gates.md
---

# Generated Bindings

## Purpose

- `generated_cpp_bindings/` is the committed C++ output of the generator
  pipeline.
- It provides a typed C++ class for every X3D-4.0 node, a full runtime
  reflection layer (type-erased `FieldInfo` tables with get/set thunks), a
  node factory, an interface registry, and the shared field-type and
  enumeration headers.
- Every other runtime subsystem — parse readers, codec writers, the event
  cascade, the extraction pipeline, and the SDK façade — depends on this layer
  as its only representation of X3D nodes.
- Extension nodes are separate from the generated core.

The bindings are not built by hand and not re-generated at compile time. They are generated once by the Python generator (`mise run gen`) and then committed to git. That commit is the golden artifact; the `mise run golden` gate detects any generator-or-template drift before it lands.

## Namespaces and header layout (ADR-0039)

The generated types live in two namespaces, mirrored by two header subdirectories under
`generated_cpp_bindings/`:

- `x3d::core` → `generated_cpp_bindings/x3d/core/` — the value/reflection vocabulary
  (`X3Dtypes.hpp`, `X3Denums.hpp`, `X3DReflection.hpp`): `SF*/MF*` types, `FieldInfo`/
  `FieldTable`/`NodeVisitor`/`RangeDiagnostic`/`X3DFieldType`/`AccessType`, and the bounded enums.
- `x3d::nodes` → `generated_cpp_bindings/x3d/nodes/` — the `X3DNode` base, every abstract
  `X3D*Node`, all concrete node classes, the node factory, and the interface registry.

- Headers use `#pragma once` and include each other (and are included by
  consumers) with a uniform in-tree-and-installed spelling: `#include
  "x3d/core/X3Dtypes.hpp"`, `#include "x3d/nodes/Transform.hpp"` (the in-tree
  include base is `generated_cpp_bindings/`).
- The one cross-namespace seam is `x3d::core::SFNode =
  std::shared_ptr<x3d::nodes::X3DNode>`.
- Consumers qualify asymmetrically: a `using namespace x3d::core;` for the
  small vocabulary set (scoped, never global), but explicit `x3d::nodes::Foo`
  (or a per-file `namespace xn = x3d::nodes;` alias in heavy dispatch files)
  for node types — no blanket `using namespace x3d::nodes`.
- `ctest x3d_namespace_taxonomy` pins this.

## Key files

| File / directory | Role |
|---|---|
| `x3d/core/X3Dtypes.hpp` | (`namespace x3d::core`) All X3D SF*/MF* C++ type aliases and struct definitions (`SFVec3f`, `SFColor`, `SFNode = std::shared_ptr<x3d::nodes::X3DNode>`, `MFNode`, etc.). The value structs carry C++20 defaulted `operator==`/`!=` (exact member-wise comparison — vocabulary, not math; arithmetic stays out per ADR-0012) |
| `x3d/core/X3Denums.hpp` | (`namespace x3d::core`) All bounded X3D enumeration vocabularies as `enum class` (e.g. `AlphaModeChoices`, `AccessTypeChoices`), with `to_string`/`from_string` support |
| `x3d/core/X3DReflection.hpp` | (`namespace x3d::core`) Shared reflection infrastructure: `X3DFieldType`, `AccessType`, `FieldInfo`, `FieldTable`, `RangeDiagnostic`, `NodeVisitor` |
| `x3d/nodes/X3DNode.hpp` / `.cpp` | (`namespace x3d::nodes`) Base class for all instantiable nodes: `metadata`, `DEF`/`USE`/`class`/`id`/`style` fields; virtual `nodeTypeName()`, `defaultContainerField()`, `fields()`, `accept()`, `validateRanges()` |
| `x3d/nodes/<NodeName>.hpp` / `.cpp` | (`namespace x3d::nodes`) One pair per concrete X3D node (e.g. `Appearance.hpp`/`.cpp`, `Box.hpp`/`.cpp`). 260 concrete node pairs. Each `.hpp` holds the class declaration with typed getters/setters and range-throwing validators (every getter returns `const T&` to its member, so reading an MF field or a string never copies it; ADR-0049); each `.cpp` holds the out-of-line `fields()` FieldTable (with thunks), `accept()`, `validateRanges()`, and `checkRanges*` statics. |
| `x3d/nodes/X3D<AbstractName>.hpp` / `.cpp` | (`namespace x3d::nodes`) 77 abstract-node and mixin-interface pairs (e.g. `X3DGeometryNode`, `X3DBoundedObject`, `X3DGroupingNode`) using `public virtual` inheritance. The 3 shared-infrastructure vocabulary headers (`X3Dtypes`, `X3Denums`, `X3DReflection`) live under `x3d/core/`; the factory + interface registry live under `x3d/nodes/` |
| `x3d/nodes/X3DNodeFactory.hpp` / `.cpp` | (`namespace x3d::nodes`) Name-to-constructor registry: `X3DNodeFactory::create(typeName)` and free function `createX3DNode(typeName)` |
| `x3d/nodes/X3DInterfaceRegistry.hpp` / `.cpp` | (`namespace x3d::nodes`) Queryable node-type → transitive interface set: `InterfaceId` enum, `X3DInterfaceRegistry::interfacesOf()`, `nodeImplements()`, `nodesImplementing()` |

Total: 343 `.hpp` + 341 `.cpp` = 684 source files in the directory.

## Interfaces and seams

### Exposed interface

Every concrete node class publicly inherits (directly or transitively) from `X3DNode`:

```cpp
// X3DNode.hpp — base of every generated node
class X3DNode {
public:
    virtual std::string nodeTypeName() const;
    virtual std::string defaultContainerField() const;
    virtual const FieldTable& fields() const;      // FieldTable = std::vector<FieldInfo>
    virtual void accept(NodeVisitor& visitor) const;
    virtual void validateRanges(std::vector<RangeDiagnostic>& out) const;
    // plus: getMetadata/setMetadata, getDEF/setDEF, getUSE/setUSE, getClass_/setClass_,
    //       getId/setId, getStyle/setStyle, getIS/setIS  // IS: SFNode (X3DNode base)
};
```

The key reflection types from `X3DReflection.hpp`:

```cpp
struct FieldInfo {
    std::string x3dName;
    X3DFieldType type;       // SFBool .. MFEnum (44 values)
    AccessType access;       // InitializeOnly | InputOnly | OutputOnly | InputOutput
    std::string containerField;  // non-empty for SFNode/MFNode fields
    std::function<std::any(const X3DNode&)> get;
    std::function<void(X3DNode&, const std::any&)> set;
    // enum-only string-round-trip thunks:
    std::function<std::string(const X3DNode&)> getEnumString;
    std::function<void(X3DNode&, const std::string&)> setEnumString;
    bool isNode() const; bool isEnum() const;
    bool isReadable() const; bool isWritable() const;
};
using FieldTable = std::vector<FieldInfo>;
```

Factory:

```cpp
// X3DNodeFactory.hpp
class X3DNodeFactory {
public:
    static const std::unordered_map<std::string, Creator>& registry();
    static std::shared_ptr<X3DNode> create(const std::string& typeName);
};
std::shared_ptr<X3DNode> createX3DNode(const std::string& typeName);
```

Interface registry (replaces `dynamic_cast` for type-membership queries):

```cpp
// X3DInterfaceRegistry.hpp
class X3DInterfaceRegistry {
public:
    static std::span<const InterfaceId> interfacesOf(const std::string& nodeTypeName);
    static bool nodeImplements(const std::string& nodeTypeName, InterfaceId iface);
    static bool nodeImplements(const X3DNode* node, InterfaceId iface);
    static const std::vector<std::string>& nodesImplementing(InterfaceId iface);
};
```

### Two write paths — typed setters never seed the event cascade

- The generated typed setters are the data layer, not the behavior layer.
- `transform->setTranslation(v)` just assigns the member: no change event, no
  cascade seed.
- Generated setters are appropriate for initial authoring.
- They do not notify the event cascade or dirty tracker.
- For live changes, use context input events or `writeField` and check its
  result.
- Raw setter writes can leave cached bounds and extracted data stale.
- See [execution context](execution-context.md).

### Seam points

- Reflection write path (lenient) — codec readers and the event cascade write field values via the reflection `set` thunk, which routes through a generated `set<Name>Unchecked()` that skips the range check. The typed public `setX()` accessor still throws `std::out_of_range` for out-of-range values; the lenient path keeps authored out-of-range values and surfaces them later via `RangeDiagnostic` (see [`docs/superpowers/specs/2026-06-07-lenient-read-range-warnings-design.md`](https://github.com/delta9000/x3d-cpp/blob/db23737f43f9ec16d983583834c40c70c810bf63/docs/superpowers/specs/2026-06-07-lenient-read-range-warnings-design.md)). This is the binding between the generated layer and every codec reader.

- `NodeVisitor` provides generated enter/leave double dispatch through `accept()`. The built-in writers traverse field tables directly.

- `X3DNodeFactory` registry — parse readers call `X3DNodeFactory::create(typeName)` (or the free function `createX3DNode`) to instantiate any X3D node by name without including its header. The factory definitions live in `X3DNodeFactory.cpp` compiled into the `x3d_cpp_nodes` library.

- `X3DInterfaceRegistry` — runtime subsystems that need to test node type membership without `dynamic_cast` (e.g. "is this node an `X3DGeometryNode`?") call `X3DInterfaceRegistry::nodeImplements(node, InterfaceId::X3DGeometryNode)`.

- `effectiveFields()` (dynamic field extension) — the `X3DScriptNode` subclass and the dynamic-field mechanism (`runtime/events/DynamicField.hpp`) extend the base `fields()` set at runtime for Script author-declared fields. The cascade, route resolution, and SAI layer call `effectiveFields()` rather than `fields()` to see both static and dynamic fields. This seam is in the runtime, not the generated bindings, but it depends on the generated `FieldInfo`/`FieldTable` types.

### CMake targets

`x3d_cpp::headers` supplies generated declarations. `x3d_cpp::nodes` supplies
compiled field tables and registries; the runtime libraries supply parser,
writer, and execution definitions. `x3d_cpp::sdk` links the full runtime.
Libraries are shared by default; `X3D_CPP_SHARED_NODES=OFF` makes the generated
and runtime libraries static together. See `cmake/x3d/libraries.cmake`.

## Related

- [ADR-0005: Golden Files in Git](../../explanation/decisions/0005-golden-files-in-git.md)
- [ADR-0006: Compiled Static Lib](../../explanation/decisions/0006-compiled-static-lib.md)
- [Generator Pipeline](generator.md)
- [Reflection](reflection.md)
- [Scene Graph](scene-graph.md)
- [Architecture](../../explanation/architecture.md)
- [Gate System](../gates.md)
- [ADR-0039: Generated Bindings Live in x3d::core + x3d::nodes](../../explanation/decisions/0039-generated-binding-namespaces.md)

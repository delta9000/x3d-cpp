---
title: Reflection Layer
summary: Field descriptors, reflection thunks, and the node factory/registry that enable generic traversal and codec IO without per-node code.
tags: [subsystem, reflection, descriptors, factory, registry, codegen]
updated: 2026-09-26
related:
  - ../architecture.md
  - ../subsystems/generated-bindings.md
  - ../subsystems/generator.md
  - ../subsystems/codecs-writers.md
  - ../subsystems/parse-readers.md
  - ../subsystems/routes.md
  - ../decisions/0003-throw-on-range.md
  - ../decisions/0005-golden-files-in-git.md
  - ../decisions/0014-dynamic-field-foundation.md
  - ../decisions/0049-zero-copy-field-reads.md
---

# Reflection Layer

## Purpose

The reflection layer is the glue between the strongly-typed generated C++ node bindings and every piece of code that must operate on nodes generically — codecs, parse readers, route resolution, the Script/SAI runtime, and extraction. It answers three questions at runtime without requiring per-node code in the caller:

1. What fields does this node have, and how do I read or write them? — answered by the `FieldTable` / `FieldInfo` mechanism emitted into every node's `.cpp`.
2. What abstract interfaces does a named node type implement? — answered by `X3DInterfaceRegistry` (transitive closure of abstract ancestor types).
3. Given a type name, how do I instantiate a node? — answered by `X3DNodeFactory`.

The layer is split across the Python generator (`src/x3d_cpp_gen/emit/`) and the generated artifacts committed in `generated_cpp_bindings/`. The Python side computes every decision (C++ type, accessor name, default expression, constraint body, thunk routing); the generated side is the compiled truth that the runtime links against. Nothing in the runtime hand-codes per-node behavior — it always goes through the reflection surface.

## Key files

| File / directory | Role |
|---|---|
| `src/x3d_cpp_gen/emit/reflection.py` | Emits `X3DReflection.hpp`: the `X3DFieldType` enum (data-driven from `X3DType`), `AccessType` enum, `FieldInfo` / `FieldTable` types, `RangeDiagnostic`, and the `NodeVisitor` base class |
| `src/x3d_cpp_gen/emit/descriptors.py` | Computes `FieldDescriptor` per field: C++ type, accessor names, default literal, constraint-check body, range-collect body, enum handling, `inherited_from` base-qualification |
| `src/x3d_cpp_gen/emit/registry.py` | Emits `X3DInterfaceRegistry.hpp/.cpp`: one `InterfaceId` enumerator per abstract node type; per-node transitive-closure table; `interfacesOf` / `nodeImplements()` / `nodesImplementing` queries |
| `src/x3d_cpp_gen/emit/factory.py` | Emits `X3DNodeFactory.hpp/.cpp`: concrete-node-only `create(typeName)` free function and `registry()` map |
| `src/x3d_cpp_gen/emit/naming.py` | `pascal()` and `sanitize_field_name()` — the canonical naming rules shared by the parser and all emitters |
| `generated_cpp_bindings/x3d/core/X3DReflection.hpp` | Committed golden: the shared reflection types every node header `#include`s |
| `generated_cpp_bindings/x3d/nodes/X3DNodeFactory.hpp` / `.cpp` | Committed golden: the factory definition (compiled into `x3d_cpp_nodes`) |
| `generated_cpp_bindings/x3d/nodes/X3DInterfaceRegistry.hpp` / `.cpp` | Committed golden: the interface-id enum and membership tables |
| `generated_cpp_bindings/x3d/nodes/<NodeName>.cpp` | Per-node golden: the `fields()` static (lambda-initialized `FieldTable`) and `accept()` double-dispatch body |
| `runtime/FieldRead.hpp` | Reads of reflected values. Zero-copy: `fieldPtr<T>` (borrow a generated field's member through `FieldInfo::view`), `FieldRef<T>` (borrow when possible, else box through `get`, e.g. for author fields) and `forEachChildNode` (walk a node's SFNode/MFNode children without copying a vector or bumping a refcount; ADR-0049). Exception-free: `fieldValueAs<T>` (pointer-form `any_cast`; empty → null, a present value of the wrong type asserts in debug) and `enumToken` (an SFEnum/MFEnum field's token via `getEnumString`). Use these instead of `any_cast` inside `try`/`catch (...)`, which also swallows unrelated errors. |
| `runtime/events/DynamicField.hpp` | Runtime extension: `effectiveFields(node, store)` concatenates the generated `fields()` table with per-instance author `<field>` declarations; `DynamicFieldStore` holds the side-table |
| `runtime/X3DRangeValidate.hpp` | Runtime helper: `collectRangeWarnings()` walks a scene calling `validateRanges()` on each node (the per-node `validateRanges()` is generated), then checks cross-element rules such as non-decreasing Background and TextureBackground sky/ground angles and emits `BACKGROUND_ANGLE_ORDER` warnings. |

## Interfaces and seams

### Exposed interface

The primary generated surface in `X3DReflection.hpp`:

```cpp
// Field-type tag — derived from X3DType so it stays in sync with the spec model.
enum class X3DFieldType { SFBool, SFColor, ..., SFEnum, MFEnum };

// Field access category.
enum class AccessType { InitializeOnly, InputOnly, OutputOnly, InputOutput };

// Type-erased per-field descriptor held in a node's FieldTable.
struct FieldInfo {
    std::string x3dName;
    X3DFieldType type;
    AccessType access;
    std::string containerField;          // non-empty for SFNode/MFNode fields
    std::function<std::any(const X3DNode&)>          get;
    std::function<void(X3DNode&, const std::any&)>   set;
    std::function<std::string(const X3DNode&)>       getEnumString;   // SFEnum/MFEnum only
    std::function<void(X3DNode&, const std::string&)> setEnumString;  // SFEnum/MFEnum only
    FieldView (*view)(const X3DNode&) = nullptr;  // zero-copy read (ADR-0049)
    bool isNode() const;   bool isEnum() const;
    bool isReadable() const; bool isWritable() const; bool isViewable() const;
};

// Borrowed pointer to a field's stored member plus its C++ type (ADR-0049).
struct FieldView { const void* data; const std::type_info* type; };

using FieldTable = std::vector<FieldInfo>;

// Out-of-range diagnostic (lenient read path; see ADR-0003).
struct RangeDiagnostic { std::string nodeType, defName, fieldName, detail; };

// Visitor for node-agnostic codec traversal.
class NodeVisitor {
public:
    virtual ~NodeVisitor() = default;
    virtual bool enter(const X3DNode& node) { return true; }  // default: visit all
    virtual void leave(const X3DNode& node) {}                 // default: no-op
};
```

Every generated node satisfies:

```cpp
class SomeNode : public ... {
    std::string nodeTypeName() const override;       // e.g. "Box"
    std::string defaultContainerField() const override;
    const FieldTable& fields() const override;       // function-local static
    void accept(NodeVisitor& visitor) const override;
    void validateRanges(std::vector<RangeDiagnostic>& out) const override;
};
```

The factory interface (`X3DNodeFactory.hpp`):

```cpp
class X3DNodeFactory {
public:
    using Creator = std::function<std::shared_ptr<X3DNode>()>;
    static const std::unordered_map<std::string, Creator>& registry();
    static std::shared_ptr<X3DNode> create(const std::string& typeName); // nullptr if unknown
};
std::shared_ptr<X3DNode> createX3DNode(const std::string& typeName);
```

The interface registry (`X3DInterfaceRegistry.hpp`):

```cpp
enum class InterfaceId : uint16_t { X3DChildNode, X3DGeometryNode,..., X3DViewpointNode };

class X3DInterfaceRegistry {
public:
    static std::span<const InterfaceId> interfacesOf(const std::string& nodeTypeName);
    static bool nodeImplements(const std::string& nodeTypeName, InterfaceId iface);
    static bool nodeImplements(const X3DNode* node, InterfaceId iface);  // convenience
    static const std::vector<std::string>& nodesImplementing(InterfaceId iface);
};
```

### Seam points

- `FieldInfo::get` / `FieldInfo::set` thunks — bound by each node's generated `fields()` implementation. The thunks call the strongly-typed getter and unchecked setter (`getSize()`, `setSizeUnchecked()`) via `dynamic_cast` to the concrete node type, boxing/unboxing the value as `std::any`. Constrained `inputOutput` fields route their set thunk through the `set<Name>Unchecked()` path (the lenient read path); range enforcement stays in the public `set<Name>()`. `initializeOnly` fields also use the unchecked path (no public setter).

- `FieldInfo::view` thunk — a plain function pointer emitted for every readable generated field. It returns the address of the member (through the field's `const T&` getter) and `&typeid(T)`, so `fieldPtr<T>` can hand out a type-checked `const T*` with no `std::any`, no copy and no `std::function` call. It is null for `inputOnly` fields and for fields with no stored member: synthesized author fields (`DynamicFieldStore`) and the hand-written `ExternalGeometry` table. `FieldRef<T>` falls back to `get` there. A borrowed pointer is valid while the node lives, and it reads the new value after a write (ADR-0049).

- `FieldDescriptor::inherited_from` — when a field is inherited through a diamond base (`X3DNode`, `X3DGeometryNode`, etc.), `descriptors.py` records the ancestor in `inherited_from`. The thunk generator emits base-qualified calls (`X3DNode::getMetadata()`) to resolve the ambiguity, making the generated `.cpp` compile cleanly despite `public virtual` multiple inheritance.

- `SFEnum` / `MFEnum` tags + `getEnumString` / `setEnumString` — a bounded enum field's C++ type is a generated `enum class`, opaque to a generic codec. The two string-typed thunks let codecs round-trip the token without knowing the concrete type. Emitted by `descriptors.py._build_enum_descriptor()`.

- `effectiveFields(node, store)` (`runtime/events/DynamicField.hpp`) — runtime extension seam. When a Script node carries author `<field>` declarations, those fields are not in the generated `fields()` table. `effectiveFields()` concatenates the static table with the explicit Scene/declaration owner's `DynamicFieldStore` entries, producing a unified `FieldTable`. Route resolution (`X3DSceneBridge::findField`) and the SAI context (`SaiContext::findField`) call `effectiveFields()`; generated field consumers use `node.fields()`; consumers needing author fields must pass the correct owner explicitly, including shader extraction.

- `NodeVisitor` double-dispatch — a codec or traversal implements `NodeVisitor` and calls `node.accept(v)`. The generated `accept()` calls `visitor.enter(*this)` (returning false skips children) then `visitor.leave(*this)`. Node children are not descended into automatically — the codec visits them explicitly via the `SFNode`/`MFNode` fields in the `FieldTable`.

- `X3DNodeFactory::create()` — parse readers call this to instantiate a node from its type-name string. Only concrete node types are registered; abstract types return `nullptr`. `XmlReader` calls `X3DNodeFactory::create()` directly (via `XmlReaderAdapter` wrapping `codec::XmlReader`); `Vrml97Reader`, `ClassicVrmlReader`, and `JsonReader` route through the `NodeBuilder` helper.

- `X3DInterfaceRegistry::nodeImplements()` — some runtime systems that need type tests without `dynamic_cast` (e.g. `BindingSystem`) call this. `PickSystem` uses `node->nodeTypeName()` string comparisons rather than `nodeImplements()`. The membership tables are built once from the same UOM inheritance data used to generate the headers, so generation uses the same inheritance model for both.

## Related

- [ADR-0049: Zero-Copy Field Reads](../decisions/0049-zero-copy-field-reads.md)
- [ADR-0003: Throw on Range](../decisions/0003-throw-on-range.md)
- [ADR-0005: Golden Files in Git](../decisions/0005-golden-files-in-git.md)
- [ADR-0014: Dynamic Field Foundation](../decisions/0014-dynamic-field-foundation.md)
- [Architecture](../architecture.md)
- [Generated Bindings](../subsystems/generated-bindings.md)
- [Generator](../subsystems/generator.md)
- [Codecs / Writers](../subsystems/codecs-writers.md)
- [Parse / Readers](../subsystems/parse-readers.md)
- [Routes](../subsystems/routes.md)

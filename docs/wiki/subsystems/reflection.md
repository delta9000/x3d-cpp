---
title: Reflection Layer
summary: Field descriptors, reflection thunks, and the node factory/registry that enable generic traversal and codec IO without per-node code.
tags: [subsystem, reflection, descriptors, factory, registry, codegen]
updated: 2026-06-20
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
---

# Reflection Layer

## Purpose

The reflection layer is the glue between the strongly-typed generated C++ node bindings and every piece of code that must operate on nodes generically — codecs, parse readers, route resolution, the Script/SAI runtime, and extraction. It answers three questions at runtime without requiring per-node code in the caller:

1. **What fields does this node have, and how do I read or write them?** — answered by the `FieldTable` / `FieldInfo` mechanism emitted into every node's `.cpp`.
2. **What abstract interfaces does a named node type implement?** — answered by `X3DInterfaceRegistry` (transitive closure of abstract ancestor types).
3. **Given a type name, how do I instantiate a node?** — answered by `X3DNodeFactory`.

The layer is split across the Python generator (`src/x3d_cpp_gen/emit/`) and the generated artifacts committed in `generated_cpp_bindings/`. The Python side computes every decision (C++ type, accessor name, default expression, constraint body, thunk routing); the generated side is the compiled truth that the runtime links against. Nothing in the runtime hand-codes per-node behavior — it always goes through the reflection surface.

## Key files

| File / directory | Role |
|---|---|
| `src/x3d_cpp_gen/emit/reflection.py` | Emits `X3DReflection.hpp`: the `X3DFieldType` enum (data-driven from `X3DType`), `AccessType` enum, `FieldInfo` / `FieldTable` types, `RangeDiagnostic`, and the `NodeVisitor` base class |
| `src/x3d_cpp_gen/emit/descriptors.py` | Computes `FieldDescriptor` per field: C++ type, accessor names, default literal, constraint-check body, range-collect body, enum handling, `inherited_from` base-qualification |
| `src/x3d_cpp_gen/emit/registry.py` | Emits `X3DInterfaceRegistry.hpp/.cpp`: one `InterfaceId` enumerator per abstract node type; per-node transitive-closure table; `interfacesOf` / `nodeImplements` / `nodesImplementing` queries |
| `src/x3d_cpp_gen/emit/factory.py` | Emits `X3DNodeFactory.hpp/.cpp`: concrete-node-only `create(typeName)` free function and `registry()` map |
| `src/x3d_cpp_gen/emit/naming.py` | `pascal()` and `sanitize_field_name()` — the canonical naming rules shared by the parser and all emitters |
| `generated_cpp_bindings/X3DReflection.hpp` | Committed golden: the shared reflection types every node header `#include`s |
| `generated_cpp_bindings/X3DNodeFactory.hpp` / `.cpp` | Committed golden: the factory definition (compiled into `x3d_cpp_nodes`) |
| `generated_cpp_bindings/X3DInterfaceRegistry.hpp` / `.cpp` | Committed golden: the interface-id enum and membership tables |
| `generated_cpp_bindings/<NodeName>.cpp` | Per-node golden: the `fields()` static (lambda-initialized `FieldTable`) and `accept()` double-dispatch body |
| `runtime/events/DynamicField.hpp` | Runtime extension: `effectiveFields(node)` concatenates the generated `fields()` table with per-instance author `<field>` declarations; `DynamicFieldStore` holds the side-table |
| `runtime/X3DRangeValidate.hpp` | Runtime helper: `collectRangeWarnings()` walks a scene calling `validateRanges()` on each node (the per-node `validateRanges` is generated) |

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
    bool isNode() const;   bool isEnum() const;
    bool isReadable() const; bool isWritable() const;
};

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
enum class InterfaceId : uint16_t { X3DChildNode, X3DGeometryNode, ..., X3DViewpointNode };

class X3DInterfaceRegistry {
public:
    static std::span<const InterfaceId> interfacesOf(const std::string& nodeTypeName);
    static bool nodeImplements(const std::string& nodeTypeName, InterfaceId iface);
    static bool nodeImplements(const X3DNode* node, InterfaceId iface);  // convenience
    static const std::vector<std::string>& nodesImplementing(InterfaceId iface);
};
```

### Seam points

- **`FieldInfo::get` / `FieldInfo::set` thunks** — bound by each node's generated `fields()` implementation. The thunks call strongly-typed accessors (`getSizeUnchecked()`, `setSizeUnchecked()`) via `dynamic_cast` to the concrete node type, boxing/unboxing the value as `std::any`. Constrained `inputOutput` fields route their set thunk through the `set<Name>Unchecked()` path (the lenient read path); range enforcement stays in the public `set<Name>()`. `initializeOnly` fields also use the unchecked path (no public setter).

- **`FieldDescriptor::inherited_from`** — when a field is inherited through a diamond base (`X3DNode`, `X3DGeometryNode`, etc.), `descriptors.py` records the ancestor in `inherited_from`. The thunk generator emits base-qualified calls (`X3DNode::getMetadata()`) to resolve the ambiguity, making the generated `.cpp` compile cleanly despite `public virtual` multiple inheritance.

- **`SFEnum` / `MFEnum` tags + `getEnumString` / `setEnumString`** — a bounded enum field's C++ type is a generated `enum class`, opaque to a generic codec. The two string-typed thunks let codecs round-trip the token without knowing the concrete type. Emitted by `descriptors.py._build_enum_descriptor()`.

- **`effectiveFields(node)`** (`runtime/events/DynamicField.hpp`) — runtime extension seam. When a Script node carries author `<field>` declarations, those fields are not in the generated `fields()` table. `effectiveFields()` concatenates the static table with the node's `DynamicFieldStore` entries, producing a unified `FieldTable`. Route resolution (`X3DSceneBridge::findField`) and the SAI context (`SaiContext::findField`) call `effectiveFields()`; geometry/extraction/bounds/material subsystems call `node.fields()` directly (they never see author fields — by design per `runtime/events/DynamicField.hpp` §3.1).

- **`NodeVisitor` double-dispatch** — a codec or traversal implements `NodeVisitor` and calls `node.accept(v)`. The generated `accept()` calls `visitor.enter(*this)` (returning false skips children) then `visitor.leave(*this)`. Node children are not descended into automatically — the codec visits them explicitly via the `SFNode`/`MFNode` fields in the `FieldTable`.

- **`X3DNodeFactory::create()`** — parse readers call this to instantiate a node from its type-name string. Only concrete node types are registered; abstract types return `nullptr`. `XmlReader` calls `X3DNodeFactory::create()` directly (via `XmlReaderAdapter` wrapping `codec::XmlReader`); `Vrml97Reader`, `ClassicVrmlReader`, and `JsonReader` route through the `NodeBuilder` helper.

- **`X3DInterfaceRegistry::nodeImplements()`** — some runtime systems that need type tests without `dynamic_cast` (e.g. `BindingSystem`) call this. `PickSystem` uses `node->nodeTypeName()` string comparisons rather than `nodeImplements()`. The membership tables are built once from the same UOM inheritance data used to generate the headers, so they can never drift.

## How it is tested

- **`tests/test_descriptors.py`** — unit tests for `build_descriptor()`: verifies accessor names, default expressions, constraint-check bodies, `reader_setter_call` routing (constrained vs. unconstrained, `initializeOnly`), move-overload eligibility, `outputOnly` / `inputOnly` role predicates, and `_render_range_collect`.

- **`tests/test_interface_registry.py`** — tests `interface_ids()`, `interfaces_of()`, `gen_interface_registry_header()`, `gen_interface_registry_source()`: checks that the `InterfaceId` enum contains all expected abstract types (the actual count is >60), that `TimeSensor` implements both `X3DTimeDependentNode` and `X3DSensorNode`, and that transitive closure is computed correctly (e.g. `MovieTexture` reaches `X3DChildNode` transitively).

- **`tests/test_emission.py`** — integration tests over a fully-rendered output directory: `test_reflection_header_defines_range_diagnostic()` checks the `RangeDiagnostic` struct; `test_node_emits_checkranges_and_validateranges()` confirms that `Material` emits `checkRangesSpecularColor` in its header and `out.push_back(RangeDiagnostic{…})` in its `.cpp`.

- **`tests/test_golden_tree.py`** — full-tree golden-drift test: regenerates the entire `generated_cpp_bindings/` directory (including `X3DReflection.hpp`, `X3DNodeFactory.{hpp,cpp}`, `X3DInterfaceRegistry.{hpp,cpp}`, and every node `.{hpp,cpp}`) and asserts byte-for-byte equality with the committed golden files.

- **`scripts/check_golden.sh`** — called by `test_check_golden_script_passes()` in the same file; the conformance gate runs this as an additional drift check.

- **C++ ctest** — the compiled golden files are exercised by the full runtime ctest suite (50 registered ctest targets, each a grouped doctest binary covering many cases), which links `x3d_cpp_nodes` and exercises factory creation, field get/set thunks, and visitor dispatch indirectly through every codec and extraction test.

## Related specs and ADRs

- [ADR-0003: Throw on Range](../decisions/0003-throw-on-range.md) — the decision that typed `set<Name>()` throws `std::out_of_range` while the reflection `set` thunk is non-validating (lenient read); determines the `reader_setter_call` / `setter_unchecked_name` routing in `FieldDescriptor`.
- [ADR-0005: Golden Files in Git](../decisions/0005-golden-files-in-git.md) — the commitment that generated headers (including all reflection artifacts) are committed and byte-identical; drives the golden-gate tests and the `DynamicField.hpp` side-table design.
- [ADR-0014: Dynamic Field Foundation](../decisions/0014-dynamic-field-foundation.md) — the decision to keep author `<field>` declarations in a per-node side-table (`DynamicFieldStore`) rather than modifying the generated nodes; extends the reflection surface via `effectiveFields()`.
- [Architecture](../architecture.md) — system-wide layer diagram; the reflection layer is the boundary between the generated bindings layer and the runtime core.
- [Generated Bindings](../subsystems/generated-bindings.md) — the committed golden headers that the reflection layer populates.
- [Generator](../subsystems/generator.md) — the overall generator pipeline that calls `reflection.py`, `descriptors.py`, `registry.py`, and `factory.py`.
- [Codecs / Writers](../subsystems/codecs-writers.md) — primary consumers of `FieldTable`, `NodeVisitor`, and `X3DNodeFactory`.
- [Parse / Readers](../subsystems/parse-readers.md) — use `X3DNodeFactory::create()` and the `FieldInfo::set` thunks to populate nodes during parse.
- [Routes](../subsystems/routes.md) — route resolution calls `effectiveFields()` to reach author fields in Script nodes.
- Spec: `docs/superpowers/specs/2026-06-07-lenient-read-range-warnings-design.md` — the design that introduced the `RangeDiagnostic` + `set<Name>Unchecked` / `checkRanges<Name>` / `validateRanges` split.
- Spec: `docs/superpowers/specs/2026-06-05-dynamic-field-foundation-spec.md` — the design for `DynamicFieldStore` and `effectiveFields()`.

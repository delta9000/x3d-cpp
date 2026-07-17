---
title: Generated Bindings
summary: The committed, golden-locked generated C++ node bindings that are the output artifact of the generator pipeline.
tags: [subsystem, generated, bindings, golden, namespace]
updated: 2026-07-17
related:
  - ../architecture.md
  - ../subsystems/generator.md
  - ../subsystems/reflection.md
  - ../subsystems/scene-graph.md
  - ../decisions/0005-golden-files-in-git.md
  - ../decisions/0006-compiled-static-lib.md
  - ../decisions/0039-generated-binding-namespaces.md
  - ../guides/gate-system.md
---

# Generated Bindings

## Purpose

`generated_cpp_bindings/` is the committed C++ output of the generator pipeline. It provides a strongly-typed, spec-correct C++ class for every X3D-4.0 node, a full runtime reflection layer (type-erased `FieldInfo` tables with get/set thunks), a node factory, an interface registry, and the shared field-type and enumeration headers. Every other runtime subsystem — parse readers, codec writers, the event cascade, the extraction pipeline, and the SDK façade — depends on this layer as its only representation of X3D nodes. No hand-written node class exists outside this directory.

The bindings are **not built by hand and not re-generated at compile time.** They are generated once by the Python generator (`mise run gen`) and then committed to git. That commit is the golden artifact; the `mise run golden` gate detects any generator-or-template drift before it lands.

## Namespaces and header layout (ADR-0039)

The generated types live in **two namespaces**, mirrored by two header subdirectories under
`generated_cpp_bindings/`:

- **`x3d::core`** → `generated_cpp_bindings/x3d/core/` — the value/reflection vocabulary
  (`X3Dtypes.hpp`, `X3Denums.hpp`, `X3DReflection.hpp`): `SF*/MF*` types, `FieldInfo`/
  `FieldTable`/`NodeVisitor`/`RangeDiagnostic`/`X3DFieldType`/`AccessType`, and the bounded enums.
- **`x3d::nodes`** → `generated_cpp_bindings/x3d/nodes/` — the `X3DNode` base, every abstract
  `X3D*Node`, all concrete node classes, the node factory, and the interface registry.

Headers use `#pragma once` and include each other (and are included by consumers) with a uniform
in-tree-and-installed spelling: `#include "x3d/core/X3Dtypes.hpp"`, `#include "x3d/nodes/Transform.hpp"`
(the in-tree include base is `generated_cpp_bindings/`). The one cross-namespace seam is
`x3d::core::SFNode = std::shared_ptr<x3d::nodes::X3DNode>`. Consumers qualify **asymmetrically**:
a `using namespace x3d::core;` for the small vocabulary set (scoped, never global), but explicit
`x3d::nodes::Foo` (or a per-file `namespace xn = x3d::nodes;` alias in heavy dispatch files) for
node types — no blanket `using namespace x3d::nodes`. `ctest x3d_namespace_taxonomy` pins this.

## Key files

| File / directory | Role |
|---|---|
| `x3d/core/X3Dtypes.hpp` | (`namespace x3d::core`) All X3D SF*/MF* C++ type aliases and struct definitions (`SFVec3f`, `SFColor`, `SFNode = std::shared_ptr<x3d::nodes::X3DNode>`, `MFNode`, etc.). The value structs carry C++20 defaulted `operator==`/`!=` (exact member-wise comparison — vocabulary, not math; arithmetic stays out per ADR-0012) |
| `x3d/core/X3Denums.hpp` | (`namespace x3d::core`) All bounded X3D enumeration vocabularies as `enum class` (e.g. `AlphaModeChoices`, `AccessTypeChoices`), with `to_string`/`from_string` support |
| `x3d/core/X3DReflection.hpp` | (`namespace x3d::core`) Shared reflection infrastructure: `X3DFieldType`, `AccessType`, `FieldInfo`, `FieldTable`, `RangeDiagnostic`, `NodeVisitor` |
| `x3d/nodes/X3DNode.hpp` / `.cpp` | (`namespace x3d::nodes`) Base class for all instantiable nodes: `metadata`, `DEF`/`USE`/`class`/`id`/`style` fields; virtual `nodeTypeName()`, `defaultContainerField()`, `fields()`, `accept()`, `validateRanges()` |
| `x3d/nodes/<NodeName>.hpp` / `.cpp` | (`namespace x3d::nodes`) One pair per concrete X3D node (e.g. `Appearance.hpp`/`.cpp`, `Box.hpp`/`.cpp`). 260 concrete node pairs. Each `.hpp` holds the class declaration with typed getters/setters and range-throwing validators (getters return by value, except `MFNode` getters which return `const MFNode&` to avoid copying the child vector — and its refcount bumps — on every read); each `.cpp` holds the out-of-line `fields()` FieldTable (with thunks), `accept()`, `validateRanges()`, and `checkRanges*` statics. |
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

The generated typed setters are the **data layer**, not the behavior layer.
`transform->setTranslation(v)` just assigns the member: no change event, no cascade seed.
The runtime only observes writes that go through the reflection path —
`X3DExecutionContext::writeField(node, "translation", value)` (or the `set` thunk in
`fields()`), which is what ROUTE fan-out needs to react to a change. Extraction reads
current field values each tick, so a typed-setter write still *renders* correctly — but
ROUTE-driven behavior downstream of that field silently does not fire. If behavior must
react to a write, use `ctx.writeField(...)`; use the typed setters for authoring/building
scenes before (or outside) runtime behavior. `x3d::sdk::RuntimeSession`'s CHANGELOG entry
records the same class of trap for session wiring.

### Seam points

- **Reflection write path (lenient)** — codec readers and the event cascade write field values via the reflection `set` thunk, which routes through a generated `set<Name>Unchecked()` that skips the range check. The typed public `setX()` accessor still throws `std::out_of_range` for out-of-range values; the lenient path keeps authored out-of-range values and surfaces them later via `RangeDiagnostic` (see `docs/superpowers/specs/2026-06-07-lenient-read-range-warnings-design.md`). This is the binding between the generated layer and every codec reader.

- **`NodeVisitor` double-dispatch** — writers (XML, JSON, VRML, canonical) call `node.accept(visitor)` to enter/leave each node and access its `fields()` without any per-node switch. Each concrete class's generated `.cpp` implements `accept`.

- **`X3DNodeFactory` registry** — parse readers call `X3DNodeFactory::create(typeName)` (or the free function `createX3DNode`) to instantiate any X3D node by name without including its header. The factory definitions live in `X3DNodeFactory.cpp` compiled into the `x3d_cpp_nodes` static library.

- **`X3DInterfaceRegistry`** — runtime subsystems that need to test node type membership without `dynamic_cast` (e.g. "is this node an `X3DGeometryNode`?") call `X3DInterfaceRegistry::nodeImplements(node, InterfaceId::X3DGeometryNode)`.

- **`effectiveFields()` (dynamic field extension)** — the `X3DScriptNode` subclass and the dynamic-field mechanism (`runtime/events/DynamicField.hpp`) extend the base `fields()` set at runtime for Script author-declared fields. The cascade, route resolution, and SAI layer call `effectiveFields()` rather than `fields()` to see both static and dynamic fields. This seam is in the runtime, not the generated bindings, but it depends on the generated `FieldInfo`/`FieldTable` types.

### CMake targets

The generated bindings are built as two complementary CMake targets (both defined in the root `CMakeLists.txt`):

- `x3d_cpp` (`x3d_cpp::x3d_cpp`) — INTERFACE library that adds `generated_cpp_bindings/` (and `runtime/`) to the include path. All generated `.hpp` files are included from this path by bare name.
- `x3d_cpp_nodes` (`x3d_cpp::nodes`) — STATIC library compiled from all `generated_cpp_bindings/*.cpp`. Carries the out-of-line `fields()` FieldTable definitions, `accept()`, `validateRanges()`, and the `X3DNodeFactory`/`X3DInterfaceRegistry` registries. `x3d_cpp INTERFACE` transitively links `x3d_cpp_nodes`, so consumers that link `x3d_cpp::x3d_cpp` automatically get the compiled definitions.

This decl/def split (the C1 milestone, 2026-06-16) moved the heavy `std::function` thunk instantiation out of every consumer TU and into the library, reducing cold build time from ~1296s to ~76s (~17×) and per-compile peak RSS to ~0.86 GB. See `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md` and [ADR-0006: Compiled Static Lib](../decisions/0006-compiled-static-lib.md).

## How it is tested

**Golden gate** — `mise run golden` runs `scripts/check_golden.sh`, which regenerates the full binding tree into a temp directory via `uv run x3d-cpp-gen --out <tmpdir>` and diffs every `.hpp` and `.cpp` file-by-file against the committed `generated_cpp_bindings/`. Any generator or template drift fails CI. The Python test `tests/test_golden_tree.py` runs the same check under `mise run test` (passing `--no-test` so `test.cpp` is excluded). There is no sha256 step; the gate is a plain path-set and content diff.

**Aggregate compile test** — `ctest` target `x3d_cpp_all_headers`: a CMake-generated TU that `#include`s every generated header plus the runtime umbrella (`X3DRuntime.hpp`), codecs (`X3DCodecs.hpp`), and event system headers, proving the entire set is mutually consistent in one translation unit.

**Per-header isolation compile tests** — when `X3D_CPP_PER_HEADER_CHECKS=ON` (default ON; the `dev` preset disables it for fast local iteration), each generated `.hpp` gets its own `compile_<Name>` ctest that compiles it in isolation with `-fsyntax-only`. This pins any regression to a single file. The `dev` preset leaves only the aggregate TU and the ~104 behavior/integration tests in the default `ctest --preset dev` run.

**Codec round-trip tests** — `ctest` targets including `x3d_codec_roundtrip`, `x3d_codec_roundtrip_audit`, `x3d_vrml_mf_bracket`, and the proto round-trip family exercise the generated `fields()` / `accept()` / `validateRanges()` paths indirectly through the full read→write round-trip.

**Corpus smoke** — `x3d_corpus_smoke` (bounded at 250 files) and `mise run corpus` (full 17,000+ file sweep) parse real X3D scenes through the factory and codec chain, catching regressions the unit tests miss.

## Related specs and ADRs

- [ADR-0005: Golden Files in Git](../decisions/0005-golden-files-in-git.md) — why the generated source is committed and what the golden gate enforces.
- [ADR-0006: Compiled Static Lib](../decisions/0006-compiled-static-lib.md) — the C1 decl/def split that moved reflection thunks into `x3d_cpp_nodes`.
- [Generator Pipeline](../subsystems/generator.md) — the Python pipeline that produces this directory.
- [Reflection](../subsystems/reflection.md) — the reflection emit layer and the `FieldInfo`/`FieldTable` design.
- [Scene Graph](../subsystems/scene-graph.md) — the document model that holds and traverses generated node instances.
- [Architecture](../architecture.md) — the full system map showing where the generated bindings sit in the stack.
- [Gate System](../guides/gate-system.md) — the gate infrastructure (golden / conformance / CLI differential).
- Spec: `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md` — C1 design doc.
- Spec: `docs/superpowers/specs/2026-06-07-lenient-read-range-warnings-design.md` — lenient read + `RangeDiagnostic` design.

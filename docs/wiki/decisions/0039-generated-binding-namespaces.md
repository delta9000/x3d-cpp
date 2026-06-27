---
title: "ADR-0039: Generated Bindings Live in x3d::core + x3d::nodes"
summary: The 685 generated binding types move out of the global namespace into two namespaces — x3d::core for the value/reflection vocabulary and x3d::nodes for the node classes — physically mirrored by x3d/core/ and x3d/nodes/ header subdirs (API-1 + API-2). A clean break, no compatibility shim; consumers qualify asymmetrically (a using-directive for the small core vocabulary, explicit x3d::nodes:: / a per-file alias for the 685 node types).
tags: [adr, api, namespace, generated, bindings, packaging, header-layout]
updated: 2026-06-27
related:
  - ../subsystems/generated-bindings.md
  - ../subsystems/generator.md
  - ../subsystems/sdk-facade.md
  - 0005-golden-files-in-git.md
  - 0006-compiled-static-lib.md
---

# ADR-0039: Generated Bindings Live in x3d::core + x3d::nodes

## Status

Accepted — 2026-06-27

## Context

Every generated binding was emitted into the **global namespace**: `class Appearance`,
`class Transform`, the abstract `X3D*Node` bases, the `SF*/MF*` value types, the reflection
primitives (`FieldInfo`, `FieldTable`, `NodeVisitor`), and the bounded enums — ~685 types
across 343 headers, each guarded by a bare `FOO_HPP` macro. That polluted both the **name**
and the **macro** namespace of any translation unit that included the SDK, and it was the
*one* layer that broke the codebase's own convention: the rest of the SDK is namespaced
(`x3d::codec`, `x3d::runtime`, `x3d::runtime::extract`, and the `x3d::sdk` façade). For a
library meant to be embedded, a global dump of 685 names is a real collision hazard.

API-2 was the physically entangled sibling: `install()` flattened the bindings into one bare
include dir and generated headers `#include`d each other by bare name, so namespacing the
types and relocating the headers had to happen together.

## Decision

- **Two namespaces, split by role** (mirrors the existing `x3d::*` layering):
  - **`x3d::core`** — the foundational vocabulary: `SF*/MF*` value types (`X3Dtypes`),
    reflection primitives (`FieldInfo`/`FieldTable`/`NodeVisitor`/`RangeDiagnostic`/
    `X3DFieldType`/`AccessType`, `X3DReflection`), and the bounded enums (`X3Denums`).
  - **`x3d::nodes`** — the `X3DNode` base, every abstract `X3D*Node`, all 685 concrete node
    classes, the node factory, and the interface registry.
  The value types are used far more widely than the node classes (e.g. `SFVec3f` in ~146
  runtime files vs. ~76 for the most-used node) and a `SFVec3f` is **not** a node, so a single
  `x3d::nodes` would be both semantically wrong and the maximum-churn choice.
- **Header layout mirrors the namespaces:** node headers/sources under
  `generated_cpp_bindings/x3d/nodes/`, vocabulary under `generated_cpp_bindings/x3d/core/`.
  The in-tree include base stays `generated_cpp_bindings/`, so in-tree and installed consumers
  use the same spelling: `#include "x3d/nodes/Transform.hpp"`, `#include "x3d/core/X3Dtypes.hpp"`.
  Include guards became `#pragma once` (killing the macro pollution).
- **Cross-namespace seam:** `x3d/core/X3Dtypes.hpp` forward-declares
  `namespace x3d::nodes { class X3DNode; }` and defines
  `using SFNode = std::shared_ptr<nodes::X3DNode>;` inside `x3d::core`.
- **Clean break, no compatibility shim.** No global `using namespace`, no forwarding header —
  appropriate for a pre-1.0 SDK with nothing to keep stable.
- **Asymmetric qualification by consumers.** The two namespaces are *not* referenced the same
  way. `x3d::core` is a small, stable vocabulary set, so a `using namespace x3d::core;`
  (scoped inside a named namespace in headers, or file scope in `.cpp`/tests — never global) is
  allowed and idiomatic. `x3d::nodes` gets **no blanket directive**: consumers qualify
  explicitly (`x3d::nodes::Foo`), or in heavy dispatch/visitor files that touch many node types
  (`SceneExtractor` ~37, `GeometryBounds` ~15) use a per-file alias `namespace xn = x3d::nodes;`.
  Generated node headers reference sibling node types unqualified (shared namespace) and pull
  the vocabulary via a scoped `using namespace x3d::core;`.
- The **`x3d::sdk` façade** re-exports `namespace core = x3d::core;` and
  `namespace nodes = x3d::nodes;` so the "one façade namespace" embedder story holds.

X3D string identity is unchanged (`nodeTypeName()` still returns `"Transform"`); the factory
registry, parsing, and serialization are semantically untouched. Only C++ symbols moved. The
binding-hash golden was re-blessed; the *behavioral* goldens/conformance view did not change.

## Consequences

**Positive:**

- The SDK no longer dumps 685 types + their guard macros into a consumer's global namespace;
  the generated layer now matches the project's own `x3d::*` convention.
- `namespace_taxonomy_test` (ctest `x3d_namespace_taxonomy`) pins `x3d::nodes::Transform` and
  `x3d::core::SFVec3f` placement so a future emitter change cannot silently relocate a type.
- Header path and namespace are symmetric (`x3d::nodes` ↔ `x3d/nodes/<Name>.hpp`), uniform
  in-tree and installed.

**Trade-offs / costs:**

- External embedders break by design (clean break); they qualify, or the façade aliases soften
  the common path. The new spelling is documented here and in the Generated Bindings subsystem.
- The migration touched ~190 consumer files (qualification + include paths). A scripted
  include-rewrite plus an asymmetric-qualification pass handled it; the per-header isolation
  check (CI preset) independently proves every relocated header self-contains.
- **Out-of-band tooling that read generated artifacts by hardcoded flat path had to be audited
  and fixed too** — the conformance view (`scripts/conformance_view.py`: flat read paths *and*
  source-scanner regexes that only matched unqualified `dynamic_cast`/`System<>` targets), the
  `.gitignore` exclusion of the generated smoke-test `test.cpp` (the move escaped the pattern),
  and the generator's own pytest suite (flat read paths + pre-namespace content assertions).
  These each failed a *different* CI gate after the C++ build was already green; the lesson is
  that a generated-code relocation's blast radius extends beyond CMake + C++ consumers.

## Related

- [Generated Bindings subsystem](../subsystems/generated-bindings.md) — namespace + layout
- [Generator subsystem](../subsystems/generator.md) — the emitter that produces this
- [SDK Façade subsystem](../subsystems/sdk-facade.md) — `x3d::sdk::{core,nodes}` aliases
- [ADR-0005: Golden Files in Git](0005-golden-files-in-git.md)
- [ADR-0006: Compiled Static Lib for Node Definitions](0006-compiled-static-lib.md)
- Primary implementation: `src/x3d_cpp_gen/` (emitter), `generated_cpp_bindings/x3d/{core,nodes}/`,
  `CMakeLists.txt` (layout + install), `include/x3d/sdk.hpp` (façade aliases)
- Regression test: `runtime/tests/namespace_taxonomy_test.cpp`
- Design record: `docs/superpowers/specs/2026-06-27-namespace-generated-bindings-design.md`

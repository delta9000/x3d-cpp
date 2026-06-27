# Namespace the generated bindings — `x3d::core` + `x3d::nodes` (API-1 + API-2)

- **Date:** 2026-06-27
- **Cards:** API-1 (685 generated node types live in the global namespace), API-2 (`install()` flattens 685 internal headers into one bare include dir)
- **Status:** Design approved; spec under review.

## Problem

Every generated binding is emitted into the **global namespace**. `class Appearance`,
`class Transform`, the abstract `X3D*Node` bases, the `SF*/MF*` value types, the
reflection primitives (`FieldInfo`, `FieldTable`, `NodeVisitor`) and the enums all land
at global scope — ~685 types across 343 generated headers, each guarded by a bare macro
(`APPEARANCE_HPP`). That is both **name** pollution and **macro** pollution of any
translation unit that includes the SDK.

This is the *only* layer that breaks the codebase convention: the rest of the SDK is
namespaced — `x3d::codec`, `x3d::runtime`, `x3d::runtime::extract` — and `include/x3d/sdk.hpp`
re-exports cleanly from those. The generated node layer is the lone global-scope dump.

API-2 is the physically entangled sibling: `install()` flattens `generated_cpp_bindings/`
into one bare flat include dir and generated headers `#include "X3DNode.hpp"` by bare name.
Namespacing the types pairs naturally with moving the headers into an `x3d/nodes/` subdir,
so the two are done together.

## Decisions (locked during brainstorming)

1. **Scope:** API-1 **and** API-2 together — namespace the types *and* reorganize the
   header layout / `install()` so the namespace and the physical path are symmetric
   (`namespace x3d::nodes` ↔ `x3d/nodes/Transform.hpp`).
2. **Migration:** **pure clean break, no compatibility shim.** No `x3d/nodes_global.hpp`,
   no transitional global `using namespace`. Appropriate for a pre-1.0 SDK; nothing left
   to rot. Every consumer (internal and external) qualifies.
3. **Taxonomy:** two namespaces that split *vocabulary* from *nodes*, mirroring the
   existing `x3d::*` layering:
   - **`x3d::core`** — the foundational vocabulary: `SF*/MF*` value types (`X3Dtypes`),
     reflection primitives `FieldInfo` / `FieldTable` / `NodeVisitor` / `RangeDiagnostic`
     / `X3DFieldType` / `AccessType` (`X3DReflection`), and the bounded enums (`X3Denums`).
   - **`x3d::nodes`** — the `X3DNode` base, every abstract `X3D*Node`, and all 685 concrete
     node classes.

   Rationale: the value types are used *far* more widely than node classes (`SFVec3f` in
   146 runtime files vs. ~76 for the most-used node), and a `SFVec3f` is **not** a node —
   so a single `x3d::nodes` would be both semantically wrong and the maximum-churn choice.

4. **Qualification policy is asymmetric** (resolved during planning). The two namespaces
   are *not* referenced the same way by consumers:
   - **`x3d::core`** (a small, stable vocabulary set): a `using namespace x3d::core;` is
     allowed, but only inside a named namespace block in headers (never global) or at file
     scope in `.cpp`/test TUs. Pulling in a curated vocabulary set wholesale is idiomatic.
   - **`x3d::nodes`** (685 types): **no blanket `using namespace x3d::nodes;`** — that would
     re-flatten the namespace we just built. Consumers qualify explicitly (`x3d::nodes::Foo`),
     or, in heavy dispatch/visitor files that touch many node types (`SceneExtractor` ~37,
     `GeometryBounds` ~15), use a per-file alias `namespace xn = x3d::nodes;`. Generated node
     headers reference sibling node types unqualified (same namespace). This supersedes any
     earlier "fully qualify everywhere" wording; the external API is identical either way.

## Architecture

```
generated_cpp_bindings/
  x3d/
    core/    → namespace x3d::core    (X3Dtypes, X3DReflection, X3Denums)
    nodes/   → namespace x3d::nodes    (X3DNode + abstract bases + 685 concrete)
```

In-tree include base becomes `generated_cpp_bindings/`, so in-tree and installed consumers
use the **same** spelling: `#include "x3d/nodes/Transform.hpp"`, `#include "x3d/core/X3Dtypes.hpp"`.

**Cross-namespace seam.** The one coupling is `SFNode = std::shared_ptr<X3DNode>`:
`x3d/core/X3Dtypes.hpp` forward-declares `namespace x3d::nodes { class X3DNode; }`, then
defines `namespace x3d::core { using SFNode = std::shared_ptr<nodes::X3DNode>; }`. Node
headers in `x3d::nodes` reach the vocabulary via a `using namespace x3d::core;` placed
**inside** their `namespace x3d::nodes { }` block (scoped to that namespace, never global —
see the asymmetric qualification policy). Sibling node types resolve unqualified within the
shared `x3d::nodes` namespace.

**X3D string identity is unchanged.** `nodeTypeName()` still returns `"Transform"`; the
factory/registry string→type maps, parsing, and serialization are semantically untouched.
Only the C++ symbols move. The golden *binding hash changes* (every header changes) and is
re-blessed; the *behavioral* golden/conformance output does not.

## Components & work breakdown

### A. Generator (the definitions — centralized, small, low-risk)

All 343 headers + `.cpp` + factory/registry are emitter output, so the definitions are one
focused change in `src/x3d_cpp_gen/`:

- `templates/class_template.hpp.jinja` — wrap each node header/impl in `namespace x3d::nodes { … }`.
- `generator.py` writers for `X3Dtypes.hpp` / `X3Denums.hpp` / `X3DReflection.hpp` — wrap in
  `namespace x3d::core { … }`; add the `x3d::nodes::X3DNode` forward-decl for the `SFNode` typedef.
- `emit/factory.py`, `emit/registry.py`, `emit/descriptors.py`, `emit/reflection.py`,
  `emit/naming.py` — emit into `x3d/core/` and `x3d/nodes/` subdirs, rewrite internal
  `#include`s to the new paths, and qualify cross-namespace references (`x3d::core::` from
  node code).
- Replace bare include guards with `#pragma once` (kills the macro-pollution half of the smell).
- `mise run gen` regenerates; commit the (large, expected) diff.

### B. Build & install (API-2)

- `CMakeLists.txt`: set the generated include base to `generated_cpp_bindings/` so
  `x3d/core/…` and `x3d/nodes/…` resolve in-tree (`BUILD_INTERFACE`); update the `.cpp`
  glob paths and the generated-smoke-test (`test.cpp`) path.
- `install(DIRECTORY …/generated_cpp_bindings/x3d/ DESTINATION …/x3d_cpp/x3d/)` preserves the
  subtree so installed consumers `#include "x3d/nodes/…"` identically.

### C. Consumer migration (the bulk — clean break)

- **~110** non-generated files (test dirs dominate) include a generated header by bare name →
  update those includes to `x3d/core/…` / `x3d/nodes/…` (scripted).
- Reference sites get qualified **per the asymmetric policy**: `using namespace x3d::core;`
  for the vocabulary (scoped inside the file's named namespace in headers; file scope in
  `.cpp`/tests), and for nodes **explicit `x3d::nodes::Foo`** — with a per-file
  `namespace xn = x3d::nodes;` alias only in the heavy dispatch/visitor files (`SceneExtractor`
  ~37 node types, `GeometryBounds` ~15). No blanket `using namespace x3d::nodes;` in any header.
- `include/x3d/sdk.hpp`: update its includes; add `namespace core = x3d::core;` and
  `namespace nodes = x3d::nodes;` inside `namespace x3d::sdk` so the "one façade namespace"
  story still holds for embedders.
- Executed as **reviewable per-subsystem commits** (scripted qualification + manual review),
  not one monolithic sed.

### D. Docs (in-diff, anti-drift)

- New ADR — the `x3d::core` + `x3d::nodes` taxonomy and the clean-break (no-shim) decision.
- `docs/wiki/subsystems/generated-bindings.md` — document the namespace layout + header paths.
- `docs/wiki/coverage.md` — new ADR row + Decisions count bump + narrative.
- `mkdocs.yml` — nav entry for the new ADR.
- `mise run docs-drift` pass on the change; `mise run docs-build` strict green.

## Testing & verification

- **Full `mise run ci` green** — the `ci` preset's per-header isolation check independently
  compiles every relocated header standalone, proving each self-contains under the new layout;
  453 ctests + golden (re-blessed) + conformance-gate + coverage-gate + cli-gate-regression
  all stay green.
- **Taxonomy lock test** — a focused `namespace_test.cpp` that includes only the new paths and
  asserts `x3d::nodes::Transform` and `x3d::core::SFVec3f` resolve there (regression guard
  against a future emitter change silently relocating a type).
- **Golden re-bless** — `mise run gen` + `mise run golden`; update any pinned binding-hash
  fixture. The behavioral goldens/conformance view do not change (string identity preserved).

## Risks & magnitude

- **Large, mostly-mechanical migration.** The generator change is small and contained; the
  consumer migration (84 includes, ~200 qualifications) and the golden re-bless are the bulk.
  This is a substantial multi-commit PR, not an afternoon. Mitigated by per-subsystem commits
  and the per-header isolation check catching any missed relocation.
- **Cross-namespace ordering.** The `SFNode`/`X3DNode` forward-decl seam must compile under
  the per-header isolation check (each header included first/alone). Validated by the `ci` preset.
- **External embedders break by design** (clean break). The ADR documents the new spelling;
  the façade namespace aliases soften the common path.

## Out of scope

- No behavioral change to any node, codec, runtime, or seam.
- No change to X3D string type names or the wire/serialization format.
- No compatibility shim or transitional global `using` (explicitly rejected).

# C1 — Declaration/Definition Split into a Compiled Node Library

**Date:** 2026-06-16
**Status:** Design (approved, pre-plan)
**Goal:** Cut C++ compile time by moving the heavy generated definitions out of the
per-node headers into compiled translation units, ending the header-only re-work
that every consuming TU currently pays.

## Problem

The generated bindings are header-only: each `<Node>.hpp` inlines the full class,
including `fields()` — a function-local-static `FieldTable` that pushes one
`FieldInfo` per field, each carrying up to **four `std::function`/lambda thunks**
(typed get, data set, enum string-get, enum string-set). Because everything is
inline, every TU that includes a node re-parses, re-instantiates, and re-codegens
all of it. There is no object reuse (the library is a CMake `INTERFACE` target).

A `clang -ftime-trace` profile of one factory-pulling TU
(`runtime/parse/tests/json_reader_test.cpp`, 47.8 s) quantifies the cost:

| Bucket | Share |
|--------|-------|
| Frontend | 71% |
| &nbsp;&nbsp;`InstantiateFunction` | 41% |
| &nbsp;&nbsp;`PerformPendingInstantiations` | 36% |
| &nbsp;&nbsp;`InstantiateClass` | 30% |
| &nbsp;&nbsp;`Source` (raw parse) | 26% |
| &nbsp;&nbsp;`ParseClass` | 25% |
| Backend (codegen, even at `-O0`) | 28% |

The dominant cost is **template instantiation**, not parsing. The top offenders are
all the reflection thunks:

```
std::function<std::any(const X3DNode&)>::function<lambda at .../NavigationInfo.hpp>
std::function<void(X3DNode&, const std::any&)>::function<lambda at .../TriangleSet.hpp>
std::is_invocable_r<...>, std::_Function_handler<...>, std::is_nothrow_constructible<...>
```

These `std::function` specializations + their type-traits are instantiated and
codegen'd **in every one of the ~48 factory-pulling TUs**. This is also why a
shared PCH measured ~0% (1296 s → 1260 s): a PCH avoids *re-parsing* but not
*instantiation* or *codegen*, which together are ~70%+ of the cost.

Secondary cost: `X3DNodeFactory.hpp` `#include`s 265 of the 342 node headers
(~178k LOC) to construct nodes by name, so the 48 factory-pulling TUs each parse
that whole surface. The per-TU memory of these inline-heavy compiles forces the
Ninja compile pool down to `-j4` on a 16-core / 60 GB host (OOM-kill at higher
`-j`), throttling parallelism on top of the per-TU cost.

## Goals / Non-goals

**Goals**
- Move the heavy generated definitions (the reflection `FieldTable`/thunks,
  `validateRanges()`, range checkers, `accept()`, and the factory create-table)
  out of headers into compiled `.cpp` TUs built once into a static library.
- Lean `X3DNodeFactory.hpp` (no 265-header fan-out).
- Keep runtime behavior byte-for-byte identical (it is the same code, relocated).
- Keep codegen deterministic; extend the golden gate to cover `.cpp`.

**Non-goals (this spec)**
- Replacing `std::function` thunks with lighter dispatch (a future absolute-cost
  win; out of scope — C1 only stops the per-TU repetition).
- Splitting the hand-written `runtime/` headers (codecs/events/parse/scene/extract)
  into `.cpp`. They link the node lib; their own decl/def split is separate.
- The orthogonal quick wins (raise `-j`, sample header-checks in dev, mold linker).
  Worth doing, tracked separately; not part of this change.
- C++20 modules.

## Design

### A. The per-node split (Approach 1)

The codegen emits two files per node instead of one.

`<Node>.hpp` keeps:
- the class definition and member declarations (brace-initialized to spec defaults),
- inline trivial accessors (`getX`/`setX`/`setXUnchecked`/emitters/handlers),
- the small statics (`getDefault<Field>`, `getContainerFieldType`,
  `getDefaultContainerField`, `componentName`/`componentLevel`,
  `acceptable<Field>NodeTypes`),
- **declarations** (no bodies) for the heavy members:
  `nodeTypeName()`, `defaultContainerField()`, `fields()`, `accept()`,
  `validateRanges()`, and the `protected` range-checker statics.

`<Node>.cpp` gets the **definitions** of those heavy members — chiefly `fields()`
(the `FieldTable` construction with the four thunks per field), `validateRanges()`,
the range checkers, and `accept()`. The function-local-static `FieldTable` and its
`std::function` thunks are therefore instantiated and codegen'd exactly once, in
this TU, instead of in every consumer.

Rationale for keeping trivial accessors inline: the profile shows the cost is the
`std::function` thunk instantiation, not the one-line `return member_;` accessors.
Moving them out would add boilerplate for ~no gain (rejected Approach 2).

Inheritance/virtual-override wiring is unchanged: the header still declares the
virtuals with `override`/`virtual` exactly as today; only the bodies move. The
thunks still `dynamic_cast<const Node&>(n).getX()` — the accessors they call remain
inline in the header and are reachable from the `.cpp`.

### B. Factory split (folds in C2)

- `X3DNodeFactory.hpp` → declarations only: the `create(const std::string&)` entry
  point and any registry type. No node `#include`s.
- `X3DNodeFactory.cpp` (generated) → `#include`s the node headers and builds the
  name→constructor table. Compiled once into the lib. This removes the 265-header
  parse from the ~48 factory-pulling consumer TUs.

`X3DReflection.hpp` is unchanged (it already declares `FieldInfo`/`FieldTable`/
`X3DFieldType`/`AccessType`/`NodeVisitor` and branches per nothing).

### C. Library and consumer shape

- New CMake `STATIC` target `x3d_cpp_nodes`, sources = the generated `*.cpp`
  (all `<Node>.cpp` + `X3DNodeFactory.cpp`).
- `x3d_cpp` stays the include-interface target but gains a link dependency:
  `x3d_cpp::x3d_cpp` now carries both the include dirs and `x3d_cpp_nodes`, so any
  consumer that links `x3d_cpp::x3d_cpp` (all ~66 tests + `runtime/`) links the lib
  and stops recompiling the definitions.
- Installed/shipped SDK = the headers + `libx3d_nodes.a` (plus the existing CMake
  package config). Consumers `find_package(x3d_cpp)` and link as before.
- The compile job-pool memory pressure drops sharply (consumer TUs no longer
  instantiate/codegen the thunks), so `X3D_CPP_COMPILE_JOBS` can be raised; that
  tuning is follow-up, not part of this change.

### D. Golden regime

- `scripts/check_golden.sh`: extend the two `find ... -name '*.hpp'` globs to also
  match `*.cpp` (both directions — drift, added, removed).
- `tests/test_golden_tree.py`: same extension.
- The golden hash changes once, by design (bodies move out-of-line and the headers
  shrink). Procedure unchanged: regenerate with
  `uv run x3d-cpp-gen --out generated_cpp_bindings`, review, commit.
- Determinism rules unchanged (no `set`-ordering in include emission, stable field
  order). clang-format is applied to `.cpp` as well as `.hpp`.

### E. Codegen changes

- `src/x3d_cpp_gen/templates/class_template.hpp.jinja` splits into a header template
  (declarations + inline trivia) and a new `class_template.cpp.jinja` (out-of-line
  definitions). The existing `FieldDescriptor`s already carry every field needed by
  both; no descriptor changes expected.
- `src/x3d_cpp_gen/backends/cpp_header.py` (rename concept: "cpp backend") renders
  and clang-formats both files per node.
- `src/x3d_cpp_gen/emit/factory.py` splits its output into the lean header + the
  `X3DNodeFactory.cpp` registration TU.
- `src/x3d_cpp_gen/generator.py` writes the extra `.cpp` artifacts and includes them
  in whatever manifest/`--no-test` logic it already runs. Multi-version
  (`--spec-version`/`--namespace`) paths must split identically.

### F. Validation plan

1. Behavior: full `mise run build` → ctest **459/459** green (no behavior change).
2. Golden: `mise run golden` clean after regenerate+commit (new hash recorded).
3. Compile-time A/B: cold, ccache-disabled build of all test exes, before
   (current `main`) vs after, at the **same `-j`**, to completion. Record the delta;
   we expect a large drop (the relocated instantiation/codegen now happens once).
   Optionally re-run `-ftime-trace` on the same TU to confirm the Instantiate*/
   Backend buckets collapse for consumers.
4. Memory: confirm per-consumer-TU peak RSS drops (sets up the later `-j` raise).

### G. Deferred escape hatch (not built now)

A `--header-only` codegen flag that emits today's fully-inline form, for embedders
wanting a single-include drop-in. Mentioned for the record; YAGNI until a consumer
needs it. The compiled lib is the default.

## Risks

- **Golden churn:** the one-time hash change touches every generated file. Mitigated
  by the regen+commit procedure already in place; reviewers diff intent, not bytes.
- **Static-init / ODR:** moving `fields()`'s function-local static to a `.cpp` is
  ODR-safe (one definition, one TU). The factory table is a single TU. No
  cross-TU static-init-order dependency is introduced (the `FieldTable` is still
  lazily built on first `fields()` call).
- **`dynamic_cast` reach:** thunks in the `.cpp` still see the full class definition
  via the node header it includes — no change to accessibility.
- **Consumer breakage:** anyone currently relying on header-only (no link step)
  must now link `x3d_cpp_nodes`. The CMake `x3d_cpp::x3d_cpp` target carries this
  transitively, so in-tree and `find_package` consumers are unaffected; only a
  hand-rolled `-I`-only consumer would need to add the link (documented; escape
  hatch G covers the rare case).

## Rollback

Revert the codegen template/emitter/CMake changes and regenerate — the headers
return to the inline form and the golden hash reverts. No runtime data or API
changes to unwind.

---
title: "Jinja2 Emit Templates"
summary: "Jinja2 templates that produce per-node C++ class headers, implementations, and test scaffolding from precomputed FieldDescriptor objects."
tags: [subsystem, templates, jinja2, codegen]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/generator.md
  - ../subsystems/reflection.md
  - ../subsystems/generated-bindings.md
---

# Jinja2 Emit Templates

## Purpose

The template subsystem is the final rendering stage of the generator pipeline. It holds three Jinja2 templates that are the authoritative source of the text format for every generated C++ binding: a node class header, a node class implementation, and a per-node value-asserting test. The templates are intentionally thin — no type decisions live here. All branching logic (C++ type strings, accessor names, constraint-check bodies, reflection table entries) is resolved upstream in `src/x3d_cpp_gen/emit/descriptors.py` and passed in as precomputed `FieldDescriptor` context objects. The template's job is structure and text, not semantics.

This boundary is the design invariant: when a C++ API changes (e.g., adding a move overload or a new reflection virtual), the change is made in the descriptor layer, not in the template. The template grows only when the *structure* of a generated class changes.

## Key files

| File / directory | Role |
|---|---|
| `src/x3d_cpp_gen/templates/class_template.hpp.jinja` | Per-node `<Node>.hpp`: class declaration with typed accessors (getters, setters, event handlers, emitters), static default/containerField/component helpers, acceptable-node-type vectors, reflection virtual declarations (`nodeTypeName`, `defaultContainerField`, `fields()`, `accept`, `validateRanges`), and range-checker statics. |
| `src/x3d_cpp_gen/templates/class_template.cpp.jinja` | Per-node `<Node>.cpp`: reflection virtual implementations — `nodeTypeName`, `defaultContainerField`, `fields()` (function-local static `FieldTable` built with type-erased get/set/enum thunks), `accept`, `validateRanges`, and `checkRanges<Name>` static definitions. |
| `src/x3d_cpp_gen/templates/test_template.cpp.jinja` | Single aggregate test file covering all concrete nodes: default-constructs each node, asserts every getter-with-default matches the static `getDefault<Name>()` using the generated equality operators, and runs a set of well-known explicit literal pins (e.g. `Box.size == {2,2,2}`). |
| `src/x3d_cpp_gen/backends/cpp_header.py` | `CppHeaderBackend`: loads both class templates into a Jinja2 `Environment`, runs the per-node render loop, calls clang-format in place. Resolves the packaged templates directory as `TEMPLATES_DIR` relative to the package file, never the CWD. |
| `src/x3d_cpp_gen/generator.py` | `generate_test_file()`: loads the test template into a separate `Environment`, builds per-node context from concrete nodes only, renders `test.cpp`. |

## Interfaces and seams

### Exposed interface

The subsystem is consumed indirectly: callers instantiate `CppHeaderBackend` and call `emit()`, or call `generate_test_file()`; neither caller touches templates directly.

```python
# src/x3d_cpp_gen/backends/cpp_header.py

class CppHeaderBackend:
    def __init__(
        self,
        templates_dir=None,          # overrides packaged dir; defaults to TEMPLATES_DIR
        clang_format: str = "clang-format",
        enum_defs=None,
):...

    def emit(
        self,
        nodes: Dict[str, X3DNode],
        dependency_graph: Dict[str, List[str]],
        out_dir: str,
) -> None:...
```

```python
# src/x3d_cpp_gen/generator.py

def generate_test_file(
    nodes: Dict[str, X3DNode],
    output_dir: str,
    templates_dir=None,
    enum_defs=None,
) -> str:   # returns path to written test.cpp
...
```

### Template context variables

`class_template.hpp.jinja` and `class_template.cpp.jinja` always emit node
classes in `x3d::nodes`. They receive a per-node render context assembled in
`CppHeaderBackend.emit()`:

| Variable | Type | Source |
|---|---|---|
| `class_name` | `str` | `node.name` |
| `fields()` | `List[FieldDescriptor]` | own fields only — drives accessor emission |
| `reflection_fields` | `List[FieldDescriptor]` | full field set (own + inherited, phantom fields dropped) — drives `FieldTable` |
| `is_root` | `bool` | `node.name == "X3DNode"` — controls `virtual` vs `override` on reflection declarations |
| `emit_reflection` | `bool` | true if node transitively inherits from `X3DNode` (mixin object-types get false) |
| `virtual_bases` | `List[str]` | all direct bases emitted as `public virtual` |
| `dependencies` | `List[str]` | transitive include list for `#include "Dep.hpp"` |
| `is_abstract` | `bool` | abstract nodes get `virtual ~Cls() = default` |
| `class_description` | `str` | Doxygen `@brief` |
| `specification_url` | `str` | Doxygen `@details` |
| `container_field` | object or None | static `getContainerFieldType` / `getDefaultContainerField` / `isValidContainerField` |
| `component` | object or None | static `componentName` / `componentLevel` |

The `FieldDescriptor` objects passed as `fields()` and `reflection_fields` are produced by `build_descriptors()` in `src/x3d_cpp_gen/emit/descriptors.py`. The template only reads predicate properties (`is_readable`, `is_readonly`, `is_event`, `is_settable`, `is_enum`, `has_member`, `has_default`, `has_constraints`, `has_data_setter`, `has_acceptable_node_types`) and string properties (`getter_name`, `setter_name`, `handler_name`, `emitter_name`, `setter_unchecked_name`, `handler_setter_name`, `handler_member`, `validator_name`, `range_checker_name`, `range_collect_body`, `constraint_checks`, `getter_call`, `reader_setter_call`, `handler_call`, `emitter_call`, `runtime_field_type`, `runtime_access`, `container_field_default`, `enum_cpp_name`) — the template never branches on raw type strings.

`test_template.cpp.jinja` receives:

| Variable | Type | Source |
|---|---|---|
| `concrete_nodes` | `List[X3DNode]` | non-abstract nodes; drives `#include "<Node>.hpp"` |
| `nodes_ctx` | `List[dict]` | per-node `{name, default_fields}` list for the assertion loop |
| `well_known` | `List[dict]` | explicit literal pins `{node, expr, desc}` (filtered to present+concrete nodes) |

### Seam points

- `templates_dir` override — both `CppHeaderBackend.__init__` and `generate_test_file()` accept a `templates_dir` path. The CLI surfaces this as `--templates` (see `src/x3d_cpp_gen/cli.py`). When omitted the packaged `TEMPLATES_DIR` constant is used, resolved as `Path(__file__).resolve().parent.parent / "templates"` inside `cpp_header.py`.
- `jinja2.ext.loopcontrols` extension — enabled in the `CppHeaderBackend` environment (`backends/cpp_header.py`), which makes `{% break %}` / `{% continue %}` available in loops. (The current class templates do not actually use them — the extension is enabled defensively; no `.jinja` template contains a `break`/`continue` tag.)
- `pascal` filter — the Jinja2 environment in `CppHeaderBackend` registers `emit.naming.pascal` as a template filter (`env.filters['pascal'] = pascal`). The test template's environment (in `generate_test_file()`) does not register extra filters.
- clang-format post-pass — `CppHeaderBackend._format()` runs `clang-format -i` on each emitted `.hpp` and `.cpp` after rendering. If clang-format is absent the backend prints a warning and skips formatting for all remaining files (degrades gracefully, but output diverges from the committed golden baseline).
- `emit_reflection` gate — mixin abstract object types (e.g. `X3DBoundedObject`) do not reach `X3DNode` in the inheritance graph, so `emit_reflection=False` is passed for them; the template skips the reflection virtual block entirely. This is what prevents the `fields()` ambiguity on diamond-inheriting concrete nodes.

## Related

- [Generator Pipeline](../subsystems/generator.md)
- [Reflection](../subsystems/reflection.md)
- [Generated Bindings](../subsystems/generated-bindings.md)
- [Architecture](../architecture.md)
- [ADR-0005: Golden Files in Git](../decisions/0005-golden-files-in-git.md)
- [ADR-0006: Compiled Static Lib](../decisions/0006-compiled-static-lib.md)

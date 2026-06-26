---
title: Generator Pipeline
summary: Reads the X3D Unified Object Model (UOM) XML and emits spec-correct C++ bindings via a parse → model → emit pipeline.
tags: [subsystem, generator, codegen, python]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/generated-bindings.md
  - ../subsystems/templates.md
  - ../subsystems/reflection.md
---

# Generator Pipeline

## Purpose

The generator pipeline is the Python-side code-generation layer. It reads the X3D Unified Object Model (UOM) XML specification, builds an in-memory IR of every node and field, resolves the full inheritance graph, and writes a set of C++ source files that are committed as the golden binding tree. Every downstream runtime capability — reflection, the node factory, the interface registry, range constraints, bounded enumerations — is emitted here from data, not from hand-written code. Running the pipeline again from the same UOM XML must produce byte-identical output (enforced by the golden gate).

The pipeline owns everything from the spec XML to the `.hpp`/`.cpp` files in `generated_cpp_bindings/`. The runtime (`runtime/`) consumes those files as a compiled static library but never imports from this layer.

## Key files

| File / directory | Role |
|---|---|
| `src/x3d_cpp_gen/generator.py` | Top-level orchestrator: `FIELD_TYPE_MAPPING`, `XS_TYPES`, type-header / enum-header / reflection-header / factory / interface-registry writers, and the `generate_cpp_bindings` / `generate_test_file` entry points |
| `src/x3d_cpp_gen/parser.py` | UOM XML parser: `X3DField`, `X3DNode` dataclasses; `parse_x3d_model`, `parse_enum_definitions`, `build_dependency_graph`, `resolve_inheritance_chain`, `get_own_fields` |
| `src/x3d_cpp_gen/backends/cpp_header.py` | `CppHeaderBackend`: per-node rendering loop; drives `build_descriptors`, resolves diamond-inheritance qualifiers, invokes clang-format, writes `<Node>.hpp` + `<Node>.cpp` |
| `src/x3d_cpp_gen/emit/descriptors.py` | `FieldDescriptor` + `build_descriptor(s)`: computes every per-field C++ decision (type, accessor names, default literal, constraint body, range-collect body, move-overload eligibility) |
| `src/x3d_cpp_gen/emit/naming.py` | `pascal()`, `sanitize_field_name()`: single naming canonicalizer shared by parser and emitter |
| `src/x3d_cpp_gen/emit/defaults.py` | `default_expr_for()`, `enum_default_expr()`, `tokenize_mfstring()`, `cpp_string_literal()`: default C++ initializer expression computation |
| `src/x3d_cpp_gen/emit/reflection.py` | `gen_reflection_header()`: emits `X3DReflection.hpp` (the `X3DFieldType` enum, `FieldInfo`, `FieldTable`, `RangeDiagnostic`, `NodeVisitor`) data-driven from `X3DType` |
| `src/x3d_cpp_gen/emit/factory.py` | `gen_node_factory_header/source()`: emits `X3DNodeFactory.hpp/.cpp` |
| `src/x3d_cpp_gen/emit/registry.py` | `gen_interface_registry_header/source()`: emits `X3DInterfaceRegistry.hpp/.cpp` |
| `src/x3d_cpp_gen/model/types.py` | `X3DType` enum, `TypeRegistry`: canonical C++ type mapping, component layout, move-overload eligibility, runtime tag for every X3D field type |
| `src/x3d_cpp_gen/model/enums.py` | `EnumDef`, `EnumMember`, `parse_enum_defs()`: parse bounded `<SimpleType>` enumerations from the UOM into named C++ `enum class` definitions |
| `src/x3d_cpp_gen/model/version.py` | `SpecVersion`: auto-detect spec version from the UOM root attribute or fallback comment; threaded through parse → emit |
| `src/x3d_cpp_gen/templates/` | Jinja2 templates — `class_template.hpp.jinja`, `class_template.cpp.jinja`, `test_template.cpp.jinja` — see [Templates](templates.md) |
| `src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml` | Packaged default UOM spec; resolved via `importlib.resources`, never relative to CWD |
| `src/x3d_cpp_gen/cli.py` | `x3d-cpp-gen` CLI entry point: wires parser → generator → `write_*` calls; `--out`, `--spec`, `--no-test`, `--namespace` flags |

## Interfaces and seams

### Exposed interface

The pipeline's public surface for callers (the CLI and tests):

```python
# Parse UOM XML -> dict of X3DNode objects
nodes: Dict[str, X3DNode] = parse_x3d_model(uom_file, FIELD_TYPE_MAPPING, XS_TYPES)

# Build inheritance adjacency graph (ordered lists for determinism)
graph: Dict[str, List[str]] = build_dependency_graph(nodes)

# Parse bounded SimpleType enumerations
enum_defs: Dict[str, EnumDef] = parse_enum_definitions(uom_file)

# Emit per-node .hpp + .cpp (delegates to CppHeaderBackend)
generate_cpp_bindings(nodes, graph, output_dir,
                      clang_format="clang-format", enum_defs=enum_defs,
                      namespace="")

# Emit shared headers
write_types_header(output_dir)       # X3Dtypes.hpp
write_enums_header(output_dir, enum_defs)  # X3Denums.hpp
write_reflection_header(output_dir)  # X3DReflection.hpp
write_node_factory(output_dir, nodes)          # X3DNodeFactory.hpp + .cpp
write_interface_registry(output_dir, nodes, graph)  # X3DInterfaceRegistry.hpp + .cpp

# Emit a self-asserting smoke-test driver
generate_test_file(nodes, output_dir, enum_defs=enum_defs)  # test.cpp
```

The `CppHeaderBackend.emit(nodes, graph, out_dir)` method is the render loop: for each `X3DNode` it calls `build_descriptors(get_own_fields(node), enum_defs)` for own-field descriptors and `build_descriptors(node.fields, enum_defs)` for the full reflection table, resolves diamond-inheritance qualifiers for each reflected field, renders `class_template.hpp.jinja` and `class_template.cpp.jinja`, then runs clang-format in place.

### Seam points

- **UOM XML path** — callers supply the spec file; the packaged default is `src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml`. A newer spec version is ingested without code changes: `SpecVersion.detect()` reads the `version` attribute from the XML root automatically.

- **Jinja2 templates** — `CppHeaderBackend` loads templates from `src/x3d_cpp_gen/templates/` (resolved via `TEMPLATES_DIR = Path(__file__).resolve().parent.parent / "templates"`). Test and CLI callers can override `templates_dir` to substitute custom templates.

- **clang-format** — `CppHeaderBackend._format()` runs `clang-format -i` in place after each file. Degrades gracefully: if the binary is absent the file is left unformatted (golden byte-equality requires clang-format). Callers pass `clang_format=""` to disable (e.g. in unit tests).

- **Namespace wrapping** — `generate_cpp_bindings(..., namespace="")` accepts an optional C++ namespace name. The empty default keeps output byte-identical to the 4.0 golden baseline. The Jinja template wraps each class in `namespace { ... }` only when non-empty.

- **`FIELD_TYPE_MAPPING` / `XS_TYPES`** — the type allowlists in `generator.py` act as a seam: adding a new UOM field type requires an entry here (and in `model/types.py`) before the parser will accept it. Unknown types are logged and skipped rather than crashing.

- **`get_own_fields(node)`** — the parser seam between the UOM's flattened field list (every inherited field present on each node) and the emitter's per-class output. Only own fields (those without `field/@inheritedFrom`) get members and accessors on the class; inherited ones are provided via C++ inheritance.

- **`build_descriptors`** — the IR boundary between parser dataclasses and the render layer. The Jinja template consumes `FieldDescriptor` objects only; it never branches on raw type strings.

## How it is tested

- `uv run pytest tests/test_parser.py` — unit tests for `parse_x3d_model`, `build_dependency_graph`, `get_own_fields`, `X3DNode`/`X3DField` dataclasses.

- `uv run pytest tests/test_descriptors.py` — unit tests for `build_descriptor` / `build_descriptors`: type resolution, constraint rendering, enum-descriptor path.

- `uv run pytest tests/test_defaults.py` + `tests/test_default_expr.py` — default-literal generation for every X3D type.

- `uv run pytest tests/test_naming.py` — `pascal()` and `sanitize_field_name()` edge cases including C++ reserved keywords.

- `uv run pytest tests/test_enums.py` — bounded SimpleType parsing: `parse_enum_defs`, member sanitization, de-collision.

- `uv run pytest tests/test_emission.py` — end-to-end render tests against the packaged 4.0 spec. Asserts specific C++ text in emitted headers (e.g. `Appearance.hpp` carries `AlphaModeChoices getAlphaMode()` and not a bare `std::string` member).

- `uv run pytest tests/test_golden_smoke.py` — byte-for-byte comparison of two representative headers (`Box.hpp`, `X3Dtypes.hpp`) against committed golden files. Skipped if clang-format is absent.

- `uv run pytest tests/test_golden_tree.py` — full-tree golden-drift test: runs the CLI into a temp dir and diffs every `*.hpp` against `generated_cpp_bindings/` (both directions: no missing, no extra, no drifted files). This is the primary regression gate for any pipeline change.

- `uv run pytest tests/test_interface_registry.py` — `X3DInterfaceRegistry` generation: verifies interface sets are computed from the transitive closure of the inheritance graph.

- `mise run golden` — shell-based drift gate (`scripts/check_golden.sh`); equivalent to the golden-tree pytest but usable standalone.

- `mise run gen` — regenerates `generated_cpp_bindings/` from the packaged 4.0 UOM. Must be run (and output committed) whenever a template or emitter changes.

## Related specs and ADRs

- [Architecture](../architecture.md)
- [Templates](templates.md)
- [Generated Bindings](generated-bindings.md)
- [Reflection](reflection.md)
- Spec (C1 decl/def split, static-lib motivation): `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md`
- ADR (golden files in git): [ADR-0005: Golden Files in Git](../decisions/0005-golden-files-in-git.md)
- ADR (virtual mixins / additional inheritance): [ADR-0004: Virtual Mixins](../decisions/0004-virtual-mixins.md)
- ADR (bounded enum `enum class` emission): see `src/x3d_cpp_gen/model/enums.py` and `src/x3d_cpp_gen/generator.py` (`gen_enums_header`)

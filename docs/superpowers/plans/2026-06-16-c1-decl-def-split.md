# C1 — Declaration/Definition Split into a Compiled Node Library — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the heavy generated definitions (the reflection `fields()` FieldTable + `std::function` thunks, `validateRanges()`, the `checkRanges*` statics, `accept()`, and the `X3DNodeFactory` registry) out of the per-node headers into generated `.cpp` files compiled once into a static library, so consumers stop re-instantiating and re-codegen-ing them per TU.

**Architecture:** The codegen emits a lean `<Node>.hpp` (class + members + inline trivial accessors + declarations of the heavy virtuals) plus a `<Node>.cpp` (out-of-line definitions). `X3DNodeFactory.hpp` becomes declarations-only; `X3DNodeFactory.cpp` holds the node-including registry. A new CMake `STATIC` target `x3d_cpp_nodes` compiles all generated `*.cpp` once; `x3d_cpp::x3d_cpp` links it transitively, so all tests and `runtime/` link the lib instead of recompiling the definitions. Behavior is byte-for-byte identical at runtime — the same code, relocated out-of-line.

**Tech Stack:** Python 3 codegen (Jinja2 templates, `uv`), CMake + Ninja + ctest, clang-format (golden formatting), pytest (generator tests).

**Reference spec:** `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md`

---

## File Structure

**Codegen (Python):**
- `src/x3d_cpp_gen/templates/class_template.hpp.jinja` — MODIFY: replace the out-of-line bodies of `nodeTypeName`/`defaultContainerField`/`fields`/`accept`/`validateRanges` and the `checkRanges*` statics with declarations; keep all inline trivia (accessors, `getDefault*`, container statics, `acceptable*NodeTypes`, throwing `validate*` validators, members).
- `src/x3d_cpp_gen/templates/class_template.cpp.jinja` — CREATE: the out-of-line definitions, qualified `<Node>::`, wrapped in the optional namespace.
- `src/x3d_cpp_gen/backends/cpp_header.py` — MODIFY: render + clang-format both `<Node>.hpp` and `<Node>.cpp`.
- `src/x3d_cpp_gen/emit/factory.py` — MODIFY: split into `gen_node_factory_header` (lean, no node includes) + `gen_node_factory_source` (the registry).
- `src/x3d_cpp_gen/generator.py` — MODIFY: `write_node_factory` writes both `.hpp` and `.cpp`.

**Build / golden:**
- `CMakeLists.txt` — MODIFY: add `x3d_cpp_nodes` STATIC lib over `generated_cpp_bindings/*.cpp`; `target_link_libraries(x3d_cpp INTERFACE x3d_cpp_nodes)`; install it.
- `scripts/check_golden.sh` — MODIFY: golden globs `*.hpp` **and** `*.cpp`.
- `tests/test_golden_tree.py` — MODIFY: same, plus a no-node-includes assertion.

**Generated output (regenerated + committed):**
- `generated_cpp_bindings/*.hpp` — shrink (bodies removed).
- `generated_cpp_bindings/*.cpp` — NEW: one per node + `X3DNodeFactory.cpp`.

**Tests (Python):**
- `tests/test_decl_def_split.py` — CREATE: assertions on the new emission shape.

---

## Task 1: Extend the golden gate to cover `.cpp`

Do this first so the new `.cpp` artifacts are gated the moment they appear. With no `.cpp` present yet, both checks still pass on the current tree.

**Files:**
- Modify: `scripts/check_golden.sh`
- Modify: `tests/test_golden_tree.py:test_golden_tree_matches`

- [ ] **Step 1: Generalize the two `find` globs in `check_golden.sh`**

In `scripts/check_golden.sh`, change the two `find ... -name '*.hpp' -print0` invocations to match both extensions. Replace:

```bash
done < <(find "${GOLDEN_DIR}" -name '*.hpp' -print0)
```
with
```bash
done < <(find "${GOLDEN_DIR}" \( -name '*.hpp' -o -name '*.cpp' \) -print0)
```

and replace:
```bash
done < <(find "${TMP_DIR}" -name '*.hpp' -print0)
```
with
```bash
done < <(find "${TMP_DIR}" \( -name '*.hpp' -o -name '*.cpp' \) -print0)
```

- [ ] **Step 2: Generalize the globs in `test_golden_tree.py`**

In `tests/test_golden_tree.py::test_golden_tree_matches`, replace:

```python
    golden = {p.relative_to(GOLDEN_DIR) for p in GOLDEN_DIR.rglob("*.hpp")}
    produced = {p.relative_to(out) for p in out.rglob("*.hpp")}
```
with
```python
    def _tree(root):
        return {p.relative_to(root)
                for ext in ("*.hpp", "*.cpp")
                for p in root.rglob(ext)}
    golden = _tree(GOLDEN_DIR)
    produced = _tree(out)
```

- [ ] **Step 3: Verify the gate still passes on the current (all-`.hpp`) tree**

Run: `uv run pytest tests/test_golden_tree.py -q`
Expected: PASS (no `.cpp` exist yet, so both trees are identical sets of `.hpp`).

Run: `bash scripts/check_golden.sh`
Expected: `Golden tree OK: ... byte-for-byte.`

- [ ] **Step 4: Commit**

```bash
git add scripts/check_golden.sh tests/test_golden_tree.py
git commit -m "golden: gate *.cpp alongside *.hpp (prep for C1 decl/def split)"
```

---

## Task 2: Split the node factory into a lean header + a source TU

**Files:**
- Modify: `src/x3d_cpp_gen/emit/factory.py`
- Modify: `src/x3d_cpp_gen/generator.py:286-292` (`write_node_factory`)
- Create: `tests/test_decl_def_split.py`

- [ ] **Step 1: Write the failing test**

Create `tests/test_decl_def_split.py`:

```python
"""Tests for the C1 declaration/definition split."""
from x3d_cpp_gen.emit.factory import (
    gen_node_factory_header, gen_node_factory_source,
)
from x3d_cpp_gen.parser import X3DNode


def _two_nodes():
    box = X3DNode(name="Box"); box.is_abstract = False
    abs = X3DNode(name="X3DGeometryNode"); abs.is_abstract = True
    return {"Box": box, "X3DGeometryNode": abs}


def test_factory_header_has_no_node_includes():
    hdr = gen_node_factory_header(_two_nodes())
    assert '#include "Box.hpp"' not in hdr
    assert "X3DNodeFactory" in hdr            # class still declared
    assert "create(" in hdr                   # entry point still declared
    assert "{ return std::make_shared" not in hdr   # registry body NOT inline


def test_factory_source_includes_concrete_nodes_and_defines_registry():
    src = gen_node_factory_source(_two_nodes())
    assert '#include "X3DNodeFactory.hpp"' in src
    assert '#include "Box.hpp"' in src                       # concrete included
    assert '#include "X3DGeometryNode.hpp"' not in src       # abstract excluded
    assert "X3DNodeFactory::registry()" in src               # out-of-line def
    assert "std::make_shared<Box>()" in src
```

Note: if the `X3DNode` constructor signature differs, adjust `_two_nodes()` to match `src/x3d_cpp_gen/parser.py` (check `class X3DNode`). The assertions are the contract.

- [ ] **Step 2: Run it to confirm it fails**

Run: `uv run pytest tests/test_decl_def_split.py -q`
Expected: FAIL — `cannot import name 'gen_node_factory_source'`.

- [ ] **Step 3: Rewrite `emit/factory.py` to split header and source**

Replace the body of `gen_node_factory_header` so the header is declarations-only (no node includes, no inline registry), and add `gen_node_factory_source`:

```python
def gen_node_factory_header(nodes: Dict[str, X3DNode]) -> str:
    """Render the declarations-only ``X3DNodeFactory.hpp`` (no node includes)."""
    lines: List[str] = []
    lines.append("// X3DNodeFactory.hpp")
    lines.append("// Auto-generated: maps an X3D node-type name to a fresh node instance.")
    lines.append("#ifndef X3D_NODE_FACTORY_HPP")
    lines.append("#define X3D_NODE_FACTORY_HPP")
    lines.append("")
    lines.append("#include <functional>")
    lines.append("#include <memory>")
    lines.append("#include <string>")
    lines.append("#include <unordered_map>")
    lines.append("")
    lines.append("class X3DNode;")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief Registry of concrete X3D node constructors, keyed by X3D type name.")
    lines.append(" * @details Definitions live in X3DNodeFactory.cpp (compiled into the node")
    lines.append(" *          library) so consumers do not parse every node header.")
    lines.append(" */")
    lines.append("class X3DNodeFactory {")
    lines.append("public:")
    lines.append("    using Creator = std::function<std::shared_ptr<X3DNode>()>;")
    lines.append("    /// The full name -> creator map (built once).")
    lines.append("    static const std::unordered_map<std::string, Creator>& registry();")
    lines.append("    /// Create a node by X3D type name, or nullptr if the name is unknown.")
    lines.append("    static std::shared_ptr<X3DNode> create(const std::string& typeName);")
    lines.append("};")
    lines.append("")
    lines.append("/// Convenience free function mirroring X3DNodeFactory::create.")
    lines.append("std::shared_ptr<X3DNode> createX3DNode(const std::string& typeName);")
    lines.append("")
    lines.append("#endif // X3D_NODE_FACTORY_HPP")
    lines.append("")
    return "\n".join(lines)


def gen_node_factory_source(nodes: Dict[str, X3DNode]) -> str:
    """Render ``X3DNodeFactory.cpp``: includes concrete nodes + defines the registry."""
    concrete: List[str] = sorted(
        n.name for n in nodes.values() if not n.is_abstract
    )
    lines: List[str] = []
    lines.append("// X3DNodeFactory.cpp")
    lines.append("// Auto-generated: the registry definition (compiled once into the node lib).")
    lines.append('#include "X3DNodeFactory.hpp"')
    lines.append("")
    lines.append('#include "X3DNode.hpp"')
    for name in concrete:
        lines.append(f'#include "{name}.hpp"')
    lines.append("")
    lines.append("const std::unordered_map<std::string, X3DNodeFactory::Creator>&")
    lines.append("X3DNodeFactory::registry() {")
    lines.append("    static const std::unordered_map<std::string, Creator> reg = {")
    for name in concrete:
        lines.append(
            f'        {{"{name}", [] {{ return std::make_shared<{name}>(); }}}},'
        )
    lines.append("    };")
    lines.append("    return reg;")
    lines.append("}")
    lines.append("")
    lines.append("std::shared_ptr<X3DNode> X3DNodeFactory::create(const std::string& typeName) {")
    lines.append("    const auto& reg = registry();")
    lines.append("    auto it = reg.find(typeName);")
    lines.append("    return it == reg.end() ? nullptr : it->second();")
    lines.append("}")
    lines.append("")
    lines.append("std::shared_ptr<X3DNode> createX3DNode(const std::string& typeName) {")
    lines.append("    return X3DNodeFactory::create(typeName);")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)
```

- [ ] **Step 4: Make the generator write both factory files**

In `src/x3d_cpp_gen/generator.py`, replace `write_node_factory` (lines ~286-292):

```python
def write_node_factory(output_dir: str, nodes: Dict[str, X3DNode]) -> None:
    """Write X3DNodeFactory.hpp (declarations) + X3DNodeFactory.cpp (registry)."""
    from x3d_cpp_gen.emit.factory import (
        gen_node_factory_header, gen_node_factory_source,
    )
    hdr = os.path.join(output_dir, "X3DNodeFactory.hpp")
    with open(hdr, "w") as f:
        f.write(gen_node_factory_header(nodes))
    print(f"Generated X3D node factory header file at {hdr}")
    src = os.path.join(output_dir, "X3DNodeFactory.cpp")
    with open(src, "w") as f:
        f.write(gen_node_factory_source(nodes))
    print(f"Generated X3D node factory source file at {src}")
```

If a clang-format pass is applied to generated files elsewhere in `generator.py`, route both new files through it the same way the existing factory header was formatted (search `generator.py` for `clang-format` / `_format`; if the factory header was not previously clang-formatted, leave both unformatted to match).

- [ ] **Step 5: Run the unit tests**

Run: `uv run pytest tests/test_decl_def_split.py -q`
Expected: PASS.

- [ ] **Step 6: Regenerate the tree**

Run: `uv run x3d-cpp-gen --out generated_cpp_bindings --no-test`
Expected: produces `generated_cpp_bindings/X3DNodeFactory.cpp` and a slimmed `X3DNodeFactory.hpp` with no node includes.

- [ ] **Step 7: Commit (codegen only; build wiring is Task 3)**

```bash
git add src/x3d_cpp_gen/emit/factory.py src/x3d_cpp_gen/generator.py \
        tests/test_decl_def_split.py \
        generated_cpp_bindings/X3DNodeFactory.hpp generated_cpp_bindings/X3DNodeFactory.cpp
git commit -m "codegen: split X3DNodeFactory into lean header + registry .cpp (C2)"
```

---

## Task 3: Build the generated `.cpp` into a static library and link it

**Files:**
- Modify: `CMakeLists.txt` (library section near `add_library(x3d_cpp INTERFACE)`, ~line 43-64; install section ~69-105)

- [ ] **Step 1: Define the static node library and link it through the interface**

In `CMakeLists.txt`, immediately AFTER the `target_compile_features(x3d_cpp INTERFACE cxx_std_20)` line (~line 64), add:

```cmake
# ---------------------------------------------------------------------------
# Compiled node library (C1): the generated *.cpp (per-node out-of-line
# reflection/validate definitions + the X3DNodeFactory registry) are compiled
# ONCE here instead of re-instantiated in every consumer TU. Consumers link
# this transitively via x3d_cpp::x3d_cpp.
# ---------------------------------------------------------------------------
file(GLOB X3D_CPP_NODE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings/*.cpp")
add_library(x3d_cpp_nodes STATIC ${X3D_CPP_NODE_SOURCES})
add_library(x3d_cpp::nodes ALIAS x3d_cpp_nodes)
target_link_libraries(x3d_cpp_nodes PUBLIC x3d_cpp)
target_compile_features(x3d_cpp_nodes PUBLIC cxx_std_20)
# Consumers that link the header interface also link the compiled definitions.
target_link_libraries(x3d_cpp INTERFACE x3d_cpp_nodes)
```

- [ ] **Step 2: Install the compiled library**

In the install section, change the existing `install(TARGETS x3d_cpp EXPORT x3d_cppTargets)` to include the node lib:

```cmake
install(TARGETS x3d_cpp x3d_cpp_nodes
    EXPORT x3d_cppTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
```

- [ ] **Step 3: Configure + build the node lib alone to confirm it compiles**

Run: `cmake --preset dev && cmake --build build --target x3d_cpp_nodes`
Expected: builds `libx3d_cpp_nodes.a` with no errors (it currently contains only `X3DNodeFactory.cpp`).

- [ ] **Step 4: Full build + ctest**

Run: `mise run build`
Expected: configures, builds, ctest **459/459 passed**. (If any test TU relied on transitively getting a node header through the old `X3DNodeFactory.hpp`, it will fail to compile here — fix by adding the explicit `#include "<Node>.hpp"` to that test; this is the expected, surfaced consequence of the lean factory header.)

- [ ] **Step 5: Golden gate**

Run: `mise run golden`
Expected: `Golden tree OK`. (The factory `.cpp`/`.hpp` are committed from Task 2 and gated by Task 1.)

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: compile generated *.cpp into x3d_cpp_nodes static lib, link via interface"
```

---

## Task 4: Split each node into a lean header + an out-of-line `.cpp`

**Files:**
- Modify: `src/x3d_cpp_gen/templates/class_template.hpp.jinja`
- Create: `src/x3d_cpp_gen/templates/class_template.cpp.jinja`
- Modify: `src/x3d_cpp_gen/backends/cpp_header.py:79-179` (`emit`)
- Modify: `tests/test_decl_def_split.py`

- [ ] **Step 1: Add failing tests for the per-node split**

Append to `tests/test_decl_def_split.py`:

```python
import subprocess, sys
from pathlib import Path


def test_node_header_declares_fields_without_inline_body(tmp_path):
    out = tmp_path / "gen"
    out.mkdir()
    subprocess.run([sys.executable, "-m", "x3d_cpp_gen.cli",
                    "--out", str(out), "--no-test"], check=True)
    box_h = (out / "Box.hpp").read_text()
    box_c = (out / "Box.cpp").read_text()
    # Header DECLARES fields() but does NOT inline its FieldTable body.
    assert "const FieldTable& fields() const override;" in box_h
    assert "static const FieldTable table" not in box_h
    # The out-of-line definition lives in the .cpp, class-qualified.
    assert "Box::fields()" in box_c
    assert "FieldTable" in box_c
    # Trivial accessors stay inline in the header.
    assert "getSize()" in box_h or "getDef" in box_h
```

- [ ] **Step 2: Run to confirm it fails**

Run: `uv run pytest tests/test_decl_def_split.py::test_node_header_declares_fields_without_inline_body -q`
Expected: FAIL — `Box.cpp` not produced / `fields()` still inline.

- [ ] **Step 3: Convert the heavy bodies in the header template to declarations**

In `src/x3d_cpp_gen/templates/class_template.hpp.jinja`, inside the `{% if emit_reflection %}` block, replace the five inline method bodies with declarations. Concretely:

`nodeTypeName`:
```jinja
    {{ "virtual " if is_root else "" }}std::string nodeTypeName() const{{ " override" if not is_root else "" }};
```
`defaultContainerField`:
```jinja
    {{ "virtual " if is_root else "" }}std::string defaultContainerField() const{{ " override" if not is_root else "" }};
```
`fields`:
```jinja
    {{ "virtual " if is_root else "" }}const FieldTable& fields() const{{ " override" if not is_root else "" }};
```
`accept`:
```jinja
    {{ "virtual " if is_root else "" }}void accept(NodeVisitor& visitor) const{{ " override" if not is_root else "" }};
```
`validateRanges` — both branches become declarations:
```jinja
    {% if is_root %}
    virtual void validateRanges(std::vector<RangeDiagnostic>& out) const;
    {% elif _constrained %}
    void validateRanges(std::vector<RangeDiagnostic>& out) const override;
    {% endif %}
```
And the `protected:` `checkRanges*` statics become declarations (keep the `protected:` label and the loop, drop the body):
```jinja
protected:
    {% for field in _own_constrained %}
    static void {{ field.range_checker_name }}(const {{ field.cpp_type }}& value,
            const std::string& nodeType, const std::string& defName,
            std::vector<RangeDiagnostic>& out);
    {% endfor %}
```
Leave everything else in the header untouched: the inline getters/setters/unchecked-setters/emitters/handlers, the `getDefault*`/container/`acceptable*NodeTypes` statics, the **private throwing `validate*` validators** (still inline, used by the inline setters), and the members. Note `fields()` and `accept()` no longer need `<algorithm>`/`<any>` only for their bodies, but leave the `#include`s as-is — the inline accessors and members still use them; removing includes would be extra golden churn for no benefit.

- [ ] **Step 4: Create the source template**

Create `src/x3d_cpp_gen/templates/class_template.cpp.jinja` with the out-of-line definitions (the bodies removed from the header), class-qualified and namespace-wrapped:

```jinja
// {{ class_name }}.cpp
#include "{{ class_name }}.hpp"

{% if namespace %}
namespace {{ namespace }} {
{% endif %}
{% if emit_reflection %}
std::string {{ class_name }}::nodeTypeName() const {
    return "{{ class_name }}";
}

std::string {{ class_name }}::defaultContainerField() const {
    return getDefaultContainerField();
}

const FieldTable& {{ class_name }}::fields() const {
    static const FieldTable table = [] {
        FieldTable t;
        {% for field in reflection_fields %}
        t.push_back(FieldInfo{
            "{{ field.x3d_name }}",
            X3DFieldType::{{ field.runtime_field_type }},
            AccessType::{{ field.runtime_access }},
            "{{ field.container_field_default }}",
            {% if field.is_readable %}
            [](const X3DNode& n) -> std::any {
                return std::any(dynamic_cast<const {{ class_name }}&>(n).{{ field.getter_call }}());
            },
            {% else %}
            nullptr,
            {% endif %}
            {% if field.has_data_setter %}
            [](X3DNode& n, const std::any& v) {
                dynamic_cast<{{ class_name }}&>(n).{{ field.reader_setter_call }}(std::any_cast<{{ field.cpp_type }}>(v));
            },
            {% elif field.is_event %}
            [](X3DNode& n, const std::any& v) {
                dynamic_cast<{{ class_name }}&>(n).{{ field.handler_call }}(std::any_cast<{{ field.cpp_type }}>(v));
            },
            {% elif field.is_readonly %}
            [](X3DNode& n, const std::any& v) {
                dynamic_cast<{{ class_name }}&>(n).{{ field.emitter_call }}(std::any_cast<{{ field.cpp_type }}>(v));
            },
            {% else %}
            nullptr,
            {% endif %}
            {% if field.is_enum %}
            {% if field.is_readable %}
            [](const X3DNode& n) -> std::string {
                {% if field.runtime_field_type == "MFEnum" %}
                const auto& vec = dynamic_cast<const {{ class_name }}&>(n).{{ field.getter_call }}();
                std::string out;
                for (std::size_t i = 0; i < vec.size(); ++i) {
                    if (i) out += ' ';
                    out += to_string(vec[i]);
                }
                return out;
                {% else %}
                return to_string(dynamic_cast<const {{ class_name }}&>(n).{{ field.getter_call }}());
                {% endif %}
            },
            {% else %}
            nullptr,
            {% endif %}
            {% if field.has_data_setter %}
            [](X3DNode& n, const std::string& s) {
                {% if field.runtime_field_type == "MFEnum" %}
                {{ field.cpp_type }} vec;
                std::size_t i = 0;
                while (i < s.size()) {
                    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' ||
                           s[i] == '\n' || s[i] == '\r' || s[i] == ',')) ++i;
                    std::size_t j = i;
                    while (j < s.size() && s[j] != ' ' && s[j] != '\t' &&
                           s[j] != '\n' && s[j] != '\r' && s[j] != ',') ++j;
                    if (j > i) {
                        {{ field.enum_cpp_name }} ev;
                        if (from_string(s.substr(i, j - i), ev)) vec.push_back(ev);
                    }
                    i = j;
                }
                dynamic_cast<{{ class_name }}&>(n).{{ field.reader_setter_call }}(std::move(vec));
                {% else %}
                {{ field.enum_cpp_name }} ev;
                if (from_string(s, ev)) dynamic_cast<{{ class_name }}&>(n).{{ field.reader_setter_call }}(ev);
                {% endif %}
            }
            {% else %}
            nullptr
            {% endif %}
            {% else %}
            nullptr,
            nullptr
            {% endif %}
        });
        {% endfor %}
        return t;
    }();
    return table;
}

void {{ class_name }}::accept(NodeVisitor& visitor) const {
    if (!visitor.enter(*this)) {
        return;
    }
    visitor.leave(*this);
}

{% set _constrained = reflection_fields | selectattr('is_settable') | selectattr('has_constraints') | list %}
{% if is_root %}
void {{ class_name }}::validateRanges(std::vector<RangeDiagnostic>& /*out*/) const {}
{% elif _constrained %}
void {{ class_name }}::validateRanges(std::vector<RangeDiagnostic>& out) const {
    {% for field in _constrained %}
    {{ field.range_checker_name }}({{ field.getter_call }}(), nodeTypeName(), "", out);
    {% endfor %}
}
{% endif %}

{% set _own_constrained = fields | selectattr('is_settable') | selectattr('has_constraints') | list %}
{% for field in _own_constrained %}
void {{ class_name }}::{{ field.range_checker_name }}(const {{ field.cpp_type }}& value,
        const std::string& nodeType, const std::string& defName,
        std::vector<RangeDiagnostic>& out) {
    {{ field.range_collect_body | indent(4) }}
}
{% endfor %}
{% endif %}
{% if namespace %}
} // namespace {{ namespace }}
{% endif %}
```

Note: this template is the exact bodies the header previously inlined, moved verbatim and class-qualified. Match the existing whitespace/markers closely — clang-format normalizes the rest, and the golden diff in Task 4 Step 7 is how you confirm equivalence of the *generated* output.

- [ ] **Step 5: Make the backend render both files**

In `src/x3d_cpp_gen/backends/cpp_header.py`, in `__init__` add the source template alongside the header template (after line 42):

```python
        self._template = env.get_template('class_template.hpp.jinja')
        self._source_template = env.get_template('class_template.cpp.jinja')
```

In `emit`, after the existing header render+write+format (after line 179, inside the per-node loop), render and write the `.cpp` with the SAME context, then clang-format it:

```python
            source_code = self._source_template.render(
                class_name=node.name,
                fields=descriptors,
                reflection_fields=reflection_descriptors,
                is_root=is_root,
                emit_reflection=emit_reflection,
                namespace=self.namespace,
            )
            source_file = os.path.join(out_dir, f"{node.name}.cpp")
            with open(source_file, 'w') as f:
                f.write(source_code)
            print(f"Generated {source_file}")
            clang_format = self._format(source_file, clang_format)
```

- [ ] **Step 6: Run the generator unit tests**

Run: `uv run pytest tests/test_decl_def_split.py -q`
Expected: PASS (both factory and per-node split assertions).

- [ ] **Step 7: Regenerate the full tree**

Run: `uv run x3d-cpp-gen --out generated_cpp_bindings --no-test`
Expected: every `generated_cpp_bindings/<Node>.cpp` is produced and every `<Node>.hpp` shrinks (no inline `fields()` body). Spot-check: `grep -L "static const FieldTable table" generated_cpp_bindings/*.hpp | wc -l` should report all node headers (none retain the inline table), and `ls generated_cpp_bindings/*.cpp | wc -l` should be ~343 (one per node + `X3DNodeFactory.cpp`).

- [ ] **Step 8: Full build + ctest + golden**

Run: `mise run build`
Expected: ctest **459/459 passed** — behavior unchanged.
Run: `mise run golden`
Expected: `Golden tree OK` (the regenerated `.hpp`+`.cpp` match the committed set).

- [ ] **Step 9: Commit**

```bash
git add src/x3d_cpp_gen/templates/class_template.hpp.jinja \
        src/x3d_cpp_gen/templates/class_template.cpp.jinja \
        src/x3d_cpp_gen/backends/cpp_header.py \
        tests/test_decl_def_split.py generated_cpp_bindings/
git commit -m "codegen: split each node into lean header + out-of-line .cpp (C1)"
```

---

## Task 5: Validate, measure, and record

**Files:**
- Modify: `docs/superpowers/BACKLOG.md` (record C1 done + new golden hash)

- [ ] **Step 1: Confirm the full suite is green and golden is clean**

Run: `mise run ci`
Expected: pytest green, golden gate clean, C++ build + ctest **459/459**.

- [ ] **Step 2: Record the new golden hash**

Run:
```bash
cat $(find generated_cpp_bindings -name '*.hpp' -o -name '*.cpp' | sort) | sha256sum
```
Note the hash for the commit message / memory (the golden hash legitimately changed; bodies moved out-of-line and `.cpp` were added).

- [ ] **Step 3: Cold-build A/B to confirm the win**

Run (cold, ccache disabled, both at the same `-j`, in throwaway dirs):
```bash
export CCACHE_DISABLE=1
# Measure AFTER (current tree), cold, no ccache, -j4 to match the baseline:
d=$(mktemp -d); cmake -G Ninja -S . -B "$d" -DX3D_CPP_COMPILE_JOBS=4 >/dev/null 2>&1
t0=$(date +%s); cmake --build "$d" >/dev/null 2>&1; echo "AFTER -j4 cold = $(( $(date +%s)-t0 ))s"; rm -rf "$d"
```
Compare against the recorded BEFORE baseline of **1296s** (`-j4`, cold, no PCH) from the spec's profiling session. Expected: a large drop (the relocated `fields()` thunks now instantiate + codegen once in `x3d_cpp_nodes`, not per consumer TU). Record the actual number.

- [ ] **Step 4: Update the backlog**

In `docs/superpowers/BACKLOG.md`, add a row (or section) recording C1 as CLOSED with the commit SHA, the new golden hash, and the measured cold-build before/after.

- [ ] **Step 5: Commit**

```bash
git add docs/superpowers/BACKLOG.md
git commit -m "backlog: C1 decl/def split landed — new golden hash + cold-build A/B"
```

---

## Notes for the implementer

- **Heavy builds:** `mise run build` recompiles widely-included headers; expect minutes. Use the `dev` preset (Ninja + ccache + `-j4` pool) it already configures. Do not raise `-j` mid-plan — that tuning is a separate follow-up enabled BY this change.
- **Golden is the equivalence proof:** Tasks 2 and 4 change generated output intentionally. The proof that the relocation is faithful is: ctest stays 459/459 AND the regenerated tree is self-consistent (golden gate passes after regenerate+commit). There is no separate "byte-identical to the old golden" expectation here — the golden hash changes by design.
- **If a consumer TU breaks** in Task 3/4 Step "build" because it used a node type without including its header (previously satisfied transitively by `X3DNodeFactory.hpp`), add the explicit `#include "<Node>.hpp"` to that TU. This is correct IWYU hygiene, not a regression.
- **Namespace/multi-version:** the `.cpp` template wraps definitions in `namespace {{ namespace }}` exactly as the header wraps the class, so the 4.1 namespaced fixture splits identically. No separate handling needed.

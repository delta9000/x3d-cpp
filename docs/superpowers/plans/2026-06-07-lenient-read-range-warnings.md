# Lenient-Read Range-Warning Collection — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Surface, as structured diagnostics, which out-of-range values the lenient read path keeps — paying the owed "+ warnings" half of the lenient-read policy.

**Architecture:** Purely additive codegen — keep the existing throwing `validate<Name>()` (strict setter path) untouched; add a sibling non-throwing `checkRanges<Name>()` static rendered from the same bound spec, plus a virtual `validateRanges()` override on constrained nodes. A hand-written `collectRangeWarnings(root)` walks the node graph via reflection and aggregates them. The `X3DParse` front door runs it and stores results on a new `X3DDocument.rangeWarnings` channel.

**Tech Stack:** Python 3.12 codegen (Jinja2 + emit/ modules), C++20 header-only runtime, pytest + ctest, mise tasks, ccache+Ninja `dev` preset.

**Design spec:** `docs/superpowers/specs/2026-06-07-lenient-read-range-warnings-design.md`

**Scope:** RANGE constraints only (SFColor/SFColorRGBA `[0,1]`, numeric `minInclusive`/`maxInclusive`). Enum/required/type/structural validation is M3, NOT here.

**Build/verify commands (this repo):**
- Python tests: `uv run pytest -q`
- Regenerate bindings: `mise run gen` (or `uv run x3d-cpp-gen --out generated_cpp_bindings`)
- Golden gate: `mise run golden`
- C++ build + tests: `mise run build` (Ninja + ccache + parallel ctest)
- Single ctest: `ctest --preset dev -R <name> --output-on-failure`

---

## File Structure

- **Modify** `src/x3d_cpp_gen/emit/reflection.py` — emit the `RangeDiagnostic` struct into `X3DReflection.hpp`.
- **Modify** `src/x3d_cpp_gen/emit/descriptors.py` — add `_render_range_collect()` (sibling of `_render_constraints()`) and a `range_collect_body` descriptor field + `range_checker_name`.
- **Modify** `src/x3d_cpp_gen/templates/class_template.hpp.jinja` — emit per-field `checkRanges<Name>()` static; emit `validateRanges()` (base no-op on X3DNode, override on constrained nodes).
- **Create** `runtime/X3DRangeValidate.hpp` — `collectRangeWarnings(const X3DNode&)` graph walk.
- **Modify** `runtime/X3DDocument.hpp` — add `std::vector<RangeDiagnostic> rangeWarnings;`.
- **Modify** `runtime/parse/X3DParse.hpp` — populate `doc.rangeWarnings` in `parseDocument`.
- **Modify** `runtime/X3DRuntime.hpp` (umbrella) — include `X3DRangeValidate.hpp` if it aggregates runtime headers (verify at execution).
- **Modify** `CMakeLists.txt` — register a new `x3d_range_warnings` ctest executable.
- **Create** `runtime/parse/tests/range_warnings_test.cpp` — C++ TDD for the pass + front-door integration.
- **Modify** `tests/test_emission.py` — Python assertions on the new generated code.

**Golden:** Tasks 1–3 change generated output; Task 4 regenerates and commits the new golden sha. Tasks 5–6 depend on the regenerated headers.

---

## Task 1: `RangeDiagnostic` struct in the reflection header

**Files:**
- Modify: `src/x3d_cpp_gen/emit/reflection.py`
- Test: `tests/test_emission.py`

- [x] **Step 1: Write the failing test**

Add to `tests/test_emission.py`:

```python
def test_reflection_header_defines_range_diagnostic():
    from x3d_cpp_gen.emit.reflection import gen_reflection_header
    src = gen_reflection_header()
    assert "struct RangeDiagnostic" in src
    # structured fields + string rendering
    for field in ("nodeType", "defName", "fieldName", "detail"):
        assert field in src
    assert "std::string message() const" in src
```

- [x] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_emission.py::test_reflection_header_defines_range_diagnostic -q`
Expected: FAIL (`struct RangeDiagnostic` not found).

- [x] **Step 3: Add the struct to `gen_reflection_header()`**

In `src/x3d_cpp_gen/emit/reflection.py`, inside `gen_reflection_header()`, after the `FieldInfo`/`FieldTable` declarations and before `NodeVisitor` (locate by reading the function; append these lines into the same `lines` list at that point):

```python
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief One out-of-range value kept by the lenient read path.")
    lines.append(" * @details Range constraints (SFColor [0,1], numeric min/maxInclusive)")
    lines.append(" *          are enforced by the typed set<Name>() but NOT by the lenient")
    lines.append(" *          reflection write path, so out-of-range authored values are")
    lines.append(" *          kept. validateRanges()/collectRangeWarnings() recover them as")
    lines.append(" *          structured diagnostics. Range constraints ONLY (not enum/type).")
    lines.append(" */")
    lines.append("struct RangeDiagnostic {")
    lines.append("    std::string nodeType;   // e.g. \"Material\"")
    lines.append("    std::string defName;    // DEF name if known, else \"\"")
    lines.append("    std::string fieldName;  // e.g. \"specularColor\"")
    lines.append("    std::string detail;     // e.g. \"specularColor.r above maximum of 1\"")
    lines.append("    std::string message() const {")
    lines.append("        std::string who = defName.empty() ? nodeType")
    lines.append("                                          : (nodeType + \" DEF \" + defName);")
    lines.append("        return who + \".\" + fieldName + \": \" + detail;")
    lines.append("    }")
    lines.append("};")
```

(`<string>` is already included by this header — verified.)

- [x] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/test_emission.py::test_reflection_header_defines_range_diagnostic -q`
Expected: PASS.

- [x] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/emit/reflection.py tests/test_emission.py
git commit -m "Range warnings: emit RangeDiagnostic struct into X3DReflection.hpp"
```

---

## Task 2: Render the non-throwing range-collect body

**Files:**
- Modify: `src/x3d_cpp_gen/emit/descriptors.py`
- Test: `tests/test_descriptors.py`

Context: `_render_constraints(name, x3d_type, lo, hi)` (descriptors.py ~line 200) renders the THROWING ladder using `TypeRegistry.components()` and `TypeRegistry.is_multi()`. We add a sibling that renders the same bounds as `out.push_back(RangeDiagnostic{...})` calls. `class_name` is injected by the template at use site, so the rendered body references a `kNodeType` symbol the template defines (see Task 3) — keep the body node-type-agnostic by emitting the literal field/detail and a `nodeType` placeholder the template supplies.

- [x] **Step 1: Write the failing test**

Add to `tests/test_descriptors.py`:

```python
def test_render_range_collect_scalar_and_color():
    from x3d_cpp_gen.emit.descriptors import _render_range_collect
    from x3d_cpp_gen.model.types import X3DType
    # scalar max-only (e.g. beamWidth <= hi)
    body = _render_range_collect("beamWidth", X3DType.SFFloat, None, "1.5707")
    assert "out.push_back(RangeDiagnostic{" in body
    assert "beamWidth above maximum of 1.5707" in body
    assert "throw" not in body
    # color: per-component [0,1]
    cbody = _render_range_collect("specularColor", X3DType.SFColor, "0", "1")
    assert "specularColor.r above maximum of 1" in cbody
    assert "specularColor.r below minimum of 0" in cbody
    assert "throw" not in cbody
```

- [x] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_descriptors.py::test_render_range_collect_scalar_and_color -q`
Expected: FAIL (`_render_range_collect` not defined).

- [x] **Step 3: Implement `_render_range_collect`**

In `src/x3d_cpp_gen/emit/descriptors.py`, directly below `_render_constraints`, add (mirrors its structure exactly, swapping `throw` for `out.push_back`):

```python
def _render_range_collect(name: str, x3d_type: "X3DType",
                          lo: Optional[str], hi: Optional[str]) -> str:
    """Render the C++ body of checkRanges<Name>(): the same bound ladder as
    _render_constraints, but APPENDING a RangeDiagnostic per violation instead of
    throwing. `nodeType` is a parameter the generated static receives (the
    template passes the class name). Returns inner statements only.
    """
    comps = TypeRegistry.components(x3d_type)

    def push(label: str, detail: str) -> str:
        return ('out.push_back(RangeDiagnostic{nodeType, defName, "%s", "%s"});'
                % (name, detail))

    def group(target: str, label: str) -> str:
        out = []
        if lo is not None:
            out.append('if (%s < %s) %s' % (
                target, lo, push(label, "%s below minimum of %s" % (label, lo))))
        if hi is not None:
            out.append('if (%s > %s) %s' % (
                target, hi, push(label, "%s above maximum of %s" % (label, hi))))
        return "\n".join(out)

    def body(prefix: str) -> str:
        if comps:
            groups = [group("%s.%s" % (prefix, c), "%s.%s" % (name, c)) for c in comps]
        else:
            groups = [group(prefix, name)]
        return "\n\n".join(groups)

    if TypeRegistry.is_multi(x3d_type):
        return ("for (const auto& v : value) {\n\n" + body("v") + "\n}")
    return body("value")
```

- [x] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/test_descriptors.py::test_render_range_collect_scalar_and_color -q`
Expected: PASS.

- [x] **Step 5: Wire the rendered body + checker name into the descriptor**

In `descriptors.py`, locate the `FieldDescriptor` dataclass (the `constraint_checks` / `validator_name` fields, ~line 40–44) and add two attributes:

```python
    range_collect_body: Optional[str]     # rendered C++ for checkRanges<Name>(), or None
```

Add a property near `setter_unchecked_name` (~line 106):

```python
    @property
    def range_checker_name(self) -> str:
        """Non-throwing range collector for a constrained field: checkRanges<Name>()."""
        name_pascal = self.validator_name[len("validate"):]  # reuse the Pascal stem
        return f"checkRanges{name_pascal}"
```

In `build_descriptor` (where `constraint_checks` is computed, ~line 287), add alongside it:

```python
    range_collect_body = (
        _render_range_collect(field.name, x3d_type, lo, hi)
        if wants_validation else None
    )
```

And pass `range_collect_body=range_collect_body` into the `FieldDescriptor(...)` construction (both the normal return ~line 311 and the enum path `_build_enum_descriptor` — enums have no ranges, pass `range_collect_body=None` there).

- [x] **Step 6: Run the full descriptor + emission suites**

Run: `uv run pytest tests/test_descriptors.py tests/test_emission.py -q`
Expected: PASS (no constructor-arg mismatch from the new dataclass field).

- [x] **Step 7: Commit**

```bash
git add src/x3d_cpp_gen/emit/descriptors.py tests/test_descriptors.py
git commit -m "Range warnings: render non-throwing checkRanges body + descriptor wiring"
```

---

## Task 3: Emit `checkRanges<Name>()` + `validateRanges()` in the template

**Files:**
- Modify: `src/x3d_cpp_gen/templates/class_template.hpp.jinja`
- Test: `tests/test_emission.py`

Context: the throwing validator is emitted around template line 356–360 (`static void {{ field.validator_name }}(...)`). The base virtuals (`fields()`, `nodeTypeName()`, `defaultContainerField()`) use the `is_root` pattern (`"virtual " if is_root else ""` + `" override" if not is_root`). `{{ class_name }}` is in scope. Constrained fields have `field.has_constraints` and `field.cpp_ident` (member), `field.x3d_name`, `field.range_collect_body`.

- [x] **Step 1: Write the failing test**

Add to `tests/test_emission.py` (use an existing helper that renders one node header — check the file for the current render helper, e.g. `render_node(...)`; the snippet below assumes a `Material`-like node with a constrained color field is rendered):

```python
def test_node_emits_checkranges_and_validateranges():
    # Render a node known to have a constrained field (specularColor on Material).
    src = render_named_node("Material")  # use the file's existing single-node render helper
    assert "checkRangesSpecularColor" in src
    assert "out.push_back(RangeDiagnostic{" in src
    assert "void validateRanges(std::vector<RangeDiagnostic>& out) const override" in src
    # base no-op lives on the root node
    root = render_named_node("X3DNode")
    assert "virtual void validateRanges(std::vector<RangeDiagnostic>&" in root
```

> If `tests/test_emission.py` has no single-node render helper, add a small one that calls the same generator path the other emission tests use (inspect the top of the file for the existing pattern and reuse it verbatim).

- [x] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_emission.py::test_node_emits_checkranges_and_validateranges -q`
Expected: FAIL (`checkRangesSpecularColor` not found).

- [x] **Step 3: Emit the per-field checker**

In `class_template.hpp.jinja`, immediately AFTER the existing throwing validator block (after the `static void {{ field.validator_name }}(...)` closing `}` near line 360), add:

```jinja
    {% if field.is_settable and field.has_constraints %}
    /**
     * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
     *        component. Used by validateRanges() to surface lenient-read values.
     */
    static void {{ field.range_checker_name }}(const {{ field.cpp_type }}& value,
            const std::string& defName, std::vector<RangeDiagnostic>& out) {
        const std::string nodeType = "{{ class_name }}";
        {{ field.range_collect_body | indent(8) }}
    }
    {% endif %}
```

- [x] **Step 4: Emit `validateRanges()` (override on constrained nodes, no-op on root)**

In the reflection section (near `fields()`, ~line 250), add a sibling method. Place it so it renders for every node. Determine `has_any_constraints` in the template by testing the fields list:

```jinja
    {% set constrained = fields | selectattr('has_constraints') | selectattr('is_settable') | list %}
    {% if is_root %}
    /**
     * @brief Append RangeDiagnostics for any out-of-range constrained field on
     *        this node (own fields only; traversal is collectRangeWarnings()'s job).
     *        Base no-op; constrained nodes override.
     */
    virtual void validateRanges(std::vector<RangeDiagnostic>& /*out*/) const {}
    {% elif constrained %}
    void validateRanges(std::vector<RangeDiagnostic>& out) const override {
        {% for field in constrained %}
        {{ field.range_checker_name }}({{ field.cpp_ident }}, "", out);
        {% endfor %}
    }
    {% endif %}
```

> Verify `fields` is the in-scope variable name for the field list in this template (the existing `{% for field in ... %}` loops reveal it — match that name). If the template iterates `reflection_fields` for `fields()`, use the list that includes own settable constrained fields with their `cpp_ident` members.

- [x] **Step 5: Run test to verify it passes**

Run: `uv run pytest tests/test_emission.py::test_node_emits_checkranges_and_validateranges -q`
Expected: PASS.

- [x] **Step 6: Run the whole Python suite**

Run: `uv run pytest -q`
Expected: PASS (some golden-tree tests may now report drift — that is expected and fixed in Task 4; note which fail).

- [x] **Step 7: Commit**

```bash
git add src/x3d_cpp_gen/templates/class_template.hpp.jinja tests/test_emission.py
git commit -m "Range warnings: emit checkRanges<Name> + validateRanges in node template"
```

---

## Task 4: Regenerate and commit the golden

**Files:**
- Modify: `generated_cpp_bindings/*.hpp` (regenerated)

- [x] **Step 1: Regenerate**

Run: `mise run gen`
Expected: regenerates all headers; `X3DReflection.hpp` now has `RangeDiagnostic`, `X3DNode.hpp` has the base `validateRanges`, constrained nodes have overrides + `checkRanges<Name>`.

- [x] **Step 2: Sanity-check the diff is additive**

Run: `git diff --stat generated_cpp_bindings | tail -5` and
`git diff generated_cpp_bindings/Material.hpp | grep -E '^[-+]' | grep -iE 'setSpecularColor|validateSpecularColor' | head`
Expected: NO `-` (removed) lines for existing `set*`/`validate*` — changes are additions only. If existing setter/validator lines changed, STOP and review (the design requires them byte-identical).

- [x] **Step 3: Confirm the golden gate now passes against the new tree**

Run: `mise run golden`
Expected: PASS (temp regen byte-identical to the committed tree you just regenerated).

- [x] **Step 4: Verify headers still compile**

Run: `mise run build`
Expected: build green; ctest 400/400 still pass (no new tests yet; existing behavior unchanged).

- [x] **Step 5: Commit the golden**

```bash
git add generated_cpp_bindings
git commit -m "Range warnings: regenerate golden (additive validateRanges/checkRanges)"
```

---

## Task 5: `collectRangeWarnings()` graph walk

**Files:**
- Create: `runtime/X3DRangeValidate.hpp`
- Modify: `runtime/X3DRuntime.hpp` (umbrella include — verify it lists runtime headers)
- Create: `runtime/parse/tests/range_warnings_test.cpp`
- Modify: `CMakeLists.txt`

Context: enumerate children exactly like `XmlWriter::writeFieldChildren` (XmlWriter.hpp:232–250): for each `FieldInfo f` in `node.fields()` where `f.isNodeField()`, `f.get(node)` is `std::any` holding `shared_ptr<X3DNode>` (SFNode) or `vector<shared_ptr<X3DNode>>` (MFNode).

- [x] **Step 1: Write the failing test**

Create `runtime/parse/tests/range_warnings_test.cpp`:

```cpp
// Range-warning collection: validateRanges() per node + collectRangeWarnings() walk.
#include "X3DRangeValidate.hpp"
#include "Material.hpp"
#include "Shape.hpp"
#include "Appearance.hpp"
#include <cassert>
#include <iostream>
#include <memory>

static int failures = 0;
#define CHECK(c) do { if(!(c)) { std::cerr << "FAIL: " #c " @" << __LINE__ << "\n"; ++failures; } } while(0)

int main() {
    // (a) a Material with an out-of-range specularColor component
    auto mat = std::make_shared<Material>();
    SFColor bad; bad.r = 1.5f; bad.g = 0.0f; bad.b = 0.0f;
    mat->setSpecularColorUnchecked(bad);           // lenient write keeps it
    std::vector<RangeDiagnostic> own;
    mat->validateRanges(own);
    CHECK(own.size() == 1);
    CHECK(own[0].nodeType == "Material");
    CHECK(own[0].fieldName == "specularColor");
    CHECK(own[0].detail.find("above maximum of 1") != std::string::npos);

    // (b) nested: Shape -> appearance -> Material; collectRangeWarnings finds it
    auto shape = std::make_shared<Shape>();
    auto app = std::make_shared<Appearance>();
    app->setMaterialUnchecked
        ? app->setMaterialUnchecked(mat) : app->setMaterial(mat); // use whichever exists
    shape->setAppearance(app);
    auto all = collectRangeWarnings(*shape);
    CHECK(all.size() == 1);
    CHECK(all[0].fieldName == "specularColor");

    // (c) in-range node yields nothing
    auto clean = std::make_shared<Material>();
    std::vector<RangeDiagnostic> none;
    clean->validateRanges(none);
    CHECK(none.empty());

    if (failures) { std::cerr << failures << " checks failed\n"; return 1; }
    std::cout << "range_warnings_test OK\n";
    return 0;
}
```

> At execution, confirm the exact setter names for `Material.specularColor` and `Appearance.material` from the generated headers and adjust the calls (e.g. `setSpecularColorUnchecked`, `setMaterial`). Use a constrained field that actually exists on `Material` (specularColor is `[0,1]`).

- [x] **Step 2: Create the implementation**

Create `runtime/X3DRangeValidate.hpp`:

```cpp
// X3DRangeValidate.hpp — collect out-of-range values kept by the lenient read
// path. Node-agnostic: walks the graph via reflection and aggregates each node's
// validateRanges(). Range constraints ONLY (SFColor [0,1], numeric min/max).
#ifndef X3D_RANGE_VALIDATE_HPP
#define X3D_RANGE_VALIDATE_HPP

#include "X3DNode.hpp"
#include <memory>
#include <vector>

/// Append every node's range diagnostics, depth-first over SFNode/MFNode fields.
inline void collectRangeWarnings(const X3DNode& node,
                                 std::vector<RangeDiagnostic>& out) {
    node.validateRanges(out);
    for (const FieldInfo& f : node.fields()) {
        if (!f.isNodeField() || !f.get) continue;
        std::any v = f.get(node);
        if (f.type == X3DFieldType::SFNode) {
            auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
            if (child) collectRangeWarnings(*child, out);
        } else { // MFNode
            auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
            for (const auto& child : vec)
                if (child) collectRangeWarnings(*child, out);
        }
    }
}

/// Convenience overload returning a fresh vector.
inline std::vector<RangeDiagnostic> collectRangeWarnings(const X3DNode& root) {
    std::vector<RangeDiagnostic> out;
    collectRangeWarnings(root, out);
    return out;
}

#endif // X3D_RANGE_VALIDATE_HPP
```

> Verify `FieldInfo`'s getter member name (`.get`) and `isNodeField()` against `generated_cpp_bindings/X3DReflection.hpp` (lines ~91–107) and match exactly. Confirm the `std::any` payload types against `XmlWriter::writeFieldChildren`.

- [x] **Step 3: Register the ctest**

In `CMakeLists.txt`, after the `x3d_lenient_read` block (~line 382–385), add:

```cmake
    # Range-warning collection: validateRanges()/collectRangeWarnings() surface
    # out-of-range values the lenient read path keeps (structured diagnostics).
    add_executable(x3d_range_warnings
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/range_warnings_test.cpp")
    target_link_libraries(x3d_range_warnings PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_range_warnings COMMAND x3d_range_warnings)
```

- [x] **Step 4: Configure + run the new test (expect compile/usage fixups first)**

Run: `cmake --preset dev && ctest --preset dev -R x3d_range_warnings --output-on-failure`
Expected: FAIL first run if setter names differ; fix the test's setter calls per the generated headers, then PASS.

- [x] **Step 5: Commit**

```bash
git add runtime/X3DRangeValidate.hpp runtime/parse/tests/range_warnings_test.cpp CMakeLists.txt runtime/X3DRuntime.hpp
git commit -m "Range warnings: collectRangeWarnings graph walk + test"
```

---

## Task 6: Front-door integration via `X3DDocument.rangeWarnings`

**Files:**
- Modify: `runtime/X3DDocument.hpp`
- Modify: `runtime/parse/X3DParse.hpp`
- Modify: `runtime/parse/tests/range_warnings_test.cpp`

Context: `parseDocument(text, enc?)` and `parseFile(path)` return `runtime::X3DDocument` by value (X3DParse.hpp:75, 99). `X3DDocument` holds the parsed `Scene`/root. Verify the root-node accessor on `X3DDocument` (e.g. `scene`, `root`, `rootNodes`) by reading `runtime/X3DDocument.hpp`.

- [x] **Step 1: Write the failing test (extend the C++ test)**

Append to `runtime/parse/tests/range_warnings_test.cpp` `main()` before the final return, an inline-XML parse that carries an out-of-range value:

```cpp
    // (d) front-door: a document with an out-of-range value populates rangeWarnings.
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'><Scene>"
        "<Shape><Appearance><Material specularColor='1.5 0 0'/></Appearance></Shape>"
        "</Scene></X3D>";
    auto doc = x3d::codec::parseDocument(xml);
    CHECK(!doc.rangeWarnings.empty());
    CHECK(doc.rangeWarnings[0].fieldName == "specularColor");
    // (e) in-range document -> empty
    const std::string ok =
        "<X3D profile='Interchange' version='4.0'><Scene>"
        "<Shape><Appearance><Material specularColor='0.5 0 0'/></Appearance></Shape>"
        "</Scene></X3D>";
    CHECK(x3d::codec::parseDocument(ok).rangeWarnings.empty());
```

Add `#include "X3DParse.hpp"` to the test includes.

- [x] **Step 2: Run to verify it fails**

Run: `ctest --preset dev -R x3d_range_warnings --output-on-failure`
Expected: FAIL (`rangeWarnings` is not a member of `X3DDocument`).

- [x] **Step 3: Add the channel to `X3DDocument`**

In `runtime/X3DDocument.hpp`, add `#include "X3DRangeValidate.hpp"` (for `RangeDiagnostic`; or include `X3DReflection.hpp` if lighter) and a member to the `X3DDocument` struct/class:

```cpp
    /// Out-of-range values kept by the lenient read path (structured; populated
    /// by the X3DParse front door). Empty for a clean or programmatically-built
    /// document until collectRangeWarnings() is run.
    std::vector<RangeDiagnostic> rangeWarnings;
```

- [x] **Step 4: Populate it at the front door**

In `runtime/parse/X3DParse.hpp`, add `#include "X3DRangeValidate.hpp"`. In `parseDocument` (and ensure `parseFile` flows through it — it calls `parseDocument`, so one site suffices), just before `return doc;`, walk each root node:

```cpp
    // Surface out-of-range values the lenient readers kept (structured channel).
    for (const auto& root : doc.<ROOT_NODES_ACCESSOR>)
        if (root) collectRangeWarnings(*root, doc.rangeWarnings);
```

> Replace `<ROOT_NODES_ACCESSOR>` with the actual member (read `X3DDocument.hpp`: likely `scene.rootNodes` / `rootNodes` / `scene.children`). If the document exposes a single scene root, walk that instead.

- [x] **Step 5: Run to verify it passes**

Run: `ctest --preset dev -R x3d_range_warnings --output-on-failure`
Expected: PASS (all of a–e).

- [x] **Step 6: Commit**

```bash
git add runtime/X3DDocument.hpp runtime/parse/X3DParse.hpp runtime/parse/tests/range_warnings_test.cpp
git commit -m "Range warnings: X3DDocument.rangeWarnings populated at X3DParse front door"
```

---

## Task 7: Full verification + memory update

**Files:** none (verification) + memory note.

- [x] **Step 1: Full Python suite**

Run: `uv run pytest -q`
Expected: PASS (all, including golden-tree).

- [x] **Step 2: Golden gate**

Run: `mise run golden`
Expected: PASS.

- [x] **Step 3: Full C++ build + all tests**

Run: `mise run build`
Expected: build green; ctest 401/401 (the prior 400 + `x3d_range_warnings`), 100% passed.

- [x] **Step 4: Corpus smoke (optional, if the sweep harness is built)**

Run a parse over a handful of known out-of-range corpus files (e.g. official examples with `specularColor>1`) and eyeball that `doc.rangeWarnings` is non-empty for them — confirms real-world coverage, not just fixtures.

- [x] **Step 5: Update memory**

Edit `<repo-root>/memory/x3d-cpp-gen-decisions.md`: mark the lenient-read "STILL TODO: warning collection" as DONE (commit refs), noting it is a structured `RangeDiagnostic` channel on `X3DDocument.rangeWarnings` + `collectRangeWarnings()`, range-only, additive golden (record the new sha).

- [x] **Step 6: Final commit + push**

```bash
git add -A && git commit -m "Range warnings: docs/memory + final verification (Step 0 complete)"
git push origin modernize-x3d-spec
```

---

## Self-Review notes (for the implementer)

- **Spec coverage:** §1 struct → T1; §2 hook (DRY checker) → T2/T3 (refined to additive sibling checker, existing validator untouched); §3 traversal → T5; §4 front door → T6 (refined to `X3DDocument.rangeWarnings` structured channel — the front door has no reader-warnings surface, so no `warnings_` stringification is needed and strict-mode fixtures are untouched by construction); §5 tests → T1–T7.
- **Deviation from spec (intentional, safer):** the strict `setX` path is NOT rerouted; the new `checkRanges<Name>` is a sibling of the unchanged throwing `validate<Name>`, both rendered from the same `lo/hi/comps`. Keeps existing golden lines byte-identical (additive only). Equally DRY at the bound-definition level.
- **Verify-at-execution markers** (kept because they depend on exact generated symbol names): the single-node render helper in `test_emission.py`; the field-list variable name in the template; `FieldInfo.get`/`isNodeField` spelling; `Material.specularColor`/`Appearance.material` setter names; `X3DDocument` root-nodes accessor. Each step says what to confirm and where.

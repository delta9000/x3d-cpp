# initializeOnly Write-Path Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `initializeOnly` fields writable at the data/initialization layer (so readers and PROTO expansion populate them) by emitting a `set<Name>Unchecked()` + reflection `set` thunk for them, without adding any public typed `set<Name>()`.

**Architecture:** A two-line change in the descriptor IR (`has_data_setter` predicate + `reader_setter_call` routing) plus three guard changes in the node template, then a full golden regeneration. The reflection `set` thunk becomes non-null for `initializeOnly` fields, so the existing reflection-driven readers populate them with no reader-logic change.

**Tech Stack:** Python generator (`src/x3d_cpp_gen/`), Jinja2 node template, `uv`/pytest, C++20 header-only runtime, CMake/CTest, `mise` tasks.

**Spec:** `docs/superpowers/specs/2026-06-12-initializeonly-write-path-design.md`

**Global guardrails (every task):**
- Branch: stay on `modernize-x3d-spec`, commit directly. Do NOT branch.
- Regenerate bindings with `mise run gen` (= `uv run x3d-cpp-gen --out generated_cpp_bindings`).
- Build/run C++ with `mise run build` then `ctest --test-dir build --output-on-failure`. NEVER pass unbounded `-j` (the all-headers TU OOMs).
- Full Python gate: `uv run pytest -q` (includes the golden-tree drift test, which regenerates and diffs byte-for-byte — so after a template change you MUST `mise run gen` and commit the new headers or it fails).
- This change is INTENDED to alter the golden. That is expected; the gate passes once the regenerated headers are committed.
- TDD: failing test first, then implement, then green.

---

## File Structure

**Modified**
- `src/x3d_cpp_gen/emit/descriptors.py` — add `has_data_setter`; route `reader_setter_call` to the unchecked setter for `initializeOnly`.
- `src/x3d_cpp_gen/templates/class_template.hpp.jinja` — emit `set<Name>Unchecked()` for `initializeOnly`; switch the two reflection `set`-thunk guards from `is_settable` to `has_data_setter`; route the enum-string set thunk through `reader_setter_call`.
- `generated_cpp_bindings/*.hpp` — regenerated (mechanical; all nodes gain `Unchecked` setters + reflection thunks for their `initializeOnly` fields).
- `runtime/codecs/XmlReader.hpp`, `runtime/parse/NodeBuilder.hpp` — update now-stale "initializeOnly: skip" comments.

**New tests**
- `tests/test_descriptors.py` — extend with `has_data_setter`/`reader_setter_call` assertions (Task 1).
- `runtime/codecs/tests/initializeonly_read_test.cpp` — behavioral proof: documents populate `initializeOnly` scalar/node fields; round-trip preserves them; ROUTE to an `initializeOnly` field is rejected (Tasks 2–3).

---

## Task 1: Descriptor IR — `has_data_setter` + `reader_setter_call` routing

This task is **golden-neutral**: `reader_setter_call` is only consumed by the template inside an `is_settable` (inputOutput) guard today, so changing its `initializeOnly` branch produces no output change until Task 2 widens the guards. Unit-tested in pytest via the existing `desc()` helper.

**Files:**
- Modify: `src/x3d_cpp_gen/emit/descriptors.py`
- Test: `tests/test_descriptors.py`

- [ ] **Step 1: Write the failing test.** Append to `tests/test_descriptors.py`:

```python
def test_initialize_only_has_data_setter_routed_to_unchecked():
    d = desc(name="radius", type="SFFloat", accessType="initializeOnly", default="1")
    # No public typed setter (unchanged contract):
    assert d.is_settable is False
    # But writable at the data layer, via the unchecked setter:
    assert d.has_data_setter is True
    assert d.setter_unchecked_name == "setRadiusUnchecked"
    assert d.reader_setter_call == "setRadiusUnchecked"


def test_input_output_reader_setter_call_unchanged():
    # Unconstrained inputOutput still routes to the plain setter.
    d = desc(name="point", type="MFVec3f", accessType="inputOutput")
    assert d.has_data_setter is True
    assert d.reader_setter_call == "setPoint"
    # Constrained inputOutput still routes to the unchecked setter.
    c = desc(name="intensity", type="SFFloat", accessType="inputOutput",
             min_inclusive="0", max_inclusive="1")
    assert c.reader_setter_call == "setIntensityUnchecked"


def test_event_fields_have_no_data_setter():
    assert desc(name="set_fraction", type="SFFloat", accessType="inputOnly").has_data_setter is False
    assert desc(name="isActive", type="SFBool", accessType="outputOnly").has_data_setter is False
```

- [ ] **Step 2: Run red.** `uv run pytest tests/test_descriptors.py -q` → FAIL (`has_data_setter` attribute missing).

- [ ] **Step 3: Implement.** In `src/x3d_cpp_gen/emit/descriptors.py`:

Replace the `reader_setter_call` body (currently around line 140):

```python
        name = self.setter_unchecked_name if self.has_constraints else self.setter_name
```

with:

```python
        # initializeOnly fields have no public set<Name>(); the data-layer write
        # goes through set<Name>Unchecked. Constrained inputOutput fields also use
        # the unchecked path (the typed set<Name> stays the enforcement point).
        if self.access_type == "initializeOnly" or self.has_constraints:
            name = self.setter_unchecked_name
        else:
            name = self.setter_name
```

Add a new property next to `is_settable` (after it, ~line 200):

```python
    @property
    def has_data_setter(self) -> bool:
        """Writable at the data/initialization layer (reflection set thunk emitted).

        inputOutput (runtime-settable) AND initializeOnly (author-settable at parse
        time, but not a runtime/event setter). inputOnly/outputOnly are excluded.
        """
        return self.access_type in ("inputOutput", "initializeOnly")
```

- [ ] **Step 4: Run green.** `uv run pytest tests/test_descriptors.py -q` → PASS. Then `mise run golden` (or `uv run pytest tests/test_golden_tree.py -q`) → still PASS (no output change yet — confirms golden-neutrality).

- [ ] **Step 5: Commit.**

```bash
git add src/x3d_cpp_gen/emit/descriptors.py tests/test_descriptors.py
git commit -m "initializeOnly write-path: descriptor has_data_setter + reader_setter_call routing"
```

---

## Task 2: Template emits the write path + regenerate golden

Driven by a C++ behavioral test: a parsed `initializeOnly` field must hold the document's value.

**Files:**
- Modify: `src/x3d_cpp_gen/templates/class_template.hpp.jinja`
- Regenerate: `generated_cpp_bindings/*.hpp`
- Test: `runtime/codecs/tests/initializeonly_read_test.cpp` (new) + root `CMakeLists.txt`

- [ ] **Step 1: Write the failing C++ test** `runtime/codecs/tests/initializeonly_read_test.cpp`:

```cpp
// initializeonly_read_test.cpp — initializeOnly fields must load from a document.
#include "X3DParse.hpp"
#include "Box.hpp"
#include "Sphere.hpp"
#include "IndexedFaceSet.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
using namespace x3d;

// Find the first descendant node of the given type under the scene roots.
template <class T>
static std::shared_ptr<T> findFirst(const runtime::Scene &scene, const char *type) {
  std::shared_ptr<T> found;
  std::function<void(const std::shared_ptr<X3DNode> &)> walk =
      [&](const std::shared_ptr<X3DNode> &n) {
        if (!n || found) return;
        if (n->nodeTypeName() == type) { found = std::dynamic_pointer_cast<T>(n); return; }
        for (const auto &f : n->fields()) {
          if (!f.get) continue;
          if (f.type == X3DFieldType::SFNode) {
            walk(std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n)));
          } else if (f.type == X3DFieldType::MFNode) {
            for (auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
              walk(c);
          }
        }
      };
  for (const auto &r : scene.rootNodes) walk(r);
  return found;
}

int main() {
  // SFFloat initializeOnly
  {
    auto doc = codec::parseDocument(
        "<X3D version='4.0'><Scene><Shape><Sphere radius='9.5'/></Shape></Scene></X3D>");
    auto s = findFirst<Sphere>(doc.getScene(), "Sphere");
    assert(s && s->getRadius() == 9.5f);
  }
  // SFVec3f initializeOnly
  {
    auto doc = codec::parseDocument(
        "<X3D version='4.0'><Scene><Shape><Box size='3 4 5'/></Shape></Scene></X3D>");
    auto b = findFirst<Box>(doc.getScene(), "Box");
    assert(b && b->getSize().x == 3.f && b->getSize().y == 4.f && b->getSize().z == 5.f);
  }
  // MFInt32 initializeOnly (geometry topology)
  {
    auto doc = codec::parseDocument(
        "<X3D version='4.0'><Scene><Shape><IndexedFaceSet coordIndex='0 1 2 -1'/>"
        "</Shape></Scene></X3D>");
    auto ifs = findFirst<IndexedFaceSet>(doc.getScene(), "IndexedFaceSet");
    assert(ifs && ifs->getCoordIndex().size() == 4 && ifs->getCoordIndex()[2] == 2);
  }
  std::puts("initializeonly_read_test OK");
  return 0;
}
```

Register in the root `CMakeLists.txt` next to the `x3d_codec_roundtrip` block (~line 258):

```cmake
    add_executable(x3d_initializeonly_read
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/initializeonly_read_test.cpp")
    target_link_libraries(x3d_initializeonly_read PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_initializeonly_read COMMAND x3d_initializeonly_read)
```

(Confirm the exact accessor/getter names against the generated headers: `Sphere::getRadius()`, `Box::getSize()`, `IndexedFaceSet::getCoordIndex()` — they follow the `get<Pascal>` convention.)

- [ ] **Step 2: Run red.** `mise run build && ctest --test-dir build -R x3d_initializeonly_read --output-on-failure` → FAIL (radius reads 1, size reads 2 2 2, coordIndex empty — current bindings drop initializeOnly values).

- [ ] **Step 3: Edit the template** `src/x3d_cpp_gen/templates/class_template.hpp.jinja`.

**(a)** Emit `set<Name>Unchecked()` for `initializeOnly` fields. The setter block currently ends:

```jinja
    {% if field.has_constraints %}
    ...
    void {{ field.setter_unchecked_name }}(const {{ field.cpp_type }}& value) {
        {{ field.cpp_ident }} = value;
    }
    {% endif %}
    {% endif %}
    {% endfor %}
```

Change the **outer** `{% endif %}` (the one closing `{% if field.is_settable %}`, ~line 221) into an `{% elif %}` branch:

```jinja
    {% if field.has_constraints %}
    ...
    void {{ field.setter_unchecked_name }}(const {{ field.cpp_type }}& value) {
        {{ field.cpp_ident }} = value;
    }
    {% endif %}
    {% elif field.access_type == "initializeOnly" %}
    {#- initializeOnly: data-layer write path only (no public typed setter). The
        reader / PROTO expansion ingest path writes the field at initialization;
        it is intentionally NOT a runtime/event setter (not a ROUTE sink). -#}
    /**
     * @brief Data-layer write of {{ field.x3d_name }} (reader/init ingest path).
     * @details {{ field.x3d_name }} is initializeOnly: author-settable at parse
     *          time but not via runtime events. No public set{{ field.name_pascal }}().
     */
    void {{ field.setter_unchecked_name }}(const {{ field.cpp_type }}& value) {
        {{ field.cpp_ident }} = value;
    }
    {% endif %}
    {% endfor %}
```

**(b)** Reflection value `set` thunk — change its guard from `is_settable` to `has_data_setter` (the `{% if field.is_settable %}` at ~line 266):

```jinja
                {% if field.has_data_setter %}
                [](X3DNode& n, const std::any& v) {
                    dynamic_cast<{{ class_name }}&>(n).{{ field.reader_setter_call }}(std::any_cast<{{ field.cpp_type }}>(v));
                },
                {% elif field.is_event %}
```

**(c)** Reflection enum-string `set` thunk — change its guard from `is_settable` to `has_data_setter` (the `{% if field.is_settable %}` at ~line 305) **and** change the two body calls from `field.setter_call` to `field.reader_setter_call` (so `initializeOnly` enums route to the unchecked setter; unconstrained inputOutput enums are unaffected since `reader_setter_call == setter_call` there):

```jinja
                {% if field.has_data_setter %}
                [](X3DNode& n, const std::string& s) {
                    {% if field.runtime_field_type == "MFEnum" %}
                    ...
                    dynamic_cast<{{ class_name }}&>(n).{{ field.reader_setter_call }}(std::move(vec));
                    {% else %}
                    {{ field.enum_cpp_name }} ev;
                    if (from_string(s, ev)) dynamic_cast<{{ class_name }}&>(n).{{ field.reader_setter_call }}(ev);
                    {% endif %}
                }
                {% else %}
                nullptr
                {% endif %}
```

Leave the public `set<Name>()` block (`{% if field.is_settable %}` at ~line 186), the `validateRanges` `_constrained`/`_own_constrained` selectors (still `is_settable`), and the validator-static block unchanged.

- [ ] **Step 4: Regenerate the golden.** `mise run gen`. This rewrites every node header. Sanity-check one: `grep -n "setRadiusUnchecked\|set>\?Radius" generated_cpp_bindings/Sphere.hpp` should show `setRadiusUnchecked` present and **no** public `void setRadius(`.

- [ ] **Step 5: Run green.** `mise run build && ctest --test-dir build -R x3d_initializeonly_read --output-on-failure` → PASS. Then full `ctest --test-dir build --output-on-failure` and `uv run pytest -q` (the golden-tree test passes because the committed headers now match the regen). If any pre-existing test counts setters/writable fields and shifts, update it to the corrected contract (the design's §7 note) — investigate, don't blindly bump.

- [ ] **Step 6: Commit** (template + all regenerated headers + the new test + CMake):

```bash
git add src/x3d_cpp_gen/templates/class_template.hpp.jinja generated_cpp_bindings \
        runtime/codecs/tests/initializeonly_read_test.cpp CMakeLists.txt
git commit -m "initializeOnly write-path: template emits setXUnchecked + reflection thunk; regenerate golden"
```

---

## Task 3: Lock-in tests + stale-comment fixes

Locks the two invariants the spec calls out (round-trip preservation; `initializeOnly` stays a non-routable sink) and removes now-false comments.

**Files:**
- Modify: `runtime/codecs/tests/initializeonly_read_test.cpp` (extend) — OR a sibling test; keep it in one TU.
- Modify: `runtime/codecs/XmlReader.hpp:199`, `runtime/parse/NodeBuilder.hpp` (comments only)

- [ ] **Step 1: Write the failing lock-in tests.** Extend `runtime/codecs/tests/initializeonly_read_test.cpp` `main()` with:

```cpp
  // Round-trip preserves an initializeOnly value (read -> write -> read).
  {
    auto doc = codec::parseDocument(
        "<X3D version='4.0'><Scene><Shape><Sphere radius='7.25'/></Shape></Scene></X3D>");
    std::string xml = codec::writeDocumentToXml(doc);     // match the actual writer API
    auto doc2 = codec::parseDocument(xml);
    auto s = findFirst<Sphere>(doc2.getScene(), "Sphere");
    assert(s && s->getRadius() == 7.25f);
  }

  // initializeOnly is writable at the data layer but is NOT a routable ROUTE sink.
  {
    auto sphere = createX3DNode("Sphere");
    const FieldInfo *radius = nullptr;
    for (const auto &f : sphere->fields()) if (f.x3dName == "radius") radius = &f;
    assert(radius && radius->isWritable());               // data-layer writable now
    assert(radius->access == AccessType::InitializeOnly);  // but access stays initializeOnly
  }
```

Add a focused bridge rejection test in the event suite (or extend `runtime/events/tests/scene_bridge_test.cpp`) asserting a ROUTE whose sink field is `initializeOnly` is rejected — model it on that file's existing rejection cases (unknown-field / wrong-direction). Representative shape:

```cpp
  // ROUTE to an initializeOnly field must be rejected (wrong direction / not a sink).
  // Build a Sphere DEF'd, a TimeSensor DEF'd, ROUTE TS.cycleTime -> Sphere.radius.
  // assert the bridge result lists it under `rejected` and routesAdded does not count it.
```

(Use the actual writer entry point — check `runtime/codecs/XmlWriter.hpp` / `X3DParse.hpp` for the round-trip helper name, e.g. `writeDocumentToXml` or an `XmlWriter` visitor; the existing `roundtrip_test.cpp` shows the canonical call.)

- [ ] **Step 2: Run red.** Build + run the test; the round-trip and bridge-rejection assertions drive any gaps. (The data-layer-writable assertion already passes post-Task-2; the round-trip passes only if the writer emits and re-reads the value — confirm.)

- [ ] **Step 3: Make green + fix stale comments.**
  - If round-trip fails because the writer does not emit `initializeOnly` values, that is a writer gap — but the writer reads via the getter (always present), so it should already emit; investigate only if red.
  - Update `runtime/codecs/XmlReader.hpp:199` comment from `// read-only (outputOnly/initializeOnly): skip` to `// read-only (outputOnly): skip` (initializeOnly is now writable).
  - Update `runtime/parse/NodeBuilder.hpp` comments at lines ~13 and ~90-91 (`read-only fields are skipped` / `read-only: skip`) to clarify only outputOnly/inputOnly are skipped on the value-write path.

- [ ] **Step 4: Run green.** `mise run build && ctest --test-dir build --output-on-failure` → all PASS. `uv run pytest -q` → PASS.

- [ ] **Step 5: Commit.**

```bash
git add runtime/codecs/tests/initializeonly_read_test.cpp runtime/events/tests/ \
        runtime/codecs/XmlReader.hpp runtime/parse/NodeBuilder.hpp CMakeLists.txt
git commit -m "initializeOnly write-path: round-trip + non-routable-sink lock-in tests; fix stale comments"
```

---

## Out of scope (documented)

- **Range warnings for constrained `initializeOnly` fields.** The `validateRanges`/`collectRangeWarnings` selectors stay gated on `is_settable` (inputOutput), so a now-writable out-of-range `initializeOnly` value (e.g. a negative `Sphere.radius`) is kept but not surfaced as a range warning. This is a small follow-up to the Step-0 range-warning feature, not part of this fix.

---

## Self-Review

**Spec coverage:** §3.1 descriptor (`has_data_setter`, `reader_setter_call`, keep `is_settable`) → Task 1. §3.2 template (public setX unchanged; widen unchecked emission; reflection-thunk guard → `has_data_setter`; enum thunk via `reader_setter_call`) → Task 2 (a)(b)(c). §3.3 readers no-logic-change + comment fix → Task 3 Step 3. §3.4 bridge unchanged + guard test → Task 3 Step 1 bridge case. §4 golden regen → Task 2 Steps 4-6. §5 tests: pytest shape → Task 1; C++ Sphere/Box/coordIndex → Task 2; round-trip + route-reject → Task 3. §6 DoD → covered across tasks; the "Task 4 PROTO test reverts to Box.size" item is verified when PROTO resumes (out of this plan). §7 naming + flipping-assertion hunt → Task 2 Step 5 / Task 3 Step 3.

**Placeholder scan:** The writer round-trip helper name and the bridge-rejection case in Task 3 are intentionally given as representative shapes with an explicit instruction to match the actual API in the named existing files (`roundtrip_test.cpp`, `scene_bridge_test.cpp`) — those are hand-written test files whose exact helper names must be read in-file, and the anchors are named. No `TBD`/`implement later`.

**Type/name consistency:** `has_data_setter` (new property) used identically in Task 1 (definition + tests) and Task 2 (template guards (b)(c)). `reader_setter_call` routing defined in Task 1 and consumed in Task 2 (b)(c). `setter_unchecked_name` (= `set<Name>Unchecked`) consistent across the template emission and the descriptor tests. The public `set<Name>()` / `is_settable` contract is explicitly left unchanged in all three template sites.

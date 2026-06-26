# Runtime/Gen Split Pivot — Registry-as-Protocol Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Land the codegen-emitted `InterfaceId`/`InterfaceRegistry` protocol, then make the hand-written behavior layer query it instead of `dynamic_cast`/string-name dispatch, and collapse the 8 per-type interpolator systems into 2 templated systems — de-risking the future-proof-architecture pivot with each step independently shippable.

**Architecture:** The scene graph stays the truth; codegen emits a registry (node-type-name → transitive-closure set of `InterfaceId`) as the *protocol*; hand-written behaviors query the registry. The registry is generated **centrally** (one `X3DInterfaceRegistry.hpp/.cpp` pair, mirroring `X3DNodeFactory`), NOT as per-node static arrays — this keeps all ~200 existing golden headers byte-identical. ComponentStore SoA projections and the scene-index filter chain come later (Phases 4–5), gated on profiling evidence and on the registry landing first.

**Tech Stack:** Python 3 codegen (lxml, Jinja2) under `src/x3d_cpp_gen/`; C++20 header-mostly runtime under `runtime/`; generated bindings under `generated_cpp_bindings/`; pytest for codegen, ctest for C++, `mise` task runner.

## Global Constraints

- **C++ standard:** C++20. Runtime code lives in `namespace x3d::runtime`.
- **Golden gate is law:** every `*.hpp` under `generated_cpp_bindings/` must be byte-identical to a fresh regeneration. The gate (`mise run golden` / `scripts/check_golden.sh`) diffs *.hpp in BOTH directions, so a *new* generated header (e.g. `X3DInterfaceRegistry.hpp`) MUST be regenerated and committed or the gate fails. Generated `*.cpp` are excluded from the golden diff but are compiled (CMake globs `generated_cpp_bindings/*.cpp`).
- **Codegen changes are opt-in:** after any emitter/template change run `mise run gen` (`uv run x3d-cpp-gen --out generated_cpp_bindings`) and COMMIT the regenerated tree.
- **Zero new third-party deps in core.** Standard library only for the registry and the interpolator templates.
- **No churn to existing generated headers** unless a task explicitly says so. Phases 1–3 add new files and edit `runtime/`; they must NOT change the byte content of any existing `generated_cpp_bindings/*.hpp`.
- **Regression bars (must stay green at every commit):** `mise run build` (ctest, currently 110/110), `mise run golden`, `mise run corpus` (bounded `x3d_corpus_smoke` ctest; full sweep on request).
- **Build/test commands:** `mise run build` (configure+build+ctest), `mise run test` (pytest), `mise run golden` (drift gate), `mise run gen` (regenerate bindings), `mise run corpus` (corpus smoke).

---

## File Structure

**New (codegen):**
- `src/x3d_cpp_gen/emit/registry.py` — emits `X3DInterfaceRegistry.hpp` + `.cpp` (the `InterfaceId` enum + membership tables). Mirrors `emit/factory.py`.
- `tests/test_interface_registry.py` — pytest over the emitter (enum membership, transitive closure, multi-interface nodes).

**New (generated, committed):**
- `generated_cpp_bindings/X3DInterfaceRegistry.hpp` / `.cpp` — produced by `mise run gen`.

**New (runtime):**
- `runtime/events/InterpolatorSystem.hpp` — the 2 templated interpolator systems.
- `runtime/events/InterpolatorRegistration.hpp` — `registerInterpolatorSystems(ctx)` helper (8 lines).
- `runtime/scene/tests/interface_registry_test.cpp` — C++ ctest for the runtime registry API.
- `runtime/ecs/ComponentStore.hpp` (Phase 4) + test.

**Modified (codegen):**
- `src/x3d_cpp_gen/generator.py` — add `write_interface_registry(...)`.
- `src/x3d_cpp_gen/cli.py` — call it in `main()`.

**Modified (runtime, Phase 2):**
- `runtime/scene/ViewDependentSystem.hpp` — env-sensor string compares → registry.
- `runtime/scene/TransformSystem.hpp` — `isTransform` string list → field-presence helper.

**Modified (runtime, Phase 3):**
- `runtime/events/Interpolation.hpp` — add generic `interpolateValue` / `interpolateMulti` helpers.
- `runtime/events/tests/interpolator_test.cpp` — retarget to the templates.
- **Deleted:** the 8 `*InterpolatorSystem.hpp` + their `*Behavior.hpp` shims.

**Modified (docs):**
- `docs/superpowers/specs/2026-06-18-future-proof-architecture-synthesis.md` — fold in the 5 rumination corrections (Phase 0).

---

## Phase 0 — Pin the corrected decisions in the design doc

### Task 0: Fold the rumination findings into the synthesis doc

**Files:**
- Modify: `docs/superpowers/specs/2026-06-18-future-proof-architecture-synthesis.md`

**Interfaces:**
- Consumes: nothing.
- Produces: the corrected design constraints every later phase relies on (registry is central; membership is transitive-closure; "always query the registry, never `dynamic_cast` to a concrete base"; version bumps may add bases).

- [ ] **Step 1: Add a "Corrections (2026-06-18 research pass)" section** near the top of the doc (after §1.3) capturing, verbatim, these five corrected facts:

```markdown
## 1.4 Corrections from the 2026-06-18 grounding pass

1. **Interface count & shape.** The UOM has ~74 interface types (68 abstract
   node types + 6 mixin object types), not "~30", and forms a diamond DAG (42
   concrete nodes implement >1 interface; MovieTexture has 3 bases over a
   multi-level diamond). The membership table MUST store the FULL TRANSITIVE
   CLOSURE of a node's ancestors, not its direct bases.
2. **Registry is emitted CENTRALLY, not as per-node static arrays.** One
   generated X3DInterfaceRegistry.hpp/.cpp pair (mirroring X3DNodeFactory),
   keyed by node-type name. This keeps all ~200 existing golden headers
   byte-identical. (The doc's `static constexpr InterfaceId interfaces_[]`
   per-node sketch is superseded.)
3. **Step 1 effort is ~1–2 days, not "one day".** No NodeTypeId enum exists
   today; node identity is string-keyed via X3DNodeFactory. The registry is
   still "free" of new source-of-truth: parser.py already parses
   base_type/additional_base_types and resolve_inheritance_chain() already
   computes the transitive closure.
4. **Versioning bet — add the third evolution mode.** Field evolution is purely
   additive across 3.3→4.0→4.1 (zero changed defaults/types/removals), BUT the
   spec re-parents existing concrete nodes by inserting new abstract interfaces
   (e.g. Material→X3DOneSidedMaterialNode). So the rule is: "version bumps may
   add bases to existing classes; consumers must query capability via the
   registry/InterfaceId, NEVER via dynamic_cast to a concrete C++ base." The
   registry is exactly what makes re-parenting non-breaking.
5. **USD rationale fixes + visitor.** Refusing versioned classes is right, but
   the reason is "USD versions classes because it's a data-interchange format
   needing decades-long multi-version coexistence; a compiled runtime owns its
   definitions" — NOT "breaks SFINAE/ABI". "O(1) registry" → "constant-ish,
   table-backed". The registry replaces type *identity* checks; the *visitor*
   pattern remains the right tool for graph *traversal* — the registry makes
   the visitor open/extensible (fixing OSG's closed-apply() expression problem),
   it does not replace traversal.
```

- [ ] **Step 2: Commit**

```bash
git add docs/superpowers/specs/2026-06-18-future-proof-architecture-synthesis.md
git commit -m "docs(arch): fold 2026-06-18 grounding corrections into synthesis spec"
```

---

## Phase 1 — Codegen: `InterfaceId` enum + central `InterfaceRegistry`

### Task 1: Emit the interface registry from the codegen model

**Files:**
- Create: `src/x3d_cpp_gen/emit/registry.py`
- Test: `tests/test_interface_registry.py`

**Interfaces:**
- Consumes: `parser.X3DNode` (`.name`, `.is_abstract`, `.base_type`, `.additional_base_types`), `parser.build_dependency_graph`, `parser.resolve_inheritance_chain`.
- Produces:
  - `gen_interface_registry_header(nodes: Dict[str, X3DNode]) -> str`
  - `gen_interface_registry_source(nodes: Dict[str, X3DNode], graph: Dict[str, List[str]]) -> str`
  - `interface_ids(nodes) -> List[str]` (sorted abstract names; the enum order)
  - `interfaces_of(node_name, nodes, graph) -> List[str]` (transitive-closure abstract ancestors of a node, sorted)

- [ ] **Step 1: Write the failing test**

```python
# tests/test_interface_registry.py
"""Codegen tests for the interface registry emitter."""
from importlib.resources import files
import pytest

from x3d_cpp_gen.parser import (
    parse_x3d_model, build_dependency_graph,
)
from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES
from x3d_cpp_gen.emit.registry import (
    interface_ids, interfaces_of,
    gen_interface_registry_header, gen_interface_registry_source,
)

SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")


@pytest.fixture(scope="module")
def model():
    nodes = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
    graph = build_dependency_graph(nodes)
    return nodes, graph


def test_enum_contains_core_interfaces(model):
    nodes, _ = model
    ids = interface_ids(nodes)
    for name in ("X3DTimeDependentNode", "X3DSensorNode", "X3DChildNode",
                 "X3DInterpolatorNode", "X3DGroupingNode", "X3DBoundedObject"):
        assert name in ids, f"{name} missing from InterfaceId enum"
    # The doc's "~30" was wrong: the real count is ~70+.
    assert len(ids) > 60


def test_multi_interface_node(model):
    nodes, graph = model
    # TimeSensor implements BOTH X3DTimeDependentNode and X3DSensorNode.
    ifaces = interfaces_of("TimeSensor", nodes, graph)
    assert "X3DTimeDependentNode" in ifaces
    assert "X3DSensorNode" in ifaces


def test_transitive_closure(model):
    nodes, graph = model
    # MovieTexture reaches X3DChildNode only transitively (via the sound /
    # time-dependent chain) — the closure must include it, not just direct bases.
    ifaces = interfaces_of("MovieTexture", nodes, graph)
    assert "X3DUrlObject" in ifaces
    assert "X3DTimeDependentNode" in ifaces
    assert "X3DChildNode" in ifaces


def test_header_and_source_render(model):
    nodes, graph = model
    hdr = gen_interface_registry_header(nodes)
    src = gen_interface_registry_source(nodes, graph)
    assert "enum class InterfaceId" in hdr
    assert "X3DInterfaceRegistry" in hdr
    assert "nodeImplements" in hdr
    # The TimeSensor row must list both its interfaces in the source table.
    assert "TimeSensor" in src
    assert "InterfaceId::X3DTimeDependentNode" in src
    assert "InterfaceId::X3DSensorNode" in src
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_interface_registry.py -v`
Expected: FAIL with `ModuleNotFoundError: No module named 'x3d_cpp_gen.emit.registry'`

- [ ] **Step 3: Write the emitter**

```python
# src/x3d_cpp_gen/emit/registry.py
"""Generation of X3DInterfaceRegistry.hpp/.cpp: a node-type-name -> transitive
interface-set lookup.

Behaviors must ask "does this node implement interface X" without dynamic_cast
or string-name sniffing. The registry answers that from the SAME inheritance
data the headers are generated from: every abstract node type and mixin
object-type becomes an InterfaceId; every node (concrete + abstract) maps to the
TRANSITIVE CLOSURE of its abstract ancestors. Emitted centrally (one .hpp/.cpp
pair, like X3DNodeFactory) so the per-node golden headers are untouched.
"""
from typing import Dict, List

from x3d_cpp_gen.parser import X3DNode, resolve_inheritance_chain


def interface_ids(nodes: Dict[str, X3DNode]) -> List[str]:
    """Sorted names of every abstract node type + mixin object-type.

    These are the InterfaceId enumerators. Sorting gives deterministic output
    (required for the golden gate)."""
    return sorted(n.name for n in nodes.values() if n.is_abstract)


def interfaces_of(node_name: str, nodes: Dict[str, X3DNode],
                  graph: Dict[str, List[str]]) -> List[str]:
    """Transitive-closure abstract ancestors of ``node_name`` (sorted).

    resolve_inheritance_chain already walks primary + additional bases
    recursively and de-dupes; we keep only the ABSTRACT ones (the interfaces)."""
    abstract = {n.name for n in nodes.values() if n.is_abstract}
    chain = resolve_inheritance_chain(node_name, graph, set())
    return sorted(b for b in chain if b in abstract)


def gen_interface_registry_header(nodes: Dict[str, X3DNode]) -> str:
    ids = interface_ids(nodes)
    lines: List[str] = []
    lines.append("// X3DInterfaceRegistry.hpp")
    lines.append("// Auto-generated: node-type-name -> transitive interface set.")
    lines.append("#ifndef X3D_INTERFACE_REGISTRY_HPP")
    lines.append("#define X3D_INTERFACE_REGISTRY_HPP")
    lines.append("")
    lines.append("#include <cstdint>")
    lines.append("#include <span>")
    lines.append("#include <string>")
    lines.append("#include <vector>")
    lines.append("")
    lines.append("class X3DNode;")
    lines.append("")
    lines.append("/// One enumerator per X3D abstract node type / mixin object-type.")
    lines.append("enum class InterfaceId : uint16_t {")
    for name in ids:
        lines.append(f"    {name},")
    lines.append("};")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief Queryable node-type -> interface-set registry (replaces dynamic_cast")
    lines.append(" *        / string-name type tests). Definitions live in the .cpp.")
    lines.append(" */")
    lines.append("class X3DInterfaceRegistry {")
    lines.append("public:")
    lines.append("    /// Transitive interface set for a node-type name (empty if unknown).")
    lines.append("    static std::span<const InterfaceId> interfacesOf(const std::string& nodeTypeName);")
    lines.append("    /// True if the named node-type implements ``iface`` (O(k), k = its #interfaces).")
    lines.append("    static bool nodeImplements(const std::string& nodeTypeName, InterfaceId iface);")
    lines.append("    /// Convenience overload: resolves the type name from a live node.")
    lines.append("    static bool nodeImplements(const X3DNode* node, InterfaceId iface);")
    lines.append("    /// All node-type names implementing ``iface`` (built once, sorted).")
    lines.append("    static const std::vector<std::string>& nodesImplementing(InterfaceId iface);")
    lines.append("};")
    lines.append("")
    lines.append("#endif // X3D_INTERFACE_REGISTRY_HPP")
    lines.append("")
    return "\n".join(lines)


def gen_interface_registry_source(nodes: Dict[str, X3DNode],
                                  graph: Dict[str, List[str]]) -> str:
    # Emit a row for EVERY node (concrete + abstract) so the query works on any
    # type name. Sorted for deterministic golden output.
    names = sorted(nodes)
    lines: List[str] = []
    lines.append("// X3DInterfaceRegistry.cpp")
    lines.append("// Auto-generated: the membership tables + lookups.")
    lines.append('#include "X3DInterfaceRegistry.hpp"')
    lines.append("")
    lines.append('#include "X3DNode.hpp"')
    lines.append("")
    lines.append("#include <array>")
    lines.append("#include <unordered_map>")
    lines.append("")
    lines.append("namespace {")
    lines.append("// Each node-type's transitive interface set, stored contiguously.")
    lines.append("const std::unordered_map<std::string, std::vector<InterfaceId>>& table() {")
    lines.append("    static const std::unordered_map<std::string, std::vector<InterfaceId>> t = {")
    for name in names:
        ifaces = interfaces_of(name, nodes, graph)
        ids = ", ".join(f"InterfaceId::{i}" for i in ifaces)
        lines.append(f'        {{"{name}", {{{ids}}}}},')
    lines.append("    };")
    lines.append("    return t;")
    lines.append("}")
    lines.append("}  // namespace")
    lines.append("")
    lines.append("std::span<const InterfaceId>")
    lines.append("X3DInterfaceRegistry::interfacesOf(const std::string& nodeTypeName) {")
    lines.append("    static const std::vector<InterfaceId> empty;")
    lines.append("    auto it = table().find(nodeTypeName);")
    lines.append("    return it == table().end() ? std::span<const InterfaceId>(empty)")
    lines.append("                              : std::span<const InterfaceId>(it->second);")
    lines.append("}")
    lines.append("")
    lines.append("bool X3DInterfaceRegistry::nodeImplements(const std::string& nodeTypeName,")
    lines.append("                                          InterfaceId iface) {")
    lines.append("    for (InterfaceId i : interfacesOf(nodeTypeName))")
    lines.append("        if (i == iface) return true;")
    lines.append("    return false;")
    lines.append("}")
    lines.append("")
    lines.append("bool X3DInterfaceRegistry::nodeImplements(const X3DNode* node, InterfaceId iface) {")
    lines.append("    return node && nodeImplements(node->nodeTypeName(), iface);")
    lines.append("}")
    lines.append("")
    lines.append("const std::vector<std::string>&")
    lines.append("X3DInterfaceRegistry::nodesImplementing(InterfaceId iface) {")
    lines.append("    static std::unordered_map<InterfaceId, std::vector<std::string>> cache;")
    lines.append("    auto it = cache.find(iface);")
    lines.append("    if (it != cache.end()) return it->second;")
    lines.append("    std::vector<std::string> out;")
    lines.append("    for (const auto& [name, ifaces] : table())")
    lines.append("        for (InterfaceId i : ifaces)")
    lines.append("            if (i == iface) { out.push_back(name); break; }")
    lines.append("    std::sort(out.begin(), out.end());")
    lines.append("    return cache.emplace(iface, std::move(out)).first->second;")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)
```

Note: add `#include <algorithm>` to the source emission if `std::sort` warns — append it to the include block above (`lines.append("#include <algorithm>")`).

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/test_interface_registry.py -v`
Expected: PASS (all 4 tests)

- [ ] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/emit/registry.py tests/test_interface_registry.py
git commit -m "feat(codegen): InterfaceId enum + transitive-closure registry emitter"
```

### Task 2: Wire the registry into the generator + CLI and regenerate

**Files:**
- Modify: `src/x3d_cpp_gen/generator.py`
- Modify: `src/x3d_cpp_gen/cli.py:168` (after `write_node_factory`)
- Create (generated, committed): `generated_cpp_bindings/X3DInterfaceRegistry.hpp` / `.cpp`

**Interfaces:**
- Consumes: `emit.registry.gen_interface_registry_header/_source`, `parser.build_dependency_graph`.
- Produces: `generator.write_interface_registry(output_dir, nodes, graph)`.

- [ ] **Step 1: Add `write_interface_registry` to generator.py**

Add after `write_node_factory` (generator.py ~line 298):

```python
def write_interface_registry(output_dir: str, nodes: Dict[str, X3DNode],
                             dependency_graph) -> None:
    """Write X3DInterfaceRegistry.hpp/.cpp (the interface membership tables)."""
    from x3d_cpp_gen.emit.registry import (
        gen_interface_registry_header, gen_interface_registry_source,
    )
    hdr = os.path.join(output_dir, "X3DInterfaceRegistry.hpp")
    with open(hdr, "w") as f:
        f.write(gen_interface_registry_header(nodes))
    print(f"Generated X3D interface registry header at {hdr}")
    src = os.path.join(output_dir, "X3DInterfaceRegistry.cpp")
    with open(src, "w") as f:
        f.write(gen_interface_registry_source(nodes, dependency_graph))
    print(f"Generated X3D interface registry source at {src}")
```

- [ ] **Step 2: Call it from cli.py**

In `cli.py`, immediately after the `write_node_factory(str(out_dir), nodes)` line (~168), add:

```python
    write_interface_registry(str(out_dir), nodes, dependency_graph)
```

And add `write_interface_registry` to the import block from `x3d_cpp_gen.generator` (cli.py lines 18–23).

- [ ] **Step 3: Regenerate the bindings**

Run: `mise run gen`
Expected: console shows `Generated ... X3DInterfaceRegistry.hpp` and `.cpp`; both files now exist under `generated_cpp_bindings/`.

- [ ] **Step 4: Verify NO existing header drifted, then confirm the new header is present**

Run: `git status --short generated_cpp_bindings/`
Expected: ONLY `X3DInterfaceRegistry.hpp` and `X3DInterfaceRegistry.cpp` appear as new (`??`). NO existing `*.hpp` shows as modified (`M`). If any existing header changed, STOP — the emitter or template was touched incorrectly.

Run: `mise run golden`
Expected: PASS (the regenerated tree, including the new committed-to-be header, matches once staged). If it reports the new header as drift, that is expected until Step 5 stages it; re-run after staging.

- [ ] **Step 5: Build to confirm the generated .cpp compiles into the node lib**

Run: `mise run build`
Expected: configure picks up `generated_cpp_bindings/X3DInterfaceRegistry.cpp` via the existing glob; build succeeds; ctest 110/110 (no behavior changed yet).

- [ ] **Step 6: Commit**

```bash
git add src/x3d_cpp_gen/generator.py src/x3d_cpp_gen/cli.py \
        generated_cpp_bindings/X3DInterfaceRegistry.hpp \
        generated_cpp_bindings/X3DInterfaceRegistry.cpp
git commit -m "feat(codegen): generate + wire X3DInterfaceRegistry into the binding tree"
```

### Task 3: Prove the runtime registry API with a C++ ctest

**Files:**
- Create: `runtime/scene/tests/interface_registry_test.cpp`
- Modify: `CMakeLists.txt` (add the test target near the other `runtime/scene/tests` targets)

**Interfaces:**
- Consumes: `X3DInterfaceRegistry::nodeImplements`, `createX3DNode`, `InterfaceId`.
- Produces: ctest `x3d_interface_registry`.

- [ ] **Step 1: Write the failing test**

```cpp
// runtime/scene/tests/interface_registry_test.cpp
#include "X3DInterfaceRegistry.hpp"
#include "X3DNodeFactory.hpp"

#include <cassert>
#include <iostream>

int main() {
  // String-keyed queries.
  assert(X3DInterfaceRegistry::nodeImplements("TimeSensor",
                                              InterfaceId::X3DTimeDependentNode));
  assert(X3DInterfaceRegistry::nodeImplements("TimeSensor",
                                              InterfaceId::X3DSensorNode));
  // Transitive closure: ProximitySensor is-a X3DEnvironmentalSensorNode and,
  // transitively, X3DSensorNode + X3DChildNode.
  assert(X3DInterfaceRegistry::nodeImplements(
      "ProximitySensor", InterfaceId::X3DEnvironmentalSensorNode));
  assert(X3DInterfaceRegistry::nodeImplements("ProximitySensor",
                                              InterfaceId::X3DChildNode));
  // Negative case.
  assert(!X3DInterfaceRegistry::nodeImplements("Box",
                                               InterfaceId::X3DSensorNode));
  // Unknown type -> empty -> false (no crash).
  assert(!X3DInterfaceRegistry::nodeImplements("__nope__",
                                               InterfaceId::X3DChildNode));
  // Live-node overload via the factory.
  auto n = createX3DNode("TimeSensor");
  assert(n);
  assert(X3DInterfaceRegistry::nodeImplements(n.get(),
                                              InterfaceId::X3DSensorNode));

  std::cout << "interface_registry_test OK\n";
  return 0;
}
```

- [ ] **Step 2: Register the ctest target in CMakeLists.txt**

Add alongside the other `runtime/scene/tests` executables (search the file for an existing `add_executable(... runtime/scene/tests/...)` block and mirror it):

```cmake
    add_executable(x3d_interface_registry
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/interface_registry_test.cpp")
    target_link_libraries(x3d_interface_registry PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_interface_registry COMMAND x3d_interface_registry)
```

- [ ] **Step 3: Run it to verify it builds and passes**

Run: `mise run build`
Expected: `x3d_interface_registry` builds; ctest now 111/111, including `x3d_interface_registry ... Passed`.

- [ ] **Step 4: Commit**

```bash
git add runtime/scene/tests/interface_registry_test.cpp CMakeLists.txt
git commit -m "test(runtime): C++ ctest for X3DInterfaceRegistry queries"
```

---

## Phase 2 — Replace string-name type tests with registry queries

> Scope discipline: target the genuine *type-identity* rot — string-name comparisons and membership-only `dynamic_cast`s. Where a `dynamic_cast` also yields a typed pointer the code then *uses*, leave it (the registry cannot hand back a typed pointer; that is idiomatic, not rot). Two worked tasks below cover the two highest-value, cleanly-registry-addressable sites. The long tail (the `nodeTypeName() == "..."` sites enumerated in Step 1 of Task 4) follows the same pattern and is promoted to its own follow-up plan.

### Task 4: ViewDependentSystem env-sensor detection → registry

**Files:**
- Modify: `runtime/scene/ViewDependentSystem.hpp:73-74`
- Test: existing `runtime/scene/tests/view_dependent_test.cpp` (regression only)

**Interfaces:**
- Consumes: `X3DInterfaceRegistry::nodeImplements`, `InterfaceId::X3DEnvironmentalSensorNode`.
- Produces: no API change (internal substitution).

- [ ] **Step 1: Confirm the current string compares and the interface that covers them**

Run: `grep -n 'ProximitySensor\|VisibilitySensor' runtime/scene/ViewDependentSystem.hpp`
Expected: lines ~73–74 compare `nodeTypeName()` against the two literals. Both nodes derive from `X3DEnvironmentalSensorNode` (verified: `class ProximitySensor : public virtual X3DEnvironmentalSensorNode`).

- [ ] **Step 2: Add the include and replace the string test**

At the top includes of `ViewDependentSystem.hpp`, add:

```cpp
#include "X3DInterfaceRegistry.hpp"
```

Replace the two-literal check (the `t == "ProximitySensor" || t == "VisibilitySensor"` form at lines 73–74) with:

```cpp
    // Environmental sensors (Proximity/Visibility/Transform...) are exactly the
    // X3DEnvironmentalSensorNode interface — query the registry, not type names.
    if (X3DInterfaceRegistry::nodeImplements(n, InterfaceId::X3DEnvironmentalSensorNode)) {
```

Adjust the surrounding `if`/local-variable lines so the conditional body is unchanged (remove the now-unused `nodeTypeName()` temporary if it is only used here).

- [ ] **Step 3: Run the regression test**

Run: `mise run build`
Expected: `x3d_view_dependent` (or the view-dependent ctest) still PASSES; ctest count unchanged (111/111). The registry-based check must accept every node the literal list did (note: `TransformSensor` or other env-sensors now also match — confirm `view_dependent_test.cpp` expectations still hold; if a newly-matched type changes behavior, that is a real coverage gain, validate it is correct).

- [ ] **Step 4: Verify golden + corpus unaffected**

Run: `mise run golden && mise run corpus`
Expected: golden PASS (no generated file touched); corpus smoke PASS.

- [ ] **Step 5: Commit**

```bash
git add runtime/scene/ViewDependentSystem.hpp
git commit -m "refactor(runtime): env-sensor detection via InterfaceRegistry, not type names"
```

### Task 5: TransformSystem::isTransform → field-presence, not a hard-coded name list

**Files:**
- Modify: `runtime/scene/TransformSystem.hpp:71-76`
- Test: existing transform/bounds ctests (regression)

**Interfaces:**
- Consumes: `X3DNode::fields()` (the reflected `FieldTable`).
- Produces: no API change (`isTransform` keeps its signature).

> Rationale: "transform-bearing" is NOT a single X3D interface (Transform is X3DGroupingNode; HAnimJoint is X3DChildNode; CADPart is X3DProductStructureChildNode — no shared "has-TRS" base). The future-proof test is field presence: a node carries a full TRS frame iff it reflects `translation` + `rotation` fields. This survives new X3D 4.x transform-bearing types with zero edits.

- [ ] **Step 1: Write a failing assertion into a focused test**

Create `runtime/scene/tests/is_transform_test.cpp`:

```cpp
#include "TransformSystem.hpp"
#include "X3DNodeFactory.hpp"

#include <cassert>
#include <iostream>

using x3d::runtime::TransformSystem;

int main() {
  for (const char* t : {"Transform", "HAnimHumanoid", "HAnimJoint", "CADPart"}) {
    auto n = createX3DNode(t);
    assert(n && "factory should create transform-bearing node");
    assert(TransformSystem::isTransform(n.get()) && t);
  }
  // A non-transform node must NOT be treated as transform-bearing.
  auto box = createX3DNode("Box");
  assert(box && !TransformSystem::isTransform(box.get()));
  // Group has children but no TRS frame -> not transform-bearing.
  auto grp = createX3DNode("Group");
  assert(grp && !TransformSystem::isTransform(grp.get()));
  std::cout << "is_transform_test OK\n";
  return 0;
}
```

Register the target in `CMakeLists.txt` (mirror Task 3 Step 2):

```cmake
    add_executable(x3d_is_transform
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/is_transform_test.cpp")
    target_link_libraries(x3d_is_transform PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_is_transform COMMAND x3d_is_transform)
```

- [ ] **Step 2: Run it against the CURRENT (string-list) implementation to confirm it passes today**

Run: `mise run build`
Expected: `x3d_is_transform` PASSES against the existing 4-name list (this pins behavior BEFORE the refactor, so the refactor is provably equivalent).

- [ ] **Step 3: Replace the string list with a field-presence check**

In `TransformSystem.hpp`, replace the `isTransform` body (lines 71–76) with:

```cpp
  // A node is transform-bearing iff it reflects a full TRS frame. We test for
  // the two defining fields (translation + rotation); this is future-proof
  // across new X3D transform-bearing node types (no hard-coded name list).
  static bool isTransform(const X3DNode *n) {
    if (!n) return false;
    bool hasT = false, hasR = false;
    for (const auto &f : n->fields()) {
      if (f.x3dName == "translation") hasT = true;
      else if (f.x3dName == "rotation") hasR = true;
      if (hasT && hasR) return true;
    }
    return false;
  }
```

- [ ] **Step 4: Run the pinning test + full regression**

Run: `mise run build`
Expected: `x3d_is_transform` STILL passes (equivalence proven), plus the existing transform/bounds ctests pass. ctest count = 112/112.

> If any existing transform ctest fails, a transform-bearing type in the 4-name list lacks a `translation`/`rotation` field, or a non-transform type has both. Investigate with `grep`; do NOT widen the test to paper over a real semantic difference.

- [ ] **Step 5: Verify golden + corpus**

Run: `mise run golden && mise run corpus`
Expected: both PASS.

- [ ] **Step 6: Commit**

```bash
git add runtime/scene/TransformSystem.hpp runtime/scene/tests/is_transform_test.cpp CMakeLists.txt
git commit -m "refactor(runtime): isTransform via reflected TRS fields, not a name list"
```

---

## Phase 3 — Collapse 8 interpolator systems into 2 templated systems

### Task 6: Generic interpolation helpers in Interpolation.hpp

**Files:**
- Modify: `runtime/events/Interpolation.hpp`
- Test: `runtime/events/tests/interpolator_test.cpp` (extended in Task 8)

**Interfaces:**
- Consumes: existing `locateKeySpan`, `KeySpan{lo,hi,t,clamped}`, `lerpf`, `lerpVec2`, `lerpVec3`, `lerpColorHsv`, `slerpRotation`, `slerpNormal`.
- Produces:
  - `template<typename T, typename Lerp> T interpolateValue(const MFFloat&, const std::vector<T>&, float, Lerp)`
  - `template<typename T, typename Lerp> std::vector<T> interpolateMulti(const MFFloat&, const std::vector<T>&, float, Lerp)`

- [ ] **Step 1: Add the helpers (with includes)**

Ensure `Interpolation.hpp` includes `<algorithm>`, `<cstddef>`, `<vector>` (add any missing). Then add, inside `namespace x3d::runtime`, after the existing lerp/slerp functions:

```cpp
// Generic single-value interpolation: clamp at the ends, else lerp the
// bracketing keyValues with the supplied function. Covers Scalar/Position/
// Position2D/Color/Orientation (the only variation is the value type + lerp fn).
template <typename T, typename Lerp>
T interpolateValue(const MFFloat &key, const std::vector<T> &keyValue,
                   float fraction, Lerp lerp) {
  if (key.empty() || keyValue.empty()) return T{};
  KeySpan s = locateKeySpan(key, fraction);
  if (s.clamped || s.hi >= keyValue.size())
    return keyValue[std::min(s.lo, keyValue.size() - 1)];
  return lerp(keyValue[s.lo], keyValue[s.hi], s.t);
}

// Generic multi-value interpolation: the flat keyValue (numKeys * numPoints) is
// reshaped by numKeys; each output element blends the bracketing rows with the
// supplied per-element function. Covers Coordinate/Coordinate2D/Normal.
template <typename T, typename Lerp>
std::vector<T> interpolateMulti(const MFFloat &key,
                                const std::vector<T> &keyValue, float fraction,
                                Lerp lerp) {
  if (key.empty() || keyValue.empty()) return {};
  std::size_t numKeys = key.size();
  std::size_t numPoints = keyValue.size() / numKeys;
  if (numPoints == 0) return {};
  KeySpan s = locateKeySpan(key, fraction);
  std::vector<T> out;
  out.reserve(numPoints);
  if (s.clamped) {
    std::size_t base = s.lo * numPoints;
    for (std::size_t p = 0; p < numPoints; ++p) out.push_back(keyValue[base + p]);
    return out;
  }
  std::size_t baseLo = s.lo * numPoints, baseHi = s.hi * numPoints;
  for (std::size_t p = 0; p < numPoints; ++p)
    out.push_back(lerp(keyValue[baseLo + p], keyValue[baseHi + p], s.t));
  return out;
}
```

- [ ] **Step 2: Build to confirm the helpers compile (no callers yet)**

Run: `mise run build`
Expected: build succeeds; ctest unchanged (112/112). The templates are uninstantiated, so this just proves they parse.

- [ ] **Step 3: Commit**

```bash
git add runtime/events/Interpolation.hpp
git commit -m "feat(runtime): generic interpolateValue/interpolateMulti helpers"
```

### Task 7: The two templated interpolator systems + registration helper

**Files:**
- Create: `runtime/events/InterpolatorSystem.hpp`
- Create: `runtime/events/InterpolatorRegistration.hpp`

**Interfaces:**
- Consumes: `System`, `X3DExecutionContext::postEvent`, `interpolateValue`/`interpolateMulti`, the lerp functions, the concrete interpolator node classes.
- Produces:
  - `template<typename NodeT, typename ValueT> class InterpolatorSystem : public System`
  - `template<typename NodeT, typename ElemT> class MultiInterpolatorSystem : public System`
  - `void registerInterpolatorSystems(X3DExecutionContext& ctx)`

- [ ] **Step 1: Write the templated systems**

```cpp
// InterpolatorSystem.hpp
// Two templated event-driven Systems replacing the 8 per-type interpolator
// systems. Single-value family (Scalar/Position/Position2D/Color/Orientation):
// InterpolatorSystem<NodeT, ValueT>. Multi-value family
// (Coordinate/Coordinate2D/Normal): MultiInterpolatorSystem<NodeT, ElemT>.
// Each is constructed with the per-type lerp function. The wiring
// (set_fraction handler -> value_changed via the cascade) is identical across
// all of them, which is exactly why they collapse.
#ifndef X3D_RUNTIME_INTERPOLATOR_SYSTEM_HPP
#define X3D_RUNTIME_INTERPOLATOR_SYSTEM_HPP

#include "Interpolation.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include <any>
#include <functional>
#include <utility>

namespace x3d::runtime {

/// Single-value interpolator: keyValue is std::vector<ValueT>, output ValueT.
template <typename NodeT, typename ValueT>
class InterpolatorSystem : public System {
public:
  using LerpFn = std::function<ValueT(const ValueT &, const ValueT &, float)>;
  explicit InterpolatorSystem(LerpFn lerp) : lerp_(std::move(lerp)) {}

  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *interp = dynamic_cast<NodeT *>(node);
    if (!interp) return;
    LerpFn lerp = lerp_;
    interp->setOnSet_fractionHandler(
        [&ctx, interp, lerp](const SFFloat &fraction) {
          ctx.postEvent(interp, "value_changed",
                        std::any(interpolateValue(interp->getKey(),
                                                  interp->getKeyValue(),
                                                  fraction, lerp)));
        });
  }

private:
  LerpFn lerp_;
};

/// Multi-value interpolator: keyValue is a flat std::vector<ElemT> reshaped by
/// numKeys; output std::vector<ElemT>.
template <typename NodeT, typename ElemT>
class MultiInterpolatorSystem : public System {
public:
  using LerpFn = std::function<ElemT(const ElemT &, const ElemT &, float)>;
  explicit MultiInterpolatorSystem(LerpFn lerp) : lerp_(std::move(lerp)) {}

  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *interp = dynamic_cast<NodeT *>(node);
    if (!interp) return;
    LerpFn lerp = lerp_;
    interp->setOnSet_fractionHandler(
        [&ctx, interp, lerp](const SFFloat &fraction) {
          ctx.postEvent(interp, "value_changed",
                        std::any(interpolateMulti(interp->getKey(),
                                                  interp->getKeyValue(),
                                                  fraction, lerp)));
        });
  }

private:
  LerpFn lerp_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_INTERPOLATOR_SYSTEM_HPP
```

- [ ] **Step 2: Write the registration helper (the "8 lines")**

```cpp
// InterpolatorRegistration.hpp
// One call wires every interpolator System into an execution context. This is
// the registry-of-behavior seam that replaces 8 separate per-type files.
#ifndef X3D_RUNTIME_INTERPOLATOR_REGISTRATION_HPP
#define X3D_RUNTIME_INTERPOLATOR_REGISTRATION_HPP

#include "InterpolatorSystem.hpp"
#include "X3DExecutionContext.hpp"

#include "ColorInterpolator.hpp"
#include "CoordinateInterpolator.hpp"
#include "CoordinateInterpolator2D.hpp"
#include "NormalInterpolator.hpp"
#include "OrientationInterpolator.hpp"
#include "PositionInterpolator.hpp"
#include "PositionInterpolator2D.hpp"
#include "ScalarInterpolator.hpp"

#include <memory>

namespace x3d::runtime {

inline void registerInterpolatorSystems(X3DExecutionContext &ctx) {
  ctx.addSystem(std::make_shared<InterpolatorSystem<ScalarInterpolator, float>>(
      [](const float &a, const float &b, float t) { return lerpf(a, b, t); }));
  ctx.addSystem(std::make_shared<InterpolatorSystem<PositionInterpolator, SFVec3f>>(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); }));
  ctx.addSystem(std::make_shared<InterpolatorSystem<PositionInterpolator2D, SFVec2f>>(
      [](const SFVec2f &a, const SFVec2f &b, float t) { return lerpVec2(a, b, t); }));
  ctx.addSystem(std::make_shared<InterpolatorSystem<ColorInterpolator, SFColor>>(
      [](const SFColor &a, const SFColor &b, float t) { return lerpColorHsv(a, b, t); }));
  ctx.addSystem(std::make_shared<InterpolatorSystem<OrientationInterpolator, SFRotation>>(
      [](const SFRotation &a, const SFRotation &b, float t) { return slerpRotation(a, b, t); }));
  ctx.addSystem(std::make_shared<MultiInterpolatorSystem<CoordinateInterpolator, SFVec3f>>(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); }));
  ctx.addSystem(std::make_shared<MultiInterpolatorSystem<CoordinateInterpolator2D, SFVec2f>>(
      [](const SFVec2f &a, const SFVec2f &b, float t) { return lerpVec2(a, b, t); }));
  ctx.addSystem(std::make_shared<MultiInterpolatorSystem<NormalInterpolator, SFVec3f>>(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return slerpNormal(a, b, t); }));
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_INTERPOLATOR_REGISTRATION_HPP
```

- [ ] **Step 3: Build to confirm both new headers compile (still alongside the old ones)**

Run: `mise run build`
Expected: build succeeds; ctest unchanged (112/112). Both families instantiate; the old per-type files still exist (deleted in Task 8).

- [ ] **Step 4: Commit**

```bash
git add runtime/events/InterpolatorSystem.hpp runtime/events/InterpolatorRegistration.hpp
git commit -m "feat(runtime): templated InterpolatorSystem + MultiInterpolatorSystem + registration"
```

### Task 8: Retarget the tests, delete the 8 old systems + their shims

**Files:**
- Modify: `runtime/events/tests/interpolator_test.cpp`
- Delete: the 8 `runtime/events/*InterpolatorSystem.hpp` + every `*InterpolatorBehavior.hpp` shim that references them
- Modify: any remaining includer found by grep (e.g. `animation_test.cpp`)

**Interfaces:**
- Consumes: the Task 7 templates + helpers.
- Produces: no public API beyond Task 7 (cleanup task).

- [ ] **Step 1: Inventory every reference to the old systems/shims**

Run:
```bash
grep -rln "InterpolatorSystem\|InterpolatorBehavior" runtime/ | sort
ls runtime/events/*InterpolatorSystem.hpp runtime/events/*InterpolatorBehavior.hpp
```
Expected: the 8 `*InterpolatorSystem.hpp`, any `*InterpolatorBehavior.hpp` shims, `runtime/events/tests/interpolator_test.cpp`, and possibly `animation_test.cpp`. Record the exact list — every file here must be updated or deleted in this task.

- [ ] **Step 2: Retarget `interpolator_test.cpp` to the templates**

Replace its `#include "*InterpolatorSystem.hpp"` lines with:

```cpp
#include "InterpolatorSystem.hpp"
#include "InterpolatorRegistration.hpp"
#include "Interpolation.hpp"
```

Replace each use of a per-type system (e.g. `ScalarInterpolatorSystem sys; sys.attach(...)`) with the templated form, e.g.:

```cpp
  using x3d::runtime::InterpolatorSystem;
  InterpolatorSystem<ScalarInterpolator, float> sys(
      [](const float &a, const float &b, float t) { return lerpf(a, b, t); });
  sys.attach(interp.get(), ctx);
```

Replace each direct `*InterpolatorSystem::interpolate(key, keyValue, frac)` static call with the free helper, e.g.:

```cpp
  auto v = x3d::runtime::interpolateValue(
      key, keyValue, frac,
      [](const SFColor &a, const SFColor &b, float t) { return lerpColorHsv(a, b, t); });
```

(For Coordinate/Normal cases use `interpolateMulti` with the matching element lerp.)

- [ ] **Step 3: Fix any other includer**

For each non-test file still referencing a deleted header (from Step 1), repoint it: replace the per-type include with `InterpolatorRegistration.hpp` and call `registerInterpolatorSystems(ctx)`, or use the templated system directly. Delete `*InterpolatorBehavior.hpp` shims outright once their includers are repointed.

- [ ] **Step 4: Delete the 8 old system headers + the shims**

```bash
git rm runtime/events/ColorInterpolatorSystem.hpp \
       runtime/events/ScalarInterpolatorSystem.hpp \
       runtime/events/PositionInterpolatorSystem.hpp \
       runtime/events/PositionInterpolator2DSystem.hpp \
       runtime/events/OrientationInterpolatorSystem.hpp \
       runtime/events/CoordinateInterpolatorSystem.hpp \
       runtime/events/CoordinateInterpolator2DSystem.hpp \
       runtime/events/NormalInterpolatorSystem.hpp
# plus any *InterpolatorBehavior.hpp shims surfaced in Step 1, e.g.:
git rm runtime/events/PositionInterpolatorBehavior.hpp
```

- [ ] **Step 5: Build + full regression**

Run: `mise run build`
Expected: NO unresolved-include or missing-symbol errors; `x3d_event_interpolators` PASSES; ctest count back to 112/112 (same as before Task 8; the interpolator test target is preserved). Then:

Run: `mise run golden && mise run corpus`
Expected: golden PASS (no generated file touched); corpus smoke PASS (animation/interpolator scenes still tick correctly).

- [ ] **Step 6: Commit**

```bash
git add -A runtime/events/
git commit -m "refactor(runtime): collapse 8 interpolator systems into 2 templated systems"
```

---

## Phases 4–7 — Outlined (promote each to its own plan at kickoff)

> Per the writing-plans Scope Check, the remaining doc steps span independent subsystems and should each become their own bite-sized plan when started. Phases 1–3 above are complete, shippable software on their own. The outlines below carry the files, approach, and exit gate so a kickoff can expand them.

### Phase 4 — `ComponentStore<T>` (doc Step 3), gated on profiling evidence

- **Files:** create `runtime/ecs/ComponentStore.hpp` (~150 LOC, SoA keyed by node pointer, no deps) + `runtime/ecs/tests/component_store_test.cpp`. Pilot consumer: `runtime/events/X3DTimeDependentSystem.hpp:322` (`std::unordered_map<X3DTimeDependentNode*, State> state_`).
- **Approach:** add the store + unit test first. Then add a micro-benchmark over a representative corpus scene (10–1000 time-dependent nodes). Swap `state_` to `ComponentStore<State>` ONLY if it measurably wins; otherwise keep the map and record the measurement in the commit message. This honors the doc's "ECS-flavored only on profiling evidence" rule and the field's hybrid-DOD caution (keep the store derived/disposable; never a second source of truth).
- **Exit gate:** ctest green; golden + corpus green; a written measurement (win → adopt; no win → documented and store left available for hot paths).

### Phase 5 — SceneExtractor → scene-index filter chain (doc Step 4 / Godot doc Phase 0–1)

- **Files:** define `runtime/extract/SceneIndex.hpp` (the filter interface: pull-based `RenderItem` access + a coarse dirty signal). Refactor `runtime/extract/SceneExtractor.hpp` into a *source* filter + at least one downstream filter (transform-flatten) as the proof, behind the same public extract API.
- **Approach:** borrow Hydra's *shape* (chain of single-purpose, observable projections) but NOT the full `HdDataSourceLocator` machinery — a coarse dirty model is sufficient for an embedded runtime (per the USD fact-check). Each filter independently testable; existing extractor tests map onto filter tests. Pin the `SceneIndex`/`RenderItem` contract as the stable embedder surface (Godot doc Phase 0).
- **Exit gate:** existing extractor/render ctests pass through the new chain; golden + corpus green; the standalone reference-viewer (Godot doc Phase 1) can consume the same `RenderItem` projection.

### Phase 6 — Custom-code preservation pilot (doc Step 6)

- **Files:** `src/x3d_cpp_gen/templates/class_template.hpp.jinja` + the backend.
- **Approach:** emit `// --(BEGIN CUSTOM CODE)--` / `--(END CUSTOM CODE)--` markers in generated headers; teach `CppHeaderBackend.emit` to preserve any text between markers on regeneration. Prove it by hand-editing one node, regenerating, and confirming the block survives. NOTE: this WILL change every golden header (the markers), so it requires a deliberate golden re-baseline — do it as its own change, isolated from Phases 1–3.
- **Exit gate:** round-trip test (edit → regen → block preserved); golden re-baselined and committed in one reviewable diff; corpus green.

### Phase 7 — Version-resilience decision (doc Step 7)

- Decision only — already pinned in Phase 0 (Task 0, correction #4): version bumps may add new node classes, add fields with spec defaults, AND add bases to existing classes; consumers query capability via the registry, never `dynamic_cast` to a concrete base. No implementation until an actual X3D 4.2 UOM lands; at that point regenerate and confirm the registry absorbs any re-parenting with zero behavior-layer edits.

---

## Self-Review

- **Spec coverage:** doc Steps 1–7 all mapped — Step 1 → Phase 1 (Tasks 1–3); the `dynamic_cast`/string-list rot → Phase 2 (Tasks 4–5); Step 2 (interpolator collapse) → Phase 3 (Tasks 6–8); Step 3 → Phase 4; Step 4 → Phase 5; Step 5 (doc rule) → Phase 0; Step 6 → Phase 6; Step 7 → Phase 0 + Phase 7. The 5 rumination corrections → Phase 0.
- **Type consistency:** `gen_interface_registry_header(nodes)` / `gen_interface_registry_source(nodes, graph)` signatures match across Tasks 1–2; `nodeImplements(const std::string&|const X3DNode*, InterfaceId)` used identically in Tasks 3–4; `InterpolatorSystem<NodeT, ValueT>` / `MultiInterpolatorSystem<NodeT, ElemT>` and `registerInterpolatorSystems(ctx)` consistent across Tasks 7–8; `interpolateValue`/`interpolateMulti` signatures consistent across Tasks 6–8.
- **Golden safety:** Phases 1–3 add new generated files and edit `runtime/` only; no existing `generated_cpp_bindings/*.hpp` byte content changes (Phase 6, which does, is explicitly isolated with a re-baseline step).
```

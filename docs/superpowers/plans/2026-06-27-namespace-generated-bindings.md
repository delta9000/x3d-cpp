# Namespace the Generated Bindings (`x3d::core` + `x3d::nodes`) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move all 685 generated X3D binding types out of the global namespace into `x3d::core` (vocabulary) and `x3d::nodes` (node classes), with headers physically laid out under `x3d/core/` and `x3d/nodes/` to match (API-1 + API-2).

**Architecture:** The entire `generated_cpp_bindings/` tree is emitter output, so the type *definitions* move via a focused change in `src/x3d_cpp_gen/` plus a regen. The emitter already has a tested node-class namespace wrapper (`--namespace`, byte-identical when empty); we set it to `x3d::nodes` and add new `x3d::core` wrapping for the three foundation headers, the subdir layout, and the `SFNode`/`X3DNode` cross-namespace seam. Consumers (~110 files) migrate by dependency layer so each layer compiles once the layers beneath it are done.

**Tech Stack:** Python 3 generator (Jinja2 templates + `emit/` package), C++20 header-only runtime, CMake + Ninja + ccache, mise task runner, doctest, pytest.

## Global Constraints

- **Two namespaces, no shim.** `x3d::core` = `SF*/MF*` value types + reflection primitives (`FieldInfo`/`FieldTable`/`NodeVisitor`/`RangeDiagnostic`/`X3DFieldType`/`AccessType`) + bounded enums. `x3d::nodes` = `X3DNode` base + all abstract `X3D*Node` + all concrete node classes + the factory + the interface registry. No compatibility shim, no transitional global `using namespace`.
- **No global-namespace pollution.** Generated and consumer headers may use `using namespace x3d::core;` / `x3d::nodes;` only **inside** a named namespace block (`x3d::nodes`, `x3d::runtime`, …) — never at global/file scope in a header. `.cpp` and test TUs may use file-scope using-directives.
- **Header path spelling is uniform:** `#include "x3d/core/X3Dtypes.hpp"`, `#include "x3d/nodes/Transform.hpp"` — identical in-tree and installed (in-tree include base is `generated_cpp_bindings/`).
- **X3D string identity is preserved.** `nodeTypeName()` still returns `"Transform"`; factory/registry string keys, parsing, and serialization are unchanged. The *behavioral* goldens/conformance view must not change; only the *binding-hash* golden is re-blessed.
- **Include guards:** generated headers use `#pragma once` (replaces bare `APPEARANCE_HPP` macros — kills macro pollution).
- **Green bar = `mise run ci`** (test + golden + conformance-gate + coverage-gate + build-ci with per-header isolation ON + cli-gate-regression). Docs strict gate = `mise run docs-build`.
- **Commit conventions:** no `Claude-Session:` trailers or `claude.ai/code/session_…` URLs in commits/PR.

> **Spec deviations to confirm before execution:** The approved spec said node/consumer headers would *fully qualify* (`x3d::core::SFVec3f`). This plan instead uses **scoped using-directives inside named namespaces** (never global) for generated node headers (Task 2) and consumer headers (Tasks 7–11). Same external API; ~100× less churn; global namespace still clean. If you want literal full-qualification instead, Tasks 2 and 7–11 change substantially — confirm before starting.

---

## File Structure

**Generator (definitions):**
- `src/x3d_cpp_gen/cli.py` — orchestrate the `x3d/core/` + `x3d/nodes/` subdir output; set node namespace to `x3d::nodes`.
- `src/x3d_cpp_gen/generator.py` — `write_types_header` / `write_enums_header` / `write_reflection_header` wrap in `namespace x3d::core`, emit `#pragma once`, write to the core subdir, add the `x3d::nodes::X3DNode` forward decl for `SFNode`.
- `src/x3d_cpp_gen/emit/reflection.py` — wrap `X3DReflection.hpp` body in `namespace x3d::core`.
- `src/x3d_cpp_gen/templates/class_template.hpp.jinja`, `class_template.cpp.jinja` — `#pragma once`, `x3d/core/` + `x3d/nodes/` includes, scoped `using namespace x3d::core;`.
- `src/x3d_cpp_gen/emit/factory.py`, `emit/registry.py` — wrap in `namespace x3d::nodes`, fix the `X3DNode` forward decl, update includes.
- `src/x3d_cpp_gen/backends/cpp_header.py` — emit node headers into the `x3d/nodes/` subdir.
- `src/x3d_cpp_gen/templates/test_template.cpp.jinja` — qualify with `x3d::nodes::` / `x3d::core::`.

**Build:**
- `CMakeLists.txt` — include base stays `generated_cpp_bindings/`; `.cpp` glob → `generated_cpp_bindings/x3d/nodes/*.cpp`; `install(DIRECTORY …/generated_cpp_bindings/x3d/ …)`; smoke-test path → `x3d/nodes/test.cpp`.

**Consumers (by dependency layer):** `runtime/` document model → `runtime/codecs/` → `runtime/extract/`, `runtime/events/`, `runtime/scene/`, `runtime/script/`, `runtime/sound/`, `runtime/physics/`, `runtime/parse/` → `include/x3d/sdk.hpp` → `tools/`, `examples/` → all `**/tests/`.

**Docs:** new ADR `docs/wiki/decisions/0039-generated-binding-namespaces.md`; `docs/wiki/subsystems/generated-bindings.md`; `docs/wiki/coverage.md`; `mkdocs.yml`.

**Helper script (created Task 6, used Tasks 7–12):** `scripts/migrate_ns_includes.sh` — rewrites bare generated-header includes to `x3d/core/…` / `x3d/nodes/…` across a path argument.

---

## Task 1: Generator emits `x3d::core` foundation headers into `x3d/core/`

**Files:**
- Modify: `src/x3d_cpp_gen/generator.py` (`gen_types_header`, `write_types_header`, `gen_enums_header`, `write_enums_header`, `write_reflection_header`)
- Modify: `src/x3d_cpp_gen/emit/reflection.py` (`gen_reflection_header`)
- Test: `tests/test_core_namespace_emit.py` (new)

**Interfaces:**
- Produces: three headers at `<out>/x3d/core/{X3Dtypes,X3Denums,X3DReflection}.hpp`, each `#pragma once`, each body inside `namespace x3d::core { … }`. `X3Dtypes.hpp` contains `namespace x3d::nodes { class X3DNode; }` before the `x3d::core` block, and defines `using SFNode = std::shared_ptr<nodes::X3DNode>;` inside `x3d::core`.

- [ ] **Step 1: Write the failing test**

```python
# tests/test_core_namespace_emit.py
from x3d_cpp_gen.generator import gen_types_header, gen_enums_header
from x3d_cpp_gen.emit.reflection import gen_reflection_header

def test_types_header_wraps_core_and_forward_decls_x3dnode():
    h = gen_types_header()
    assert "#pragma once" in h
    assert "namespace x3d::nodes { class X3DNode; }" in h
    assert "namespace x3d::core {" in h
    assert "} // namespace x3d::core" in h
    # SFNode points at the nodes-namespace X3DNode:
    assert "using SFNode = std::shared_ptr<nodes::X3DNode>;" in h

def test_enums_and_reflection_wrap_core():
    assert "namespace x3d::core {" in gen_enums_header({})
    assert "namespace x3d::core {" in gen_reflection_header()
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_core_namespace_emit.py -v`
Expected: FAIL — current output has bare `#ifndef X3D_ENUMS_HPP` guards, no namespace, `typedef std::shared_ptr<X3DNode> SFNode;`.

- [ ] **Step 3: Implement — wrap the three foundation headers**

In `generator.py` `gen_types_header()`: replace the `#ifndef/#define … #endif` guard with `#pragma once`; emit `namespace x3d::nodes { class X3DNode; }` where the bare `class X3DNode;` forward decl is today; open `namespace x3d::core {` after the includes and close `} // namespace x3d::core` before EOF; change the `SFNode` typedef line to `using SFNode = std::shared_ptr<nodes::X3DNode>;` (drop the old `typedef std::shared_ptr<X3DNode> SFNode;`).

In `gen_enums_header()`: replace the `X3D_ENUMS_HPP` guard pair with `#pragma once`; wrap everything after the includes in `namespace x3d::core { … } // namespace x3d::core`.

In `emit/reflection.py` `gen_reflection_header()`: replace its guard with `#pragma once`; wrap the body in `namespace x3d::core { … }`; the existing `class X3DNode;` / `class NodeVisitor;` forward decls — keep `NodeVisitor` inside `x3d::core`, move `X3DNode` out to `namespace x3d::nodes { class X3DNode; }` above the core block.

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/test_core_namespace_emit.py -v`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/generator.py src/x3d_cpp_gen/emit/reflection.py tests/test_core_namespace_emit.py
git commit -m "gen(core): wrap X3Dtypes/X3Denums/X3DReflection in namespace x3d::core"
```

---

## Task 2: Generator emits node headers into `x3d/nodes/` under `namespace x3d::nodes`

**Files:**
- Modify: `src/x3d_cpp_gen/templates/class_template.hpp.jinja`
- Modify: `src/x3d_cpp_gen/templates/class_template.cpp.jinja`
- Modify: `src/x3d_cpp_gen/backends/cpp_header.py` (output subdir)
- Modify: `src/x3d_cpp_gen/cli.py` (set `namespace="x3d::nodes"`, build the subdirs)
- Test: `tests/test_nodes_namespace_emit.py` (new)

**Interfaces:**
- Consumes: `x3d/core/*.hpp` from Task 1.
- Produces: each node header at `<out>/x3d/nodes/<Name>.hpp`, `#pragma once`, including `"x3d/core/X3Dtypes.hpp"` etc. and `"x3d/nodes/<dep>.hpp"`, body inside `namespace x3d::nodes {` with `using namespace x3d::core;` as the first line inside that block.

- [ ] **Step 1: Write the failing test**

```python
# tests/test_nodes_namespace_emit.py
import subprocess, sys, pathlib

def test_generated_node_header_is_namespaced(tmp_path):
    out = tmp_path / "gen"
    subprocess.run([sys.executable, "-m", "x3d_cpp_gen.cli",
                    "--out", str(out)], check=True)
    appearance = (out / "x3d" / "nodes" / "Appearance.hpp").read_text()
    assert "#pragma once" in appearance
    assert '#include "x3d/core/X3Dtypes.hpp"' in appearance
    assert "namespace x3d::nodes {" in appearance
    assert "using namespace x3d::core;" in appearance
    # core header landed in the core subdir:
    assert (out / "x3d" / "core" / "X3Dtypes.hpp").exists()
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_nodes_namespace_emit.py -v`
Expected: FAIL — today node headers emit flat (`<out>/Appearance.hpp`), no namespace, bare includes.

- [ ] **Step 3: Implement — template + backend + cli**

`class_template.hpp.jinja`: replace lines 2-3 (`#ifndef…/#define…`) and the trailing `#endif` with `#pragma once`; change `#include "X3Dtypes.hpp"`/`"X3Denums.hpp"`/`"X3DReflection.hpp"` to `"x3d/core/…"`; change `#include "{{ dep }}.hpp"` to `#include "x3d/nodes/{{ dep }}.hpp"`; the `{% if namespace %}namespace {{ namespace }} {` block stays, and immediately inside it add `using namespace x3d::core;`.

`class_template.cpp.jinja`: same include rewrites; it already wraps the impl in `namespace {{ namespace }}`. Add `#include "x3d/nodes/<Name>.hpp"` via the existing own-header include (rewrite to the `x3d/nodes/` path).

`backends/cpp_header.py`: in `emit(...)`, write each node header/cpp into `os.path.join(output_dir, "x3d", "nodes")` (create it); keep filenames.

`cli.py`: after `out_dir.mkdir`, create `out_dir/"x3d"/"core"` and `out_dir/"x3d"/"nodes"`; pass the **core** dir to `write_types_header`/`write_enums_header`/`write_reflection_header`; pass `out_dir` (the backend appends `x3d/nodes`) to `generate_cpp_bindings`; set `namespace = "x3d::nodes"` unconditionally (remove the `--namespace`-driven value for the default path, or default the flag to `x3d::nodes`).

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/test_nodes_namespace_emit.py -v`
Expected: PASS

- [ ] **Step 5: Sanity-compile two generated headers standalone**

```bash
uv run x3d-cpp-gen --out /tmp/nsgen
printf '#include "x3d/nodes/Appearance.hpp"\nint main(){ x3d::nodes::Appearance a; (void)a; }\n' > /tmp/ns_smoke.cpp
g++ -std=c++20 -I/tmp/nsgen -c /tmp/ns_smoke.cpp -o /tmp/ns_smoke.o
```
Expected: compiles clean (proves the core/nodes seam + scoped using resolve).

- [ ] **Step 6: Commit**

```bash
git add src/x3d_cpp_gen/templates/class_template.hpp.jinja src/x3d_cpp_gen/templates/class_template.cpp.jinja src/x3d_cpp_gen/backends/cpp_header.py src/x3d_cpp_gen/cli.py tests/test_nodes_namespace_emit.py
git commit -m "gen(nodes): emit node headers under x3d/nodes in namespace x3d::nodes"
```

---

## Task 3: Factory, interface registry, and smoke test into `x3d::nodes`

**Files:**
- Modify: `src/x3d_cpp_gen/emit/factory.py`
- Modify: `src/x3d_cpp_gen/emit/registry.py`
- Modify: `src/x3d_cpp_gen/generator.py` (`write_node_factory`, `write_interface_registry` → nodes subdir; `generate_test_file` → nodes subdir)
- Modify: `src/x3d_cpp_gen/templates/test_template.cpp.jinja`
- Test: extend `tests/test_nodes_namespace_emit.py`

**Interfaces:**
- Produces: `<out>/x3d/nodes/{X3DNodeFactory,X3DInterfaceRegistry}.{hpp,cpp}` and `<out>/x3d/nodes/test.cpp`, all in `namespace x3d::nodes`. `createX3DNode` / `X3DNodeFactory` / the registry free functions are now `x3d::nodes::…`.

- [ ] **Step 1: Add failing assertions**

```python
def test_factory_and_registry_namespaced(tmp_path):
    out = tmp_path / "gen"
    import subprocess, sys
    subprocess.run([sys.executable, "-m", "x3d_cpp_gen.cli", "--out", str(out)], check=True)
    fac = (out / "x3d" / "nodes" / "X3DNodeFactory.hpp").read_text()
    assert "#pragma once" in fac
    assert "namespace x3d::nodes {" in fac
    assert "class X3DNode;" in fac  # forward-decl now lives inside x3d::nodes
```

- [ ] **Step 2: Run to verify it fails**

Run: `uv run pytest tests/test_nodes_namespace_emit.py::test_factory_and_registry_namespaced -v`
Expected: FAIL — factory emits global today.

- [ ] **Step 3: Implement**

`emit/factory.py`: `gen_node_factory_header` — `#pragma once` instead of `X3D_NODE_FACTORY_HPP`; after the `#include`s, open `namespace x3d::nodes {`, emit the `class X3DNode;` forward decl and `class X3DNodeFactory` inside it, close `} // namespace x3d::nodes`. `gen_node_factory_source` — rewrite the `#include "<Name>.hpp"` lines to `"x3d/nodes/<Name>.hpp"`, include `"x3d/nodes/X3DNodeFactory.hpp"`, wrap the registry definition in `namespace x3d::nodes { … }`.

`emit/registry.py`: same pattern — `#pragma once`, wrap the public types in `namespace x3d::nodes`, rewrite includes to `x3d/nodes/…`; the existing anonymous-namespace block (`namespace {`) stays nested inside.

`generator.py`: `write_node_factory`, `write_interface_registry`, `generate_test_file` write into `os.path.join(output_dir, "x3d", "nodes")`.

`test_template.cpp.jinja`: qualify instantiations — `x3d::nodes::{{ node.name }}`; include `"x3d/nodes/<Name>.hpp"`.

- [ ] **Step 4: Run to verify it passes**

Run: `uv run pytest tests/test_nodes_namespace_emit.py -v`
Expected: PASS (all cases)

- [ ] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/emit/factory.py src/x3d_cpp_gen/emit/registry.py src/x3d_cpp_gen/generator.py src/x3d_cpp_gen/templates/test_template.cpp.jinja tests/test_nodes_namespace_emit.py
git commit -m "gen(nodes): factory + interface registry + smoke test under x3d::nodes"
```

---

## Task 4: Regenerate the tree + rewire CMake layout/install

**Files:**
- Regenerate: `generated_cpp_bindings/**` (now `generated_cpp_bindings/x3d/core/**`, `generated_cpp_bindings/x3d/nodes/**`)
- Modify: `CMakeLists.txt`
- Modify: `scripts/check_golden.sh` if it hard-codes flat paths (verify)

**Interfaces:**
- Consumes: Tasks 1–3 emitter.
- Produces: the committed regenerated tree + a configuring build (consumers still red — expected until Tasks 7–12).

- [ ] **Step 1: Regenerate**

```bash
git rm -r --quiet generated_cpp_bindings
mise run gen
git add generated_cpp_bindings
```
Expected: new tree under `generated_cpp_bindings/x3d/core/` and `generated_cpp_bindings/x3d/nodes/`.

- [ ] **Step 2: Update CMake**

In `CMakeLists.txt`: keep `generated_cpp_bindings` on the include path (so `x3d/core/…`, `x3d/nodes/…` resolve). Change the node-source glob to `"${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings/x3d/nodes/*.cpp"`. Change the unity-build smoke-test exclusion path to `…/generated_cpp_bindings/x3d/nodes/test.cpp`. Change `install(DIRECTORY "…/generated_cpp_bindings/" …)` to `install(DIRECTORY "…/generated_cpp_bindings/x3d/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/x3d_cpp/x3d" FILES_MATCHING PATTERN "*.hpp")`.

- [ ] **Step 3: Verify configure + generated layer compiles**

Run: `cmake --preset dev && cmake --build build --target x3d_cpp_nodes`
Expected: `x3d_cpp_nodes` (the generated `.cpp`) compiles clean — proves the generated layer self-consistently builds in-tree. (Consumer targets are not built yet.)

- [ ] **Step 4: Verify golden gate sees only the binding move**

Run: `mise run golden`
Expected: reports drift (regenerated tree differs from a fresh temp regen only if non-deterministic — should be clean). If `check_golden.sh` diffs against the committed tree, it now passes since we committed the regen.

- [ ] **Step 5: Commit**

```bash
git add generated_cpp_bindings CMakeLists.txt
git commit -m "gen+build: regenerate bindings under x3d/{core,nodes}; rewire CMake layout & install"
```

---

## Task 5: Taxonomy lock test (red now, green after migration)

**Files:**
- Create: `runtime/tests/namespace_taxonomy_test.cpp`
- Modify: `CMakeLists.txt` (register `x3d_namespace_taxonomy` test)

**Interfaces:**
- Consumes: the regenerated tree.
- Produces: a ctest `x3d_namespace_taxonomy` pinning type placement.

- [ ] **Step 1: Write the test**

```cpp
// runtime/tests/namespace_taxonomy_test.cpp
#include "x3d/nodes/Transform.hpp"
#include "x3d/core/X3Dtypes.hpp"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("generated node classes live in x3d::nodes") {
    x3d::nodes::Transform t;
    CHECK(t.nodeTypeName() == "Transform");
}
TEST_CASE("value vocabulary lives in x3d::core") {
    x3d::core::SFVec3f v{1.f, 2.f, 3.f};
    x3d::core::SFNode n = nullptr;
    CHECK(n == nullptr);
    (void)v;
}
```

- [ ] **Step 2: Register + run to verify it fails to compile (red)**

Add to `CMakeLists.txt` near the other `add_executable`/`add_test` blocks:
```cmake
add_executable(x3d_namespace_taxonomy runtime/tests/namespace_taxonomy_test.cpp)
target_link_libraries(x3d_namespace_taxonomy PRIVATE x3d_cpp::sdk)
add_test(NAME x3d_namespace_taxonomy COMMAND x3d_namespace_taxonomy)
```
Run: `cmake --preset dev && cmake --build build --target x3d_namespace_taxonomy`
Expected: **builds** (this test already uses the new qualified names — it should pass even now since it only depends on the regenerated layer). If `doctest.h` include path differs, match the spelling used by a sibling test (grep an existing `_test.cpp`).

- [ ] **Step 3: Run the test**

Run: `ctest --test-dir build -R x3d_namespace_taxonomy --output-on-failure`
Expected: PASS

- [ ] **Step 4: Commit**

```bash
git add runtime/tests/namespace_taxonomy_test.cpp CMakeLists.txt
git commit -m "test: pin x3d::core / x3d::nodes type placement"
```

---

## Task 6: Migration helper script

**Files:**
- Create: `scripts/migrate_ns_includes.sh`

**Interfaces:**
- Produces: a script that rewrites bare generated-header `#include`s to the `x3d/core/…` and `x3d/nodes/…` spellings under a given path. Used by Tasks 7–12.

- [ ] **Step 1: Write the script**

```bash
#!/usr/bin/env bash
# Rewrite bare generated-header includes to the namespaced subdir spelling.
# Usage: scripts/migrate_ns_includes.sh <path> [<path>...]
set -euo pipefail
CORE='X3Dtypes X3Denums X3DReflection'
for root in "$@"; do
  # core headers
  for h in $CORE; do
    grep -rIl --include='*.hpp' --include='*.cpp' "#include \"$h.hpp\"" "$root" 2>/dev/null \
      | xargs -r sed -i "s|#include \"$h.hpp\"|#include \"x3d/core/$h.hpp\"|g"
  done
  # node headers: any include of a CapitalizedName.hpp that exists under x3d/nodes/
  while IFS= read -r f; do
    base=$(basename "$f" .hpp)
    grep -rIl --include='*.hpp' --include='*.cpp' "#include \"$base.hpp\"" "$root" 2>/dev/null \
      | xargs -r sed -i "s|#include \"$base.hpp\"|#include \"x3d/nodes/$base.hpp\"|g"
  done < <(find generated_cpp_bindings/x3d/nodes -maxdepth 1 -name '*.hpp')
done
echo "include rewrite complete for: $*"
```

- [ ] **Step 2: Verify it is a no-op on already-migrated generated tree**

Run: `bash scripts/migrate_ns_includes.sh generated_cpp_bindings && git diff --quiet generated_cpp_bindings && echo CLEAN`
Expected: `CLEAN` (generated tree already uses the new spellings).

- [ ] **Step 3: Commit**

```bash
git add scripts/migrate_ns_includes.sh && chmod +x scripts/migrate_ns_includes.sh
git commit -m "scripts: add namespaced-include migration helper"
```

---

## Tasks 7–11: Consumer migration by dependency layer

> Each task applies the **same recipe** to one layer. The layers are ordered bottom-up so each compiles once the layers beneath it are migrated. The recipe per file:
> 1. Run `scripts/migrate_ns_includes.sh <layer-dir>` to fix include paths.
> 2. For each **header** in the layer that references generated types: inside its existing `namespace x3d::runtime {` (or `x3d::codec`, `x3d::runtime::extract`, …) block, add as the first two lines `using namespace x3d::core;` and (if it names node types) `using namespace x3d::nodes;`. If a file has no enclosing namespace, wrap its generated-type usage or qualify explicitly.
> 3. For each **.cpp / test** TU: add file-scope `using namespace x3d::core;` and `using namespace x3d::nodes;` after the includes.
> 4. Build that layer's target(s) + run its tests.
> 5. Commit.

### Task 7: `runtime/` document model (depends only on generated)

**Files:** `runtime/X3DDocument.hpp`, `runtime/X3DProto.hpp`, `runtime/X3DProtoClone.hpp`, `runtime/InlineExpand.hpp`, `runtime/X3DRangeValidate.hpp`, and the other 5 top-level `runtime/*.hpp` flagged by the bare-include grep.

- [ ] **Step 1: Rewrite includes**

Run: `bash scripts/migrate_ns_includes.sh runtime` *(top-level only in this task; subdirs handled later — restrict with the file list below)*

For precision, restrict to this layer:
```bash
git ls-files 'runtime/*.hpp' 'runtime/*.cpp' | xargs -r bash -c 'scripts/migrate_ns_includes.sh "$@"' _
```

- [ ] **Step 2: Add scoped using-directives**

In each modified `runtime/*.hpp`, locate the `namespace x3d::runtime {` line and insert directly after it:
```cpp
  using namespace x3d::core;
  using namespace x3d::nodes;
```

- [ ] **Step 3: Build the document-model headers**

Run: `cmake --build build --target x3d_cpp_nodes && cmake --build build 2>&1 | grep -i "runtime/X3DDocument\|error:" | head`
Expected: no errors originating in `runtime/*.hpp`.

- [ ] **Step 4: Verify no bare generated includes remain in this layer**

Run: `git ls-files 'runtime/*.hpp' 'runtime/*.cpp' | xargs grep -l '#include "X3D\(types\|enums\|Reflection\|Node\)\.hpp"' || echo CLEAN`
Expected: `CLEAN`

- [ ] **Step 5: Commit**

```bash
git add runtime/*.hpp runtime/*.cpp
git commit -m "migrate(runtime-core): x3d::core/x3d::nodes qualification + include paths"
```

### Task 8: `runtime/codecs/` (depends on document model)

**Files:** `runtime/codecs/*.hpp`/`*.cpp` flagged by the grep (`ProtoNameMaps.hpp`, `FieldValueIO.hpp`, …).

- [ ] **Step 1–5:** Apply the recipe to `runtime/codecs` (its files open `namespace x3d::codec {`). Build target check: `cmake --build build 2>&1 | grep -i "runtime/codecs.*error:" | head` → none. Verify-clean grep on `runtime/codecs`. Commit `migrate(codecs): …`.

### Task 9: `runtime/extract`, `events`, `scene`, `script`, `sound`, `physics`, `parse`, `math`, `ext` (non-test)

- [ ] **Step 1–5:** Apply the recipe to each subsystem dir's non-test `*.hpp`/`*.cpp`. These open `namespace x3d::runtime` / `x3d::runtime::extract`. After each, build and grep-clean. Commit per subsystem (e.g. `migrate(extract): …`) — these are independent enough to split or batch as one `migrate(runtime-subsystems): …` commit.

### Task 10: `include/x3d/sdk.hpp` façade

**Files:** `include/x3d/sdk.hpp`

- [ ] **Step 1: Rewrite includes + add namespace aliases**

Run `scripts/migrate_ns_includes.sh include`. Then inside `namespace x3d::sdk {` add:
```cpp
namespace core  = x3d::core;
namespace nodes = x3d::nodes;
```

- [ ] **Step 2: Build the façade test**

Run: `cmake --build build --target x3d_sdk_facade_test 2>&1 | grep error: | head` (grep the real target name from CMake) → none.

- [ ] **Step 3: Commit** `migrate(sdk): namespaced includes + x3d::sdk::{core,nodes} aliases`

### Task 11: `tools/`, `examples/` (non-test)

- [ ] **Step 1–5:** Apply the recipe to `tools/x3d-cli/*.cpp` and `examples/cpu_raster/**` (non-test). These are mostly `.cpp` → file-scope using-directives. Build `x3d-cli` and the cpuraster example targets; grep-clean; commit `migrate(tools+examples): …`.

---

## Task 12: Migrate all test TUs + full green bar

**Files:** every `**/tests/*.cpp` flagged by the grep (~74 files across `runtime/*/tests`, `tools`, `examples`).

**Interfaces:**
- Consumes: all migrated layers.
- Produces: a fully green `mise run ci`.

- [ ] **Step 1: Rewrite test includes**

```bash
git ls-files '*/tests/*.cpp' | xargs -r bash -c 'scripts/migrate_ns_includes.sh "$@"' _
```

- [ ] **Step 2: Add file-scope using-directives to each test TU**

For each test `.cpp` that names generated types, insert after the last `#include`:
```cpp
using namespace x3d::core;
using namespace x3d::nodes;
```
(Test TUs are standalone `main()`s, so file-scope using-directives are safe.)

- [ ] **Step 3: Build everything (dev preset)**

Run: `cmake --build build 2>&1 | grep -i error: | head -40`
Expected: no errors. Fix any straggler unqualified references (ambiguities surface here — e.g. a test that used a bare node name now needs the using-directive or an explicit `x3d::nodes::` prefix).

- [ ] **Step 4: Run the full local CI**

Run: `mise run ci`
Expected: PASS — pytest (incl. golden drift), golden gate, conformance-gate, coverage-gate, **build-ci with per-header isolation ON** (independently proves every relocated header self-contains), cli-gate-regression. The per-header check is the real proof the new layout is airtight.

- [ ] **Step 5: Run the sanitizer suite**

Run: `mise run build-san`
Expected: 100% ctests pass under ASan/UBSan (no behavioral change expected; this is a rename).

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "migrate(tests): qualify generated types across the test suite; full ci green"
```

---

## Task 13: Docs (ADR + subsystem + coverage + nav)

**Files:**
- Create: `docs/wiki/decisions/0039-generated-binding-namespaces.md`
- Modify: `docs/wiki/subsystems/generated-bindings.md`
- Modify: `docs/wiki/coverage.md`
- Modify: `mkdocs.yml`

**Interfaces:**
- Produces: docs that match the shipped code; strict docs gate green.

- [ ] **Step 1: Write ADR-0039**

Frontmatter (title/summary/tags/updated/related) per the ADR-0038 template. Body: Status (Accepted 2026-06-27); Context (685 types + macros in global namespace, the lone layer breaking the `x3d::*` convention; API-2 entanglement); Decision (`x3d::core` vocabulary / `x3d::nodes` classes split, clean break no shim, `#pragma once`, `x3d/{core,nodes}/` layout, scoped using-directives never global, `SFNode = shared_ptr<x3d::nodes::X3DNode>` seam, façade `x3d::sdk::{core,nodes}` aliases); Consequences (clean global scope; external embedders must qualify — documented; binding-hash golden re-blessed, behavior unchanged); Related (links to generated-bindings.md, sdk-facade.md, ADR-0038).

- [ ] **Step 2: Update subsystem + coverage + nav**

`generated-bindings.md`: document the namespace taxonomy and `x3d/core/` + `x3d/nodes/` header paths + the `namespace_taxonomy_test`. `coverage.md`: add the ADR-0039 row, bump the Decisions count, extend the narrative. `mkdocs.yml`: nav entry for ADR-0039 after ADR-0038.

- [ ] **Step 3: Drift + strict build**

Run: `mise run docs-drift working && mise run docs-build`
Expected: drift advisory reviewed; `docs-build --strict` PASS (no dead links / nav orphans).

- [ ] **Step 4: Commit**

```bash
git add docs/wiki/decisions/0039-generated-binding-namespaces.md docs/wiki/subsystems/generated-bindings.md docs/wiki/coverage.md mkdocs.yml
git commit -m "docs: ADR-0039 generated-binding namespaces + subsystem/coverage/nav"
```

---

## Task 14: Refresh RAG stores + final verification

- [ ] **Step 1: Re-ingest moved symbols**

Run: `mise run code-ingest && mise run docs-ingest`
Expected: completes (search/drift stay accurate after the symbol move).

- [ ] **Step 2: Final full gate**

Run: `mise run ci && mise run docs-build`
Expected: both PASS.

- [ ] **Step 3: Open the PR**

```bash
git push -u origin api/namespace-generated-bindings
gh pr create --title "api: namespace generated bindings — x3d::core + x3d::nodes (API-1+API-2)" --body "<summary of the change, links to ADR-0039 and the spec; note the clean break for embedders>"
```

---

## Self-Review

**Spec coverage:** API-1 (namespace) → Tasks 1–3 (emitter) + 4 (regen) + 7–12 (consumers). API-2 (header layout/install) → Task 2/3 subdirs + Task 4 CMake install. `x3d::core`+`x3d::nodes` taxonomy → Tasks 1–3. Clean break / no shim → Global Constraints + recipe (no shim header anywhere). `SFNode`/`X3DNode` seam → Task 1. Façade aliases → Task 10. Lock test → Task 5. Verification (full ci, per-header isolation, golden re-bless, sanitizers) → Tasks 4, 12. Docs/ADR → Task 13. RAG refresh → Task 14. No spec requirement is unmapped.

**Placeholder scan:** PR body in Task 14 is the one intentional fill-at-time item (depends on final diff). All emitter/CMake/test steps carry concrete code or exact commands.

**Type consistency:** `x3d::core::{SFVec3f,SFNode,FieldTable,NodeVisitor}`, `x3d::nodes::{X3DNode,Transform,Appearance,X3DNodeFactory,createX3DNode}`, `using SFNode = std::shared_ptr<nodes::X3DNode>` used consistently across Tasks 1, 2, 3, 5, 10.

**Known risk carried forward:** the spec-deviation banner (scoped using-directives vs literal full-qualification) must be confirmed before Task 2. The per-layer migration assumes each consumer file opens a single enclosing `x3d::*` namespace; files that don't (or that re-open multiple) need the explicit-qualify fallback noted in the recipe — the Task-12 dev build surfaces these.

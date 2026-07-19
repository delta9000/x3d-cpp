# SAI Sister-Repo Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the unmerged `sai/semantic-kernel` branch into a standalone `x3d-sai` repository with a self-contained generator and zero dependency on x3d-cpp, and leave x3d-cpp an archive tag, an ADR, and the one runtime check that moves back.

**Architecture:** `git filter-repo` rewrites the 40-commit branch into the new repo's layout, preserving history. Three seed commits then add what could not be filtered: build system, vendored parse layer, generator wiring. The generator becomes `x3d_sai_gen`, reading a vendored copy of the Web3D UOM XML rather than importing `x3d_cpp_gen`. A separate small PR lands in x3d-cpp.

**Tech Stack:** C++20, CMake, doctest 2.4.11, tl::expected 1.3.1, Python 3 with uv, mise, PyYAML, git-filter-repo.

## Global Constraints

- Source branch head is `8e2a000` on `sai/semantic-kernel`. x3d-cpp `main` is `3ce7a85`.
- `main` contains none of this work. Nothing is removed from x3d-cpp; only added.
- `x3d-sai` must have no build-time, link-time, or Python dependency on x3d-cpp.
- Namespace stays `x3d::sai::experimental`. Do not rename it.
- Vendored UOM SHA-256 must be `0f1d9ede593b159469936470c7404629b27b4eb8ccbfef3d07dbb1f27df6443f`.
- These paths keep their exact names in `x3d-sai`, because Python path constants and
  C++ includes depend on them: `scripts/`, `docs/conformance/`, `docs/plans/`,
  `tests/`, and `generated_cpp_bindings/x3d/...`.
- Do not put `Claude-Session:` trailers or `claude.ai/code/session_…` URLs in any commit
  message or PR body in either repo.
- Describe third-party projects and standards factually. No disparagement in committed
  artifacts.
- `git-filter-repo` is already installed at `/usr/bin/git-filter-repo`.

---

## File Structure

**New repo `x3d-sai` (sibling of `x3d-cpp`, e.g. `/home/ben/code/x3d-sai`):**

| Path | Responsibility |
| --- | --- |
| `include/x3d/sai/experimental/kernel.hpp` | the semantic kernel interface |
| `include/x3d/sai/experimental/metadata.hpp` | catalog bridge interface |
| `src/kernel.cpp`, `src/metadata.cpp` | implementations |
| `generated_cpp_bindings/x3d/sai/experimental/` | 338 generated binding headers + catalog |
| `generated_cpp_bindings/x3d/nodes/X3DSemanticMetadataRegistry.{hpp,cpp}` | generated UOM catalog |
| `src/x3d_sai_gen/` | self-contained generator package |
| `src/x3d_sai_gen/data/X3dUnifiedObjectModel-4.0.xml` | vendored Web3D UOM |
| `tests/semantic_kernel_test.cpp` | 91 doctest cases |
| `tests/test_sai_bindings.py`, `tests/test_sai_conformance.py` | generator tests |
| `examples/author_inspect.cpp`, `examples/generated_author_inspect.cpp` | runnable examples |
| `third_party/tl/` | tl::expected 1.3.1 |
| `third_party/doctest/` | doctest.h + doctest_main.cpp |
| `scripts/sai_conformance.py`, `scripts/check_sai_services.py`, `scripts/check_sai_invariants.py` | conformance tooling |
| `docs/conformance/` | sai-services, sai-invariants, sai-service-catalog, SAI-BASELINE.md |
| `docs/plans/` | the 14 SAI design and implementation docs |
| `CMakeLists.txt`, `mise.toml`, `pyproject.toml`, `NOTICE`, `LICENSE`, `README.md` | root |

**Modified in `x3d-cpp` (branch `docs/sai-sister-repo-split`, already created):**

| Path | Change |
| --- | --- |
| `docs/wiki/decisions/NNNN-sai-sister-repo-split.md` | new ADR |
| `docs/wiki/subsystems/sai-semantic-kernel.md` | pointer page |
| `docs/wiki/coverage.md` | one row |
| `mkdocs.yml` | one nav entry |
| `runtime/scene/tests/node_reflection_uom_parity_test.cpp` | re-homed runtime check |
| `CMakeLists.txt` | register that test |

---

## Task 1: Import branch history into x3d-sai

**Files:**
- Create: `/home/ben/code/x3d-sai/` (whole repo)

**Interfaces:**
- Produces: a git repo whose history is the 40 SAI commits, paths already renamed.

- [ ] **Step 1: Make a scratch clone of the branch**

```bash
cd /tmp/claude-1000/-home-ben-code-x3d-cpp/*/scratchpad
git clone --no-local --single-branch --branch sai/semantic-kernel \
  /home/ben/code/x3d-cpp x3d-sai-import
cd x3d-sai-import
git log --oneline | wc -l   # expect 40 SAI commits plus main's history
```

- [ ] **Step 2: Write the filter-repo path spec**

Create `/tmp/.../scratchpad/sai-paths.txt`:

```
experimental/sai/
generated_cpp_bindings/x3d/sai/
generated_cpp_bindings/x3d/nodes/X3DSemanticMetadataRegistry.hpp
generated_cpp_bindings/x3d/nodes/X3DSemanticMetadataRegistry.cpp
src/x3d_cpp_gen/emit/sai_bindings.py
src/x3d_cpp_gen/emit/semantic_metadata.py
scripts/sai_conformance.py
scripts/check_sai_services.py
scripts/check_sai_invariants.py
docs/conformance/sai-services.yaml
docs/conformance/sai-invariants.yaml
docs/conformance/sai-service-catalog.yaml
docs/conformance/SAI-BASELINE.md
tests/test_sai_bindings.py
tests/test_sai_conformance.py
tests/sai_invariants/
```

- [ ] **Step 3: Run filter-repo with paths and renames**

```bash
git filter-repo --force \
  --paths-from-file ../sai-paths.txt \
  --path-rename experimental/sai/include/:include/ \
  --path-rename experimental/sai/src/:src/ \
  --path-rename experimental/sai/tests/:tests/ \
  --path-rename experimental/sai/examples/:examples/ \
  --path-rename experimental/sai/third_party/:third_party/ \
  --path-rename experimental/sai/README.md:README.md \
  --path-rename src/x3d_cpp_gen/emit/:src/x3d_sai_gen/emit/
```

- [ ] **Step 4: Verify the result**

```bash
git log --oneline | wc -l          # expect ~40, no SDK commits
find . -path ./.git -prune -o -type f -print | wc -l   # expect ~360
ls include/x3d/sai/experimental/    # kernel.hpp metadata.hpp
ls src/                             # kernel.cpp metadata.cpp x3d_sai_gen/
test ! -e experimental && echo "flatten OK"
git log --oneline --all -- runtime/ | wc -l   # expect 0
```

Expected: no `runtime/`, no SDK generated bindings, no `CMakeLists.txt` yet.

- [ ] **Step 5: Move into place and commit nothing yet**

```bash
mv /tmp/.../x3d-sai-import /home/ben/code/x3d-sai
cd /home/ben/code/x3d-sai
git remote remove origin
git status --short   # expect clean
```

---

## Task 2: Seed the build system and vendored test support

**Files:**
- Create: `CMakeLists.txt`, `LICENSE`, `NOTICE`, `mise.toml`
- Create: `third_party/doctest/doctest.h`, `third_party/doctest/doctest_main.cpp`
- Modify: `README.md`

**Interfaces:**
- Consumes: the imported tree from Task 1.
- Produces: CMake targets `x3d_sai_experimental`, `x3d_sai_experimental_metadata`,
  `x3d_sai_doctest_main`, `x3d_sai_experimental_tests`, `x3d_sai_experimental_example`,
  `x3d_sai_experimental_generated_example`, and all `add_test(NAME sai_*)` entries.

- [ ] **Step 1: Vendor doctest**

```bash
cd /home/ben/code/x3d-sai
mkdir -p third_party/doctest
cp /home/ben/code/x3d-cpp/runtime/test_support/doctest/doctest.h third_party/doctest/
cp /home/ben/code/x3d-cpp/runtime/test_support/doctest_main.cpp third_party/doctest/
```

- [ ] **Step 2: Write the root CMakeLists.txt**

Port the target definitions from `x3d-cpp` branch `CMakeLists.txt:1718-1956`, dropping
`x3d_cpp_headers`, `x3d_cpp::nodes`, and `x3d_doctest_main`. Keep every
`add_test(NAME ...)` name unchanged, because `scripts/sai_conformance.py:548`
cross-links registered services against literal CTest names parsed out of this file.

```cmake
cmake_minimum_required(VERSION 3.24)
project(x3d_sai VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(_sai_generated "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings")

# Firewalled modern-C++ SAI proposal. Links only the C++ standard library.
add_library(x3d_sai_experimental STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/kernel.cpp")
target_include_directories(x3d_sai_experimental PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_compile_features(x3d_sai_experimental PUBLIC cxx_std_20)

# Optional generated-schema bridge over the UOM catalog.
add_library(x3d_sai_experimental_metadata STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/metadata.cpp"
    "${_sai_generated}/x3d/nodes/X3DSemanticMetadataRegistry.cpp")
target_include_directories(x3d_sai_experimental_metadata PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${_sai_generated}")
target_link_libraries(x3d_sai_experimental_metadata PUBLIC x3d_sai_experimental)
target_compile_features(x3d_sai_experimental_metadata PUBLIC cxx_std_20)

add_library(x3d_sai_doctest_main STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/doctest/doctest_main.cpp")
target_include_directories(x3d_sai_doctest_main PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_compile_features(x3d_sai_doctest_main PUBLIC cxx_std_20)

add_executable(x3d_sai_experimental_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_kernel_test.cpp")
target_link_libraries(x3d_sai_experimental_tests PRIVATE
    x3d_sai_experimental_metadata x3d_sai_doctest_main)
target_compile_features(x3d_sai_experimental_tests PRIVATE cxx_std_20)

add_test(NAME x3d_sai_experimental COMMAND x3d_sai_experimental_tests)
```

Then append every `add_test(NAME sai_... COMMAND x3d_sai_experimental_tests
"--test-case=...")` line verbatim from the branch file, and both example targets:

```cmake
add_executable(x3d_sai_experimental_example
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/author_inspect.cpp")
target_link_libraries(x3d_sai_experimental_example PRIVATE x3d_sai_experimental)
target_compile_features(x3d_sai_experimental_example PRIVATE cxx_std_20)
add_test(NAME x3d_sai_experimental_example COMMAND x3d_sai_experimental_example)

add_executable(x3d_sai_experimental_generated_example
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/generated_author_inspect.cpp")
target_link_libraries(x3d_sai_experimental_generated_example PRIVATE
    x3d_sai_experimental_metadata)
target_compile_features(x3d_sai_experimental_generated_example PRIVATE cxx_std_20)
add_test(NAME x3d_sai_experimental_generated_example
    COMMAND x3d_sai_experimental_generated_example)
```

- [ ] **Step 3: Write NOTICE**

```
x3d-sai
Copyright (c) 2026 Ben Sandbrook

This product includes third-party components:

  Component     Path                    License
  tl::expected  third_party/tl/         CC0-1.0 (v1.3.1)
                (C++20 expected/monadic compatibility for the experimental SAI;
                 see third_party/tl/COPYING)
  doctest       third_party/doctest/    MIT (test-only; v2.4.11)

  X3D Unified Object Model (X3dUnifiedObjectModel-4.0.xml) is published by the
  Web3D Consortium at https://www.web3d.org/specifications/ and is vendored under
  src/x3d_sai_gen/data/ as generator input.
```

- [ ] **Step 4: Copy LICENSE from x3d-cpp and write mise.toml**

```bash
cp /home/ben/code/x3d-cpp/LICENSE .
```

```toml
[tools]
python = "3.12"

[tasks.build]
description = "Configure and build the experimental SAI kernel."
run = "cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build -j"

[tasks.ctest]
description = "Run the SAI falsification suite."
run = "ctest --test-dir build --output-on-failure"

[tasks.sai-baseline]
description = "Regenerate the SAI service/invariant baseline from the editable YAML registers."
run = "uv run python scripts/sai_conformance.py generate"

[tasks.sai-conformance-gate]
description = "Validate the SAI service and invariant registers, their cross-links, and generated baseline drift (runs in CI)."
run = "uv run python scripts/check_sai_services.py"

[tasks.sai-invariants]
description = "Validate the semantic invariant registry, service cross-links, and generated coverage view."
run = "uv run python scripts/check_sai_invariants.py"

[tasks.sai-conformance]
description = "Opt-in convergence gate: require every registered SAI service and falsification test to be complete. Expected to fail until the SAI is complete."
run = "uv run python scripts/check_sai_services.py --strict"
```

- [ ] **Step 5: Prepend the provenance and history note to README.md**

Insert above the existing `# Experimental C++ SAI semantic kernel` heading a short
section stating: the repo was split from x3d-cpp on 2026-07-19; history before the
seed commits is provenance and is not bisectable, because the imported commits predate
this build system; the endgame is absorption into x3d-cpp's authoring API when
`mise run sai-conformance` goes green.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt LICENSE NOTICE mise.toml README.md third_party/doctest
git commit -m "build: root build system, vendored doctest, and provenance notes

Ports the SAI target definitions from the x3d-cpp branch as a root project.
CTest names are preserved verbatim because the conformance gate cross-links
registered services against literal add_test names in this file."
```

Note: the build does not compile yet. Task 3 removes the last x3d-cpp include.

---

## Task 3: Cut the test's dependency on x3d-cpp runtime nodes

**Files:**
- Modify: `tests/semantic_kernel_test.cpp` (remove includes at lines 12-13, remove the
  live-node block at lines 594-610)

**Interfaces:**
- Consumes: CMake targets from Task 2.
- Produces: a test binary that links only `x3d_sai_experimental_metadata` and
  `x3d_sai_doctest_main`.

- [ ] **Step 1: Confirm the build fails for exactly the expected reason**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build -j
```

Expected: FAIL with `fatal error: x3d/nodes/X3DNode.hpp: No such file or directory`.

- [ ] **Step 2: Remove the two runtime includes**

Delete these two lines from `tests/semantic_kernel_test.cpp`:

```cpp
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
```

Keep `#include "x3d/nodes/X3DSemanticMetadataRegistry.hpp"`. That header is generated
and ships in this repo.

- [ ] **Step 3: Remove the live-node parity block**

In the test case `"generated metadata adapter has exhaustive ordered descriptor parity"`,
delete the whole conditional (branch lines 594-610):

```cpp
    if (!source_node.abstract) {
      const auto live = x3d::nodes::X3DNodeFactory::create(source_node.name);
      REQUIRE(live);
      const auto &reflected = live->fields();
      REQUIRE(reflected.size() == source_node.fields.size());
      for (std::size_t field_index = 0; field_index < reflected.size();
           ++field_index) {
        CHECK(reflected[field_index].x3dName ==
              source_node.fields[field_index].name);
        CHECK(reflected[field_index].type ==
              source_node.fields[field_index].type);
        CHECK(reflected[field_index].access ==
              source_node.fields[field_index].access);
      }
    }
```

Leave every catalog-to-adapter assertion above it untouched. Those are the SAI claim.
Add a comment in its place:

```cpp
    // Live x3d-cpp runtime reflection parity moved to x3d-cpp with the
    // 2026-07-19 split; it asserted an SDK property, not a SAI one.
```

- [ ] **Step 4: Build and run the full suite**

```bash
cmake --build build -j && ctest --test-dir build --output-on-failure
```

Expected: PASS. 91 test cases, and every `sai_*` named test green, including
`sai_generated_metadata_drift`.

- [ ] **Step 5: Verify the firewall holds**

```bash
grep -rn 'x3d/nodes/X3DNode' include src tests examples   # expect only Registry hits
ldd build/x3d_sai_experimental_tests | grep -i x3d        # expect no x3d-cpp libs
grep -rn 'x3d_cpp' CMakeLists.txt                          # expect no output
```

- [ ] **Step 6: Commit**

```bash
git add tests/semantic_kernel_test.cpp
git commit -m "test(sai): drop live runtime-node parity, completing the firewall

The block asserted that x3d-cpp's X3DNodeFactory reflection matches the
semantic catalog. That is an SDK property; it moves to x3d-cpp. Catalog-to-
adapter parity, the SAI claim, is unchanged."
```

---

## Task 4: Stand up the self-contained generator

**Files:**
- Create: `src/x3d_sai_gen/__init__.py`, `parser.py`, `model/{__init__,types,enums,version}.py`,
  `emit/{__init__,naming,semantic_fields,registry}.py`, `cli.py`
- Create: `src/x3d_sai_gen/data/X3dUnifiedObjectModel-4.0.xml`
- Create: `pyproject.toml`
- Modify: `src/x3d_sai_gen/emit/sai_bindings.py`, `src/x3d_sai_gen/emit/semantic_metadata.py`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces: `x3d_sai_gen.cli:main`, invoked as `mise run gen`, writing into
  `generated_cpp_bindings/`. Module names mirror the x3d-cpp originals so the moved
  emitters need only their import prefix changed.

- [ ] **Step 1: Vendor the UOM and pin its hash**

```bash
mkdir -p src/x3d_sai_gen/data
cp /home/ben/code/x3d-cpp/src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml \
   src/x3d_sai_gen/data/
sha256sum src/x3d_sai_gen/data/X3dUnifiedObjectModel-4.0.xml \
  | cut -d' ' -f1 > src/x3d_sai_gen/data/UOM-4.0.sha256
cat src/x3d_sai_gen/data/UOM-4.0.sha256
```

Expected: `0f1d9ede593b159469936470c7404629b27b4eb8ccbfef3d07dbb1f27df6443f`

- [ ] **Step 2: Copy the parse and model substrate**

```bash
cd /home/ben/code/x3d-sai
mkdir -p src/x3d_sai_gen/model src/x3d_sai_gen/emit
S=/home/ben/code/x3d-cpp/src/x3d_cpp_gen
cp $S/parser.py src/x3d_sai_gen/
cp $S/model/{__init__.py,types.py,enums.py,version.py} src/x3d_sai_gen/model/
cp $S/emit/{__init__.py,naming.py,semantic_fields.py,registry.py} src/x3d_sai_gen/emit/
```

Trim `emit/naming.py` to `sanitize_field_name`, `pascal`, `cpp_str`; trim
`emit/registry.py` to `interface_ids` and `interfaces_of`, deleting
`gen_interface_registry_header` and `gen_interface_registry_source`, which emit an SDK
artifact this repo does not build.

- [ ] **Step 3: Copy the two data tables the binding test imports**

`tests/test_sai_bindings.py:8` imports `FIELD_TYPE_MAPPING` and `XS_TYPES` from
`x3d_cpp_gen.generator`. Copy just those two module-level dicts from
`/home/ben/code/x3d-cpp/src/x3d_cpp_gen/generator.py` into a new
`src/x3d_sai_gen/fieldtypes.py`. Do not copy the rest of `generator.py`; it emits SDK
bindings.

- [ ] **Step 4: Rewrite imports across the package**

```bash
grep -rl 'x3d_cpp_gen' src/x3d_sai_gen tests \
  | xargs sed -i 's/x3d_cpp_gen/x3d_sai_gen/g'
sed -i 's/from x3d_sai_gen.generator import/from x3d_sai_gen.fieldtypes import/' \
  tests/test_sai_bindings.py
grep -rn 'x3d_cpp_gen' src tests   # expect no output
```

- [ ] **Step 5: Write the CLI**

`src/x3d_sai_gen/cli.py`:

```python
"""Generate the experimental SAI bindings and semantic catalog from the UOM."""

import argparse
import hashlib
import os
from importlib.resources import files

from x3d_sai_gen.parser import (
    parse_x3d_model, build_dependency_graph, parse_enum_definitions,
)
from x3d_sai_gen.emit.sai_bindings import (
    gen_sai_binding_index, gen_sai_bindings_catalog, gen_sai_node_binding,
)
from x3d_sai_gen.emit.semantic_metadata import (
    gen_semantic_metadata_header, gen_semantic_metadata_source,
)

SPEC_VERSION = "4.0"


def _uom_path():
    return files("x3d_sai_gen.data") / f"X3dUnifiedObjectModel-{SPEC_VERSION}.xml"


def _expected_sha():
    return (files("x3d_sai_gen.data") / f"UOM-{SPEC_VERSION}.sha256") \
        .read_text().strip()


def verify_uom() -> str:
    """Return the vendored UOM's SHA-256, raising if it drifted from the pin."""
    data = _uom_path().read_bytes()
    actual = hashlib.sha256(data).hexdigest()
    expected = _expected_sha()
    if actual != expected:
        raise SystemExit(
            f"vendored UOM hash {actual} does not match pin {expected}")
    return actual


def main(argv=None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", default="generated_cpp_bindings")
    parser.add_argument("--check-uom-only", action="store_true")
    args = parser.parse_args(argv)

    verify_uom()
    if args.check_uom_only:
        return 0

    nodes = parse_x3d_model(str(_uom_path()))
    graph = build_dependency_graph(nodes)
    enum_defs = parse_enum_definitions(str(_uom_path()))

    nodes_dir = os.path.join(args.out_dir, "x3d", "nodes")
    os.makedirs(nodes_dir, exist_ok=True)
    with open(os.path.join(nodes_dir, "X3DSemanticMetadataRegistry.hpp"), "w") as f:
        f.write(gen_semantic_metadata_header())
    with open(os.path.join(nodes_dir, "X3DSemanticMetadataRegistry.cpp"), "w") as f:
        f.write(gen_semantic_metadata_source(nodes, graph, enum_defs, SPEC_VERSION))

    root = os.path.join(args.out_dir, "x3d", "sai", "experimental")
    bindings_dir = os.path.join(root, "bindings")
    os.makedirs(bindings_dir, exist_ok=True)
    for name, node in nodes.items():
        if node.abstract:
            continue
        with open(os.path.join(bindings_dir, f"{name}.hpp"), "w") as f:
            f.write(gen_sai_node_binding(node, nodes, graph, enum_defs))
    with open(os.path.join(root, "X3DSAIBindings.hpp"), "w") as f:
        f.write(gen_sai_binding_index(nodes))
    with open(os.path.join(root, "X3DSAIBindingCatalog.hpp"), "w") as f:
        f.write(gen_sai_bindings_catalog(nodes, graph, enum_defs, SPEC_VERSION))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

Reconcile the exact call signatures against the moved
`src/x3d_sai_gen/emit/sai_bindings.py` and `semantic_metadata.py`; on the x3d-cpp branch
these were invoked from `generator.py:write_sai_bindings` and
`write_semantic_metadata_registry`, which are the reference for argument order.

- [ ] **Step 6: Write pyproject.toml**

```toml
[project]
name = "x3d-sai-gen"
version = "0.1.0"
description = "Generator for the experimental C++ X3D SAI semantic kernel"
requires-python = ">=3.11"
authors = [{ name = "Ben Sandbrook", email = "sandbrookvt@gmail.com" }]
dependencies = ["pyyaml>=6.0"]

[project.scripts]
x3d-sai-gen = "x3d_sai_gen.cli:main"

[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[tool.hatch.build.targets.wheel]
packages = ["src/x3d_sai_gen"]

[tool.pytest.ini_options]
testpaths = ["tests"]
```

- [ ] **Step 7: Add the gen and determinism tasks to mise.toml**

```toml
[tasks.gen]
description = "Regenerate the SAI bindings and semantic catalog from the vendored UOM."
run = "uv run python -m x3d_sai_gen.cli --out-dir generated_cpp_bindings"

[tasks.gen-determinism]
description = "Fail if regenerating the committed bindings produces any diff."
run = "uv run python -m x3d_sai_gen.cli --out-dir generated_cpp_bindings && git diff --exit-code -- generated_cpp_bindings"

[tasks.uom-pin]
description = "Fail if the vendored UOM drifted from its recorded SHA-256."
run = "uv run python -m x3d_sai_gen.cli --check-uom-only"
```

- [ ] **Step 8: Prove regeneration is byte-identical to the committed tree**

```bash
uv sync
mise run uom-pin        # expect exit 0
mise run gen-determinism
```

Expected: PASS with no diff. A diff here means the port changed emitter behaviour and
must be fixed before proceeding, since the 338 committed headers are the artifact the
C++ tests compile against.

- [ ] **Step 9: Run the Python tests**

```bash
uv run pytest tests/ -v
```

Expected: `test_sai_bindings.py` and `test_sai_conformance.py` PASS.

- [ ] **Step 10: Commit**

```bash
git add src/x3d_sai_gen pyproject.toml mise.toml tests/
git commit -m "gen: self-contained SAI generator over a vendored, SHA-pinned UOM

The X3D Unified Object Model is published by the Web3D Consortium, so this
repo consumes it directly rather than depending on x3d-cpp's generator. The
parse and model layer is copied; the two SAI emitters are moved. Regeneration
is byte-identical to the committed bindings."
```

---

## Task 5: Verify the conformance tooling runs unchanged

**Files:**
- Verify only: `scripts/sai_conformance.py`, `scripts/check_sai_services.py`,
  `scripts/check_sai_invariants.py`, `docs/conformance/*`

**Interfaces:**
- Consumes: root `CMakeLists.txt` from Task 2, whose `add_test(NAME ...)` entries
  `scripts/sai_conformance.py:548` parses.

- [ ] **Step 1: Run the blocking gate**

```bash
mise run sai-conformance-gate
```

Expected: PASS. `_repo_root()` resolves to the repo root from `scripts/`, and
`docs/conformance/` kept its path, so no code edit should be needed. If it reports
unknown CTest names, a `add_test(NAME ...)` line was dropped in Task 2; restore it
rather than editing the register.

- [ ] **Step 2: Run the invariants gate**

```bash
mise run sai-invariants
```

Expected: PASS.

- [ ] **Step 3: Confirm the convergence gate still fails, and says why**

```bash
mise run sai-conformance; echo "exit=$?"
```

Expected: non-zero exit listing incomplete services. This is the designed state and is
the re-entry trigger. A green result here would mean the register lost rows in the move.

- [ ] **Step 4: Confirm the baseline does not drift**

```bash
mise run sai-baseline && git diff --exit-code -- docs/conformance/SAI-BASELINE.md
```

Expected: no diff.

- [ ] **Step 5: Commit any path fixes, or record that none were needed**

```bash
git add -A && git commit -m "conformance: verify the SAI registers and gates in the split repo" \
  || echo "no changes required"
```

---

## Task 6: Add CI

**Files:**
- Create: `.github/workflows/ci.yml`
- Modify: `mise.toml` (aggregate `ci` task)

- [ ] **Step 1: Add the aggregate task**

```toml
[tasks.ci]
description = "Full blocking gate set: UOM pin, generator determinism, python tests, build, ctest, conformance."
depends = ["uom-pin", "gen-determinism", "pytest", "build", "ctest", "sai-conformance-gate", "sai-invariants"]

[tasks.pytest]
description = "Run the generator test suite."
run = "uv run pytest tests/ -v"
```

- [ ] **Step 2: Write the workflow**

```yaml
name: ci
on: [push, pull_request]
jobs:
  ci:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: jdx/mise-action@v2
      - run: uv sync
      - run: mise run ci
```

- [ ] **Step 3: Verify locally**

```bash
mise run ci
```

Expected: PASS. Note `sai-conformance` (strict) is deliberately not in `depends`.

- [ ] **Step 4: Commit**

```bash
git add .github mise.toml
git commit -m "ci: blocking gate set for the split repo

The strict convergence gate stays out of the blocking set by design; it is
the re-entry trigger, not a health check."
```

---

## Task 7: Re-home the runtime reflection parity check in x3d-cpp

**Files:**
- Create: `/home/ben/code/x3d-cpp/runtime/scene/tests/node_reflection_uom_parity_test.cpp`
- Modify: `/home/ben/code/x3d-cpp/CMakeLists.txt` (register the test)

**Interfaces:**
- Consumes: nothing from the sister repo. This is x3d-cpp work on branch
  `docs/sai-sister-repo-split`.
- Produces: ctest `x3d_node_reflection_uom_parity`.

- [ ] **Step 1: Write the failing test**

The dropped block compared `X3DNodeFactory::create(name)->fields()` against the semantic
catalog. That catalog is gone from x3d-cpp, so compare against x3d-cpp's own generated
descriptors instead, which derive from the same UOM. Locate the descriptor accessor
first:

```bash
cd /home/ben/code/x3d-cpp
grep -rn 'descriptors\|fieldDescriptors' generated_cpp_bindings/x3d/nodes/X3DNodeFactory.hpp \
  runtime/scene/*.hpp | head
```

Then write, following the surrounding runtime test style:

```cpp
#include "doctest/doctest.h"

#include "x3d/nodes/X3DNodeFactory.hpp"

#include <string>

TEST_CASE("runtime node reflection matches the UOM-derived descriptors") {
  for (const auto &name : x3d::nodes::X3DNodeFactory::registeredNames()) {
    CAPTURE(name);
    const auto node = x3d::nodes::X3DNodeFactory::create(name);
    REQUIRE(node);
    const auto &reflected = node->fields();
    const auto &declared = x3d::nodes::X3DNodeFactory::descriptorsFor(name);
    REQUIRE(reflected.size() == declared.size());
    for (std::size_t i = 0; i < reflected.size(); ++i) {
      CHECK(reflected[i].x3dName == declared[i].name);
      CHECK(reflected[i].type == declared[i].type);
      CHECK(reflected[i].access == declared[i].access);
    }
  }
}
```

Adjust `registeredNames` and `descriptorsFor` to the real accessor names found in
Step 1. If no descriptor accessor exists, assert instead that field order, name, type,
and access are stable against `X3DSemanticMetadataRegistry`-equivalent data already
present in `generated_cpp_bindings/x3d/nodes/X3DNodeReflection*`.

- [ ] **Step 2: Register it in CMakeLists.txt**

Add next to the other `runtime/scene/tests` executables:

```cmake
    add_executable(x3d_node_reflection_uom_parity
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/node_reflection_uom_parity_test.cpp")
    target_link_libraries(x3d_node_reflection_uom_parity PRIVATE
        x3d_cpp::nodes x3d_doctest_main)
    add_test(NAME x3d_node_reflection_uom_parity
        COMMAND x3d_node_reflection_uom_parity)
```

- [ ] **Step 3: Build and run**

```bash
cmake --build build -j --target x3d_node_reflection_uom_parity \
  && ctest --test-dir build -R x3d_node_reflection_uom_parity --output-on-failure
```

Expected: PASS. If it fails, that is a genuine x3d-cpp bug the SAI test was catching;
fix it rather than weakening the assertion.

- [ ] **Step 4: Commit**

```bash
git add runtime/scene/tests/node_reflection_uom_parity_test.cpp CMakeLists.txt
git commit -m "test(runtime): assert node reflection matches UOM-derived descriptors

Re-homes the only runtime-level check that field order, name, type, and
access agree with the generated model. It previously lived in the
experimental SAI test suite, which moves to a sister repository."
```

---

## Task 8: Land the x3d-cpp ADR, docs, and archive tag

**Files:**
- Create: `docs/wiki/decisions/NNNN-sai-sister-repo-split.md`
- Create: `docs/wiki/subsystems/sai-semantic-kernel.md`
- Modify: `docs/wiki/coverage.md`, `mkdocs.yml`

- [ ] **Step 1: Pick the next ADR number**

```bash
ls docs/wiki/decisions/ | tail -3
```

Use the next unused number. ADR-0045 is taken (extraction instancing), so expect 0046
or higher.

- [ ] **Step 2: Write the ADR**

Content, following the existing ADR format in that directory:

- Context: the 40-commit unmerged branch, its size, the three drivers (independent
  lifecycle and audience, SDK scope and CI surface, contamination of the generated tree).
- Decision: sister repo `x3d-sai`, self-contained generator over the vendored Web3D UOM,
  no dependency in either direction.
- Consequences, stated plainly:
  - Accepted cost: two independently evolving parse layers, roughly 800 duplicated LOC,
    making eventual re-entry a reconciliation rather than a merge.
  - Cross-repo coherence is a data check: both repos pin the UOM by SHA-256
    (`0f1d9ede…6443f` for 4.0).
  - The live runtime-node parity assertion returned to x3d-cpp as
    `x3d_node_reflection_uom_parity` (Task 7).
- Re-entry: `mise run sai-conformance` going green in `x3d-sai` is the trigger; the
  decision is carried by a Project #2 card and is a decision, not a commitment.
- Rejected alternative: requiring an adapter in `x3d-sai` that drives x3d-cpp's runtime
  scene graph, because it reintroduces the dependency the split removes.

- [ ] **Step 3: Write the pointer page and wire the nav**

`docs/wiki/subsystems/sai-semantic-kernel.md`: a short page saying the experimental
semantic kernel lives in the `x3d-sai` sister repository, why, what x3d-cpp still owns
(`runtime/script/SaiContext.hpp` and the Script subsystem, which is a different thing),
and a link to the ADR. Add a `docs/wiki/coverage.md` row and an `mkdocs.yml` nav entry.

- [ ] **Step 4: Run the strict docs gate**

```bash
mise run docs-build
```

Expected: PASS. It fails on dead links and nav orphans, so a missing nav entry or a bad
ADR link shows up here.

- [ ] **Step 5: Run the drift suggester and the full CI**

```bash
mise run docs-drift working
mise run ci
```

Expected: `ci` PASS. Review the drift review-list, prioritising `CITES` hits.

- [ ] **Step 6: Commit**

```bash
git add docs/ mkdocs.yml
git commit -m "docs(adr): record the SAI sister-repository split

Adds the ADR, a subsystem pointer page, a coverage row, and the nav entry."
```

- [ ] **Step 7: Tag the archive and remove the branch and worktree**

Do this only after `x3d-sai` is verified green, since it is the point of no easy return.

```bash
git tag -a archive/sai-semantic-kernel 8e2a000 \
  -m "Pre-split head of the experimental SAI semantic kernel; continued in x3d-sai"
git worktree remove .worktrees/sai-semantic-kernel
git branch -D sai/semantic-kernel
git tag -l 'archive/*'
```

- [ ] **Step 8: Open the PR**

```bash
git push -u origin docs/sai-sister-repo-split
git push origin archive/sai-semantic-kernel
gh pr create --title "docs: split the experimental SAI semantic kernel into a sister repo" \
  --body "..."
```

The PR body should summarise the ADR and note the re-homed runtime test. No tool-specific
trailers or session URLs.

---

## Task 9: Create the re-entry tracking card

**Files:** none; this is GitHub Project #2 state.

- [ ] **Step 1: Check auth scope**

```bash
gh auth status
```

Creating project items needs the `project` scope. If absent, run
`gh auth refresh -s project`.

- [ ] **Step 2: Create the card**

```bash
gh project item-create 2 --owner delta9000 \
  --title "Evaluate SAI kernel re-entry into x3d-cpp" \
  --body "Blocked on: mise run sai-conformance going green in the x3d-sai sister repo.
When the strict convergence gate passes, decide whether to absorb the semantic
kernel as x3d-cpp's authoring API. See ADR-NNNN. Known cost at re-entry: two
independently evolved UOM parse layers to reconcile."
```

Replace `ADR-NNNN` with the number chosen in Task 8.

- [ ] **Step 3: Verify**

```bash
gh project item-list 2 --owner delta9000 | grep -i "SAI kernel re-entry"
```

---

## Self-Review Notes

Spec coverage check against `docs/superpowers/specs/2026-07-19-sai-sister-repo-split-design.md`:

| Spec section | Task |
| --- | --- |
| Repository layout | 1, 2 |
| Firewall correction: doctest | 2 |
| Firewall correction: runtime nodes | 3, 7 |
| Self-contained generator, UOM SHA pin | 4 |
| Migration: filter-repo, three seed commits | 1, 2, 4 |
| Migration: x3d-cpp archive tag and docs | 8 |
| Gates | 5, 6 |
| Re-entry contract | 8 (ADR), 9 (card) |

Deviation from the spec's layout section, adopted deliberately: `docs/conformance/`,
`scripts/`, `tests/`, and `generated_cpp_bindings/x3d/...` keep their original paths
rather than flattening to `conformance/` and `generated/`. `scripts/sai_conformance.py`
resolves `_repo_root() / "docs" / "conformance"`, and `metadata.cpp` includes
`"x3d/nodes/X3DSemanticMetadataRegistry.hpp"`. Keeping the paths means zero path edits
in Python or C++, which removes a whole class of migration bug. The C++ sources still
flatten out of `experimental/sai/`, since nothing depends on that prefix.

Known soft spots for the implementer:

- Task 4 Step 5: the CLI's calls into the moved emitters are reconstructed from
  `generator.py:write_sai_bindings` and `write_semantic_metadata_registry` on the
  branch. Read those two functions before writing the CLI; the argument order is the
  contract, and Step 8's determinism check is what proves it right.
- Task 7 Step 1: the exact descriptor accessor names are unverified. Grep first, then
  write the test against what exists.

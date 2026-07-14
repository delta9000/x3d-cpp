# Compile Topology Follow-up Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Remove accidental node-library work from header CI, compile the doctest
entry point once, and eliminate the all-node factory compile straggler without
changing public APIs.

**Architecture:** Compile-only targets consume the exported headers usage layer,
while link-and-run installation coverage belongs to behavior. A single internal
static test-main library serves every doctest executable. Generated concrete
node sources define creator functions and the factory becomes a declaration and
function-pointer table instead of an all-headers translation unit.

**Tech Stack:** CMake 3.20+, Ninja, CTest, C++20, GCC, ccache, Python 3.12,
pytest, Jinja2, GitHub Actions.

**Design reference:** `docs/plans/2026-07-14-compile-topology-follow-up-design.md`

---

## Execution rules

1. Work only in `.worktrees/compile-contract-factory-phase2` on
   `perf/compile-contract-factory-phase2`.
2. Add a failing structural test before each CMake or generator change.
3. Keep graph correction, doctest reuse, and factory generation in separate
   commits so their measurements and reviews remain attributable.
4. Use `CCACHE_DISABLE=1` and new build directories for before/after timings.
5. Do not begin moving hand-written runtime definitions in this branch.

## Task 1: Capture the merged baseline

**Files:**

- Create after implementation: `docs/plans/2026-07-14-compile-topology-follow-up-results.md`
- Do not commit build directories or raw timing logs.

**Step 1: Configure clean before-build directories from the merged worktree**

Use the unchanged PR #54 worktree as the before source:

```bash
CCACHE_DISABLE=1 cmake --preset ci \
  -S ../perf-build-ci-compile-structure \
  -B /tmp/x3d-compile-topology-before
```

Expected: configure succeeds with shared nodes and 407 contract sources.

**Step 2: Record the compile-contract command graph**

```bash
ninja -C /tmp/x3d-compile-topology-before -t commands \
  x3d_compile_contracts > /tmp/x3d-contract-commands-before.txt
rg -c 'x3d_cpp_nodes.dir' /tmp/x3d-contract-commands-before.txt
```

Expected before the fix: the count is nonzero and includes the generated node
runtime compile/link commands.

**Step 3: Measure the cold contract target**

```bash
CCACHE_DISABLE=1 /usr/bin/time -v cmake --build \
  /tmp/x3d-compile-topology-before --target x3d_compile_contracts -j 16
```

Record elapsed time, user time, system time, and maximum RSS.

**Step 4: Identify and record the factory unity object**

```bash
rg -l 'X3DNodeFactory.cpp' \
  /tmp/x3d-compile-topology-before/CMakeFiles/x3d_cpp_nodes.dir/Unity/*.cxx
```

Record its `.ninja_log` duration and object size after the contract build.

## Task 2: Make compile contracts independent of the node runtime

**Files:**

- Modify: `tests/test_cmake_aggregates.py`
- Modify: `tests/test_header_contract_shape.py`
- Modify: `CMakeLists.txt`

**Step 1: Write failing graph and label assertions**

Extend the configured-CI tests to run:

```python
commands = run_checked(
    "ninja", "-C", str(configured_ci), "-t", "commands",
    "x3d_compile_contracts",
)
assert "x3d_cpp_nodes.dir" not in commands
assert "libx3d_cpp_nodes" not in commands
```

Update the label assertions so:

```python
assert labels_for(tests["x3d_install_embed_smoke"]) == {"behavior"}
assert labels_for(tests["x3d_cpp_all_headers"]) == {"compile-contract"}
assert labels_for(tests["x3d_header_isolation"]) == {"compile-contract"}
```

Also assert the contract aggregate still has exactly
`x3d_cpp_all_headers` and `x3d_header_isolation` as inputs.

**Step 2: Verify RED**

```bash
uv run pytest tests/test_cmake_aggregates.py \
  tests/test_header_contract_shape.py -q
```

Expected: failures show node-runtime commands in the compile-contract graph and
the install smoke still labeled `compile-contract`.

**Step 3: Apply the minimal CMake correction**

Change both compile-only targets:

```cmake
target_link_libraries(x3d_cpp_all_headers PRIVATE x3d_cpp::headers)
target_link_libraries(x3d_header_isolation PRIVATE x3d_cpp::headers)
```

Move `x3d_install_embed_smoke` from `_x3d_compile_contract_tests` to
`_x3d_behavior_tests` when `NOT X3D_CPP_SAN`. Do not add it as a build target;
the behavior aggregate already builds linked SDK consumers and the test itself
owns the install/configure/build/run sequence.

**Step 4: Verify GREEN and inspect the graph**

```bash
uv run pytest tests/test_cmake_aggregates.py \
  tests/test_header_contract_shape.py -q
cmake --preset ci -B /tmp/x3d-contract-graph-after
ninja -C /tmp/x3d-contract-graph-after -t commands \
  x3d_compile_contracts | rg 'x3d_cpp_nodes|X3DNodeFactory'
```

Expected: pytest passes and the final `rg` returns no matches.

**Step 5: Commit**

```bash
git add CMakeLists.txt tests/test_cmake_aggregates.py \
  tests/test_header_contract_shape.py
git commit -m "build: detach header contracts from node runtime"
```

## Task 3: Compile doctest main once

**Files:**

- Modify: `tests/test_cmake_aggregates.py`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing reuse test**

Read `compile_commands.json` from the configured CI fixture and assert exactly
one entry has `runtime/test_support/doctest_main.cpp` as its source. For the five
default grouped suites, query Ninja and assert each link graph contains
`libx3d_doctest_main.a`:

```python
DEFAULT_DOCTEST_TARGETS = {
    "x3d_geometry_scene_tests",
    "x3d_codecs_tests",
    "x3d_parse_tests",
    "x3d_extract_tests",
    "x3d_events_tests",
}
```

**Step 2: Verify RED**

```bash
uv run pytest \
  tests/test_cmake_aggregates.py::test_doctest_main_is_compiled_once_and_reused -q
```

Expected: five compile-command entries and no shared archive.

**Step 3: Add the internal test-main library**

Immediately after the top-level test options, add:

```cmake
if(X3D_CPP_BUILD_TESTS)
    add_library(x3d_doctest_main STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp")
    target_include_directories(x3d_doctest_main PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support")
endif()
```

Remove `doctest_main.cpp` from all eight doctest executable source lists and add
`x3d_doctest_main` to their private link libraries. Include the optional text,
texture, and movie suites as well as the five default suites. Classify
`x3d_doctest_main` as `internal` in the target-purpose inventory.

**Step 4: Verify GREEN**

```bash
uv run pytest tests/test_cmake_aggregates.py -q
cmake --preset ci -B /tmp/x3d-doctest-main
cmake --build /tmp/x3d-doctest-main --target \
  x3d_geometry_scene_tests x3d_codecs_tests x3d_parse_tests \
  x3d_extract_tests x3d_events_tests -j 16
ctest --test-dir /tmp/x3d-doctest-main -R \
  'x3d_(geometry_scene|codecs_tests|parse_tests|extract_tests|events_tests)$' \
  --output-on-failure
```

Expected: one main compile, five links, and five passing grouped tests.

**Step 5: Commit**

```bash
git add CMakeLists.txt tests/test_cmake_aggregates.py
git commit -m "build(test): reuse one doctest main library"
```

## Task 4: Distribute node creator functions

**Files:**

- Modify: `tests/test_decl_def_split.py`
- Modify: `src/x3d_cpp_gen/backends/cpp_header.py`
- Modify: `src/x3d_cpp_gen/templates/class_template.cpp.jinja`
- Modify: `src/x3d_cpp_gen/emit/factory.py`
- Regenerate: `generated_cpp_bindings/x3d/nodes/*.cpp`

**Step 1: Write failing generator tests**

Assert the small two-node factory source:

```python
assert '#include "x3d/nodes/Box.hpp"' not in src
assert "std::make_shared<Box>()" not in src
assert "std::shared_ptr<X3DNode> createBox();" in src
assert '{"Box", &factory_detail::createBox}' in src
```

Extend the real generator test so `Box.cpp` contains both
`factory_detail` and `createBox`, while abstract `X3DGeometryNode.cpp` does not
contain a creator.

**Step 2: Verify RED**

```bash
uv run pytest tests/test_decl_def_split.py -q
```

Expected: the factory still includes `Box.hpp` and node sources have no creator.

**Step 3: Emit creators only for concrete nodes**

Pass `is_abstract=node.is_abstract` to the source template. At the end of a
concrete node source, inside `x3d::nodes`, emit:

```cpp
namespace factory_detail {
std::shared_ptr<X3DNode> createBox() { return std::make_shared<Box>(); }
} // namespace factory_detail
```

Use the generated class name rather than special-casing node types.

**Step 4: Make the factory include-free**

In `gen_node_factory_source`, remove `X3DNode.hpp` and every concrete node
include. Emit matching declarations in `factory_detail`, then use function
pointers in the map:

```cpp
namespace factory_detail {
std::shared_ptr<X3DNode> createBox();
}

static const std::unordered_map<std::string, Creator> reg = {
    {"Box", &factory_detail::createBox},
};
```

Do not use static self-registration; it is unsafe with the supported static
node-library mode because archive members may be discarded.

**Step 5: Verify generator tests and regenerate the committed tree**

```bash
uv run pytest tests/test_decl_def_split.py tests/test_nodes_namespace_emit.py -q
uv run x3d-cpp-gen --out generated_cpp_bindings --no-test
uv run pytest tests/test_golden_tree.py tests/test_golden_smoke.py \
  tests/test_emission.py -q
git diff --check
```

Expected: concrete generated sources gain one creator; abstract sources do not;
`X3DNodeFactory.cpp` contains no concrete node include; golden checks pass.

**Step 6: Build and exercise both library modes**

```bash
cmake --preset ci -B /tmp/x3d-factory-shared
cmake --build /tmp/x3d-factory-shared --target \
  x3d_cpp_nodes x3d_sdk_facade x3d_parse_tests -j 16
ctest --test-dir /tmp/x3d-factory-shared -R \
  'x3d_(sdk_facade|parse_tests)$' --output-on-failure

cmake --preset ci -B /tmp/x3d-factory-static \
  -DX3D_CPP_SHARED_NODES=OFF
cmake --build /tmp/x3d-factory-static --target \
  x3d_sdk_facade x3d_parse_tests -j 16
ctest --test-dir /tmp/x3d-factory-static -R \
  'x3d_(sdk_facade|parse_tests|install_embed_smoke)$' --output-on-failure
```

**Step 7: Commit**

```bash
git add src/x3d_cpp_gen tests/test_decl_def_split.py \
  generated_cpp_bindings/x3d/nodes
git commit -m "build(codegen): distribute node factory creators"
```

## Task 5: Measure and record the result

**Files:**

- Create: `docs/plans/2026-07-14-compile-topology-follow-up-results.md`

**Step 1: Repeat the clean configuration and contract timing**

```bash
CCACHE_DISABLE=1 cmake --preset ci \
  -B /tmp/x3d-compile-topology-after
CCACHE_DISABLE=1 /usr/bin/time -v cmake --build \
  /tmp/x3d-compile-topology-after --target x3d_compile_contracts -j 16
```

**Step 2: Record structural deltas**

Record:

- node compile/link commands reachable from `x3d_compile_contracts`: before vs
  after;
- doctest-main compile-command count: before vs after;
- factory unity object `.ninja_log` milliseconds and bytes: before vs after;
- cold contract wall/user/system/RSS: before vs after.

Explain that whole-target timing is machine-specific while graph removal and
compile counts are deterministic.

**Step 3: Commit**

```bash
git add docs/plans/2026-07-14-compile-topology-follow-up-results.md
git commit -m "docs: record compile topology measurements"
```

## Task 6: Full verification and publication

**Files:** No expected source changes unless verification exposes a defect.

**Step 1: Run the full Python suite**

```bash
uv run pytest
```

Expected: 343+ tests pass with only environment-gated skips.

**Step 2: Run scoped C++ gates**

```bash
cmake --preset ci -B build-ci
cmake --build build-ci --target x3d_behavior_tests -j 16
ctest --test-dir build-ci -L behavior --output-on-failure -j 16
cmake --build build-ci --target x3d_compile_contracts -j 16
ctest --test-dir build-ci -L compile-contract --output-on-failure -j 16
```

Expected: both labeled suites pass; the install smoke runs in behavior only.

**Step 3: Run sanitizer behavior and example consumers**

```bash
cmake --preset san -B build-san
cmake --build build-san --target x3d_sanitizer_tests -j 16
ctest --test-dir build-san -L behavior --output-on-failure -j 16
scripts/validate-examples.sh
```

Expected: sanitizer behavior passes; CPU raster, PoC renderer, and asset-import
consumers compile and run their headless probes.

**Step 4: Review and publish**

Inspect `git diff origin/main...HEAD`, run `git diff --check`, confirm the
worktree is clean, push `perf/compile-contract-factory-phase2`, and open a draft
PR summarizing deterministic graph changes, measured timings, compatibility,
and verification evidence.

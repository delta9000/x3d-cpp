# Build and CI Compile-Structure Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Cut cold compile and PR latency while ensuring every compiled target has an explicit production, behavioral, compile-contract, or opt-in purpose, then move high-cost hand-written runtime implementations into compiled libraries.

**Architecture:** Phase one makes the existing graph intentional: explicit target taxonomy, aggregate behavior/sanitizer/header-contract targets, scoped CI jobs, Debug sanitizers, and removal of accidental C++ compilation from Python namespace tests. Phase two introduces an acyclic headers → generated nodes → authoring runtime → full runtime topology and migrates measured high-fan-out non-template implementations out of headers in subsystem waves.

**Tech Stack:** CMake 3.20+, Ninja, CTest, C++20, GCC/Clang, ccache, Python/pytest, mise, GitHub Actions.

**Design reference:** `docs/plans/2026-07-13-build-ci-compile-structure-design.md`

---

## Global execution rules

1. Work from a clean dedicated branch/worktree. Do not mix this with asset-import feature changes.
2. Preserve generated output unless a separately reviewed generator change requires it.
3. Use `CCACHE_DISABLE=1` for before/after cold-build comparisons. Cache-hit timings are diagnostic only.
4. Use the same compiler, generator, build type, and parallelism for every comparison.
5. After moving any public definition out of a header, run the install/embed smoke before proceeding.
6. After each phase-two wave, keep the change only if it produces a material measured benefit: at least 10% cumulative compiler-time improvement in affected targets, or an equivalent demonstrated memory/codegen reduction.
7. Templates, `constexpr` functions, small POD accessors, and seam type aliases stay in headers unless a failing test proves otherwise.
8. Commit after every task. Each commit must be independently buildable and reviewable.

## Phase 0: establish reproducible evidence

### Task 1: Add a repeatable build benchmark script

**Files:**
- Create: `scripts/benchmark_build.py`
- Create: `tests/test_benchmark_build.py`
- Modify: `.gitignore`

**Step 1: Write the failing parser test**

Test pure functions that parse `/usr/bin/time` and Ninja log records into this JSON shape:

```python
{
    "configure_wall_s": 0.35,
    "build_wall_s": 80.65,
    "build_user_s": 1081.60,
    "test_wall_s": 28.88,
    "targets": {"x3d_cpp_nodes": {"objects": 43, "elapsed_sum_s": 202.3}},
}
```

Also test rejection of a benchmark directory inside the source tree and rejection of an existing directory unless `--reuse` is supplied.

**Step 2: Verify RED**

Run:

```bash
uv run pytest tests/test_benchmark_build.py -q
```

Expected: FAIL because `scripts.benchmark_build` does not exist.

**Step 3: Implement the minimal benchmark driver**

The script must:

- accept `--source`, `--build-dir`, `--preset dev|ci|san`, `--jobs`, `--output`, and `--reuse`;
- set `CCACHE_DISABLE=1` in child processes;
- configure, build, and test with `/usr/bin/time -f`;
- parse `.ninja_log` by target;
- write JSON under `build-benchmarks/`, which is gitignored;
- print the output path and the three wall/user totals.

Do not delete build directories. Refuse unsafe reuse instead.

**Step 4: Verify GREEN**

```bash
uv run pytest tests/test_benchmark_build.py -q
uv run python scripts/benchmark_build.py --help
```

**Step 5: Capture the baseline**

```bash
uv run python scripts/benchmark_build.py \
  --source . --build-dir /tmp/x3d-benchmark-dev-before \
  --preset dev --jobs 16 \
  --output build-benchmarks/phase0-dev-before.json

uv run python scripts/benchmark_build.py \
  --source . --build-dir /tmp/x3d-benchmark-ci-before \
  --preset ci --jobs 16 \
  --output build-benchmarks/phase0-ci-before.json
```

Record the machine/compiler metadata in the JSON. Do not commit the measurement file.

**Step 6: Commit**

```bash
git add scripts/benchmark_build.py tests/test_benchmark_build.py .gitignore
git commit -m "build: add reproducible cold-build benchmark"
```

## Phase 1: make the current graph intentional

### Task 2: Fix accidental C++ compilation in Python namespace tests

**Files:**
- Modify: `tests/test_nodes_namespace_emit.py`

This is test-harness configuration, not production behavior. The existing tests are the regression coverage; no meta-test that inspects its own source should be added.

**Step 1: Record the failing performance baseline**

```bash
uv run pytest tests/test_nodes_namespace_emit.py -q --durations=2
```

Expected before the change: both tests pass but each takes roughly 189 seconds on the reference machine because the CLI compiles `test.cpp`.

**Step 2: Make the minimal change**

Add `--no-test` to both `x3d_cpp_gen.cli` subprocess argument lists. Also remove duplicate/local imports and format the module imports one per line.

**Step 3: Verify behavior and timing**

```bash
uv run pytest tests/test_nodes_namespace_emit.py -q --durations=2
```

Expected: both assertions still pass; neither invocation performs a C++ compile; combined runtime is dominated by generation/formatting rather than compiler execution.

**Step 4: Run the generator-focused suite**

```bash
uv run pytest tests/test_nodes_namespace_emit.py tests/test_golden_tree.py \
  tests/test_decl_def_split.py tests/test_golden_smoke.py -q
```

**Step 5: Commit**

```bash
git add tests/test_nodes_namespace_emit.py
git commit -m "test(generator): skip irrelevant C++ smoke compiles"
```

### Task 3: Introduce and enforce target-purpose metadata

**Files:**
- Create: `cmake/X3DTargetPurpose.cmake`
- Create: `tests/cmake/target_purpose/CMakeLists.txt`
- Create: `tests/test_cmake_target_purpose.py`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing CMake fixture**

The fixture should include `X3DTargetPurpose.cmake`, add one classified and one unclassified target, then call `x3d_validate_target_purposes()`.

Add a pytest/CMake script test that expects configuration to fail with:

```text
repository-owned target 'unclassified' has no X3D_TARGET_PURPOSE
```

Also test duplicate classification and an invalid purpose.

Valid purposes are exactly:

```text
production
behavior
compile-contract
opt-in
internal
```

**Step 2: Verify RED**

```bash
uv run pytest tests/test_cmake_target_purpose.py -q
```

Expected: FAIL because the helper is absent.

**Step 3: Implement the helper**

Provide:

```cmake
x3d_set_target_purpose(<target> <purpose>)
x3d_get_targets_by_purpose(<out-var> <purpose>)
x3d_register_test_target(<target> SUITES <behavior|sanitizer|compile-contract>...)
x3d_get_test_targets(<out-var> <suite>)
x3d_validate_target_purposes()
```

Store primary-purpose classifications and test-suite memberships as separate target properties/global lists. A shipped target such as `x3d` remains `production` while also belonging to the `behavior` and `sanitizer` test suites. Validation must inspect repository-owned targets in the root directory, ignore imported/alias targets, and emit one actionable error listing all unclassified targets. It must also reject test-suite membership for a nonexistent target.

**Step 4: Classify the current root targets**

At minimum:

- Production: `x3d_cpp`, `x3d_cpp_nodes`, `x3d_cpp_authoring`, `x3d_cpp_sdk`, `x3d_cli`.
- Behavior: every target directly executed by normal CTest.
- Compile contract: `x3d_cpp_all_headers` and the future header aggregate.
- Opt-in: corpus-only gates, fuzz, seam backends/tests, and renderer/asset-import consumers.
- Internal: vendored implementation libraries such as Duktape.

Place `x3d_validate_target_purposes()` after all root target declarations.

**Step 5: Verify GREEN**

```bash
uv run pytest tests/test_cmake_target_purpose.py -q
cmake -S . -B /tmp/x3d-purpose-check -G Ninja \
  -DX3D_CPP_PER_HEADER_CHECKS=OFF
```

Expected: fixture tests pass and the real project configures without unclassified-target errors.

**Step 6: Commit**

```bash
git add cmake/X3DTargetPurpose.cmake tests/test_cmake_target_purpose.py \
  tests/cmake/target_purpose/CMakeLists.txt CMakeLists.txt
git commit -m "build: classify every repository-owned target"
```

### Task 4: Replace 407 header targets with one object-library contract

**Files:**
- Create: `tests/test_header_contract_shape.py`
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`

**Step 1: Write the failing graph-shape test**

Configure the CI preset in a temporary build directory and assert:

- CTest contains exactly one test named `x3d_header_isolation`.
- Ninja contains one target named `x3d_header_isolation`.
- No target or test name begins with `x3d_syntax_` or `compile_`.
- The object target has one generated source per discovered contract header.

The expected source count must be derived from the same five header sets, not hard-coded to 407.

**Step 2: Verify RED**

```bash
uv run pytest tests/test_header_contract_shape.py -q
```

Expected: FAIL because the current graph contains hundreds of syntax targets/tests.

**Step 3: Refactor CMake source generation**

Accumulate all one-include generated source paths in `X3D_CPP_HEADER_ISOLATION_SOURCES`, then create exactly one target:

```cmake
add_library(x3d_header_isolation OBJECT EXCLUDE_FROM_ALL
    ${X3D_CPP_HEADER_ISOLATION_SOURCES})
target_link_libraries(x3d_header_isolation PRIVATE x3d_cpp::x3d_cpp)
x3d_set_target_purpose(x3d_header_isolation compile-contract)
add_test(NAME x3d_header_isolation
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
            --target x3d_header_isolation --config "$<CONFIG>")
set_tests_properties(x3d_header_isolation PROPERTIES LABELS compile-contract)
```

Keep each header in a distinct source/TU. Do not generate one all-includes source; `x3d_cpp_all_headers` already covers aggregate compatibility.

**Step 4: Verify GREEN and diagnostics**

```bash
uv run pytest tests/test_header_contract_shape.py -q
cmake -S . -B /tmp/x3d-header-contract -G Ninja \
  -DX3D_CPP_WERROR=ON -DX3D_CPP_PER_HEADER_CHECKS=ON
cmake --build /tmp/x3d-header-contract --target x3d_header_isolation -j 16
ctest --test-dir /tmp/x3d-header-contract -R '^x3d_header_isolation$' \
  --output-on-failure
```

Temporarily inject a missing include into a generated source in the temporary build directory and confirm Ninja names the failing isolation source; do not modify a repository header for this diagnostic check.

**Step 5: Compare graph size**

Record before/after values for Ninja targets, CTest entries, `build.ninja` bytes, wall time, user time, and system time.

**Step 6: Commit**

```bash
git add CMakeLists.txt CMakePresets.json tests/test_header_contract_shape.py
git commit -m "build: aggregate header isolation into one target"
```

### Task 5: Add behavior and sanitizer aggregate targets

**Files:**
- Create: `tests/test_cmake_aggregates.py`
- Modify: `cmake/X3DTargetPurpose.cmake`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing aggregate-coverage test**

From a configured build, obtain:

- targets registered in each test suite;
- executables referenced by CTest commands;
- dependencies of `x3d_behavior_tests`, `x3d_sanitizer_tests`, and `x3d_compile_contracts`.

Assert that every normal CTest executable is covered by `x3d_behavior_tests`, every sanitizer-eligible behavior target is covered by `x3d_sanitizer_tests`, and build-backed compile-contract tests are covered by `x3d_compile_contracts`. Neither behavior nor sanitizer aggregates may contain `x3d_cli_gate`, `x3d_canon_gate`, header isolation, fuzz, examples, or backend swap tests.

**Step 2: Verify RED**

```bash
uv run pytest tests/test_cmake_aggregates.py -q
```

**Step 3: Create aggregates from purpose metadata**

Register each CTest-backed executable when its test is declared. Production targets may also be suite members:

```cmake
x3d_set_target_purpose(x3d_cli production)
x3d_register_test_target(x3d_cli SUITES behavior sanitizer)
```

After all root targets are defined:

```cmake
x3d_get_test_targets(_x3d_behavior_targets behavior)
add_custom_target(x3d_behavior_tests)
add_dependencies(x3d_behavior_tests ${_x3d_behavior_targets})

x3d_get_test_targets(_x3d_sanitizer_targets sanitizer)
add_custom_target(x3d_sanitizer_tests)
add_dependencies(x3d_sanitizer_tests ${_x3d_sanitizer_targets})

x3d_get_test_targets(_x3d_contract_targets compile-contract)
add_custom_target(x3d_compile_contracts)
add_dependencies(x3d_compile_contracts ${_x3d_contract_targets})
```

Register behavior targets in `sanitizer` by default and omit them only when they cannot execute meaningfully under ASan/UBSan. Require an adjacent comment for every omission.

Apply the `behavior` CTest label to normal runtime tests and `compile-contract` to header/install/build-only contracts. Register `x3d_cpp_all_headers` and `x3d_header_isolation` in the compile-contract target suite. The shell-driven installed-consumer smoke has no owning executable target, so label it `compile-contract` without adding a target dependency; its script builds what it needs.

**Step 4: Verify GREEN**

```bash
uv run pytest tests/test_cmake_aggregates.py -q
cmake --build /tmp/x3d-purpose-check --target x3d_behavior_tests -j 16
ctest --test-dir /tmp/x3d-purpose-check -L behavior --output-on-failure -j 16
cmake --build /tmp/x3d-purpose-check --target x3d_compile_contracts -j 16
ctest --test-dir /tmp/x3d-purpose-check -L compile-contract \
  --output-on-failure -j 16
```

**Step 5: Commit**

```bash
git add cmake/X3DTargetPurpose.cmake CMakeLists.txt tests/test_cmake_aggregates.py
git commit -m "build: add behavior and sanitizer target aggregates"
```

### Task 6: Remove external-corpus gates from the default graph

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `mise.toml`
- Modify: `tests/test_cmake_aggregates.py`

**Step 1: Extend the failing test**

Assert `x3d_cli_gate` and `x3d_canon_gate` exist but are absent from Ninja's default `all` dependencies.

**Step 2: Verify RED**

```bash
uv run pytest tests/test_cmake_aggregates.py -q
```

**Step 3: Make gate tools explicit**

Declare both executables with `EXCLUDE_FROM_ALL` and classify them `opt-in`.

Update `tasks.cli-gate-regression` to run:

```bash
cmake --preset dev
cmake --build --preset dev --target x3d_cli_gate x3d_canon_gate
```

before executing the binaries. Check every other mise task that executes either binary and make its build dependency explicit.

**Step 4: Verify GREEN**

```bash
uv run pytest tests/test_cmake_aggregates.py -q
cmake --build /tmp/x3d-purpose-check --target x3d_behavior_tests -j 16
cmake --build /tmp/x3d-purpose-check --target x3d_cli_gate x3d_canon_gate -j 16
```

If the local corpus exists, run `mise run cli-gate-regression`. Otherwise verify both binaries start with `--help` or their bounded fixture mode.

**Step 5: Commit**

```bash
git add CMakeLists.txt mise.toml tests/test_cmake_aggregates.py
git commit -m "build: make corpus gates opt-in targets"
```

### Task 7: Make the sanitizer build debug and behavior-only

**Files:**
- Modify: `CMakePresets.json`
- Modify: `mise.toml`
- Modify: `.github/workflows/ci.yml`
- Modify: `tests/test_cmake_aggregates.py`

**Step 1: Extend the failing preset test**

Assert the `san` configure preset uses `CMAKE_BUILD_TYPE=Debug`, keeps `X3D_CPP_SAN=ON`, keeps per-header checks off, and the build command targets `x3d_sanitizer_tests`.

**Step 2: Verify RED**

```bash
uv run pytest tests/test_cmake_aggregates.py -q
```

**Step 3: Update local and CI commands**

Use:

```bash
cmake --preset san
cmake --build --preset san --target x3d_sanitizer_tests
ctest --preset san -L behavior --output-on-failure -j "$(nproc)"
```

Do not set optimization flags manually; let CMake's Debug flags apply. Keep frame pointers and the existing sanitizer flags.

**Step 4: Verify sanitizer behavior**

```bash
CCACHE_DISABLE=1 cmake --preset san -B /tmp/x3d-san-debug
CCACHE_DISABLE=1 cmake --build /tmp/x3d-san-debug \
  --target x3d_sanitizer_tests -j 16
ctest --test-dir /tmp/x3d-san-debug -L behavior --output-on-failure -j 16
```

Compare against the phase-0 sanitizer baseline. Investigate any test removed from execution; do not accept a count reduction without an explicit classification reason.

**Step 5: Commit**

```bash
git add CMakePresets.json mise.toml .github/workflows/ci.yml \
  tests/test_cmake_aggregates.py
git commit -m "build(ci): scope sanitizer compilation to debug behavior tests"
```

### Task 8: Extract and test CI change classification

**Files:**
- Create: `scripts/classify_ci_changes.py`
- Create: `tests/test_classify_ci_changes.py`
- Modify: `.github/workflows/ci.yml`

**Step 1: Write table-driven failing tests**

Cover at least:

| Changed path | Expected classes |
|---|---|
| `runtime/parse/X3DParse.hpp` | `cpp`, parse/asset-related seam classes |
| `generated_cpp_bindings/x3d/nodes/Box.hpp` | `cpp` |
| `src/x3d_cpp_gen/generator.py` | `generator`, not `cpp` |
| generator plus regenerated `Box.hpp` | `generator`, `cpp` |
| `scripts/code_rag.py` | Python/tooling only; not `cpp` |
| `docs/wiki/index.md` | `docs` only |
| `CMakeLists.txt` | `cpp` and every build-sensitive seam |
| empty/unknown input | fail-safe classes |

**Step 2: Verify RED**

```bash
uv run pytest tests/test_classify_ci_changes.py -q
```

**Step 3: Implement the pure classifier**

The script reads newline-delimited paths from stdin and emits `key=true|false` lines suitable for `$GITHUB_OUTPUT`. Keep regex tables in Python constants and expose a pure `classify(paths)` function.

**Step 4: Replace embedded shell regexes**

The `changes` job still computes the merge base and changed path list, then pipes it to the classifier. Manual dispatch must request every class. Preserve Forgejo compatibility: Python 3 and git only, no GitHub-specific third-party action.

Gate sanitizer, fuzz, texture, and examples on `cpp`. Keep stable job names. The normal C++ job may skip when `cpp=false`; document branch-protection implications in the workflow comment.

**Step 5: Verify GREEN**

```bash
uv run pytest tests/test_classify_ci_changes.py -q
python3 scripts/classify_ci_changes.py <<'EOF'
src/x3d_cpp_gen/generator.py
EOF
```

Expected: `generator=true`, `cpp=false`.

**Step 6: Commit**

```bash
git add scripts/classify_ci_changes.py tests/test_classify_ci_changes.py \
  .github/workflows/ci.yml
git commit -m "ci: classify C++ changes separately from tooling"
```

### Task 9: Rewire the fast C++ and header-contract jobs

**Files:**
- Modify: `.github/workflows/ci.yml`
- Modify: `CMakePresets.json`
- Modify: `mise.toml`
- Modify: `tests/test_header_contract_shape.py`

**Step 1: Extend the failing workflow test**

Parse the workflow YAML and assert:

- the fast C++ job builds `x3d_behavior_tests` and runs `ctest -L behavior`;
- a separate compile-contract job builds `x3d_compile_contracts` and runs
  `ctest -L compile-contract`;
- both are gated by `cpp` but retain stable display names;
- no job invokes the unconstrained default build for these gates.

**Step 2: Verify RED**

```bash
uv run pytest tests/test_header_contract_shape.py -q
```

**Step 3: Update workflow and local mirrors**

Use the `ci` preset for both jobs. The behavior job builds `x3d_behavior_tests`; the contract job builds `x3d_compile_contracts`, which includes `x3d_header_isolation` and `x3d_cpp_all_headers`, then runs the `compile-contract` label (including install/embed). `mise run build-ci` runs both aggregates locally in sequence and then their labels/tests.

Keep ccache namespaces separate for behavior, sanitizer, and header contracts. Add `ccache --max-size=750M` before compilation so one PR cannot fill the repository cache quota with multi-gigabyte archives.

**Step 4: Verify locally**

```bash
cmake --preset ci
cmake --build --preset ci --target x3d_behavior_tests -j 16
ctest --preset ci -L behavior --output-on-failure -j 16
cmake --build --preset ci --target x3d_compile_contracts -j 16
ctest --preset ci -L compile-contract --output-on-failure -j 16
```

**Step 5: Commit**

```bash
git add .github/workflows/ci.yml CMakePresets.json mise.toml \
  tests/test_header_contract_shape.py
git commit -m "ci: split behavior and header compile contracts"
```

### Task 10: Verify and document phase one

**Files:**
- Modify: `docs/wiki/guides/build-and-mise.md`
- Modify: `docs/wiki/architecture.md`
- Modify: `README.md` if command behavior changed there
- Modify: `CLAUDE.md` if the canonical gate list changed

**Step 1: Run phase-one gates**

```bash
uv run pytest -q --durations=20
mise run golden
mise run conformance-gate
mise run coverage-gate
mise run doc-ctest-gate
mise run build
mise run build-ci
mise run build-san
mise run authoring-footprint
scripts/validate-examples.sh
```

**Step 2: Re-run cold benchmarks**

Create `phase1-dev-after.json`, `phase1-ci-after.json`, and `phase1-san-after.json` under the ignored benchmark directory using identical settings to phase 0.

**Step 3: Update documentation with measured facts**

Document:

- behavior/header/sanitizer aggregate commands;
- exact current target/test counts;
- phase-zero and phase-one cold timings;
- why header isolation is one target but still one TU per header;
- why global unity and PCH are not the selected remedies;
- which gates are opt-in.

Remove stale current-tense claims about roughly 70 executables, roughly 800 tests, and the historical 39-second cold build.

**Step 4: Run docs gates**

```bash
mise run docs-drift working
mise run docs-build
```

**Step 5: Commit**

```bash
git add docs/wiki/guides/build-and-mise.md docs/wiki/architecture.md \
  README.md CLAUDE.md
git commit -m "docs(build): record phase-one compile topology and timings"
```

## Phase 2: compile the hand-written runtime once

### Task 11: Establish the acyclic compiled-library topology

**Files:**
- Create: `tests/cmake/target_topology/CMakeLists.txt`
- Create: `tests/test_target_topology.py`
- Modify: `CMakeLists.txt`
- Modify: `cmake/x3d_cppConfig.cmake.in` if exported names require it

**Step 1: Write failing topology/install tests**

Assert the configured and installed target graph is:

```text
x3d_cpp_headers
  <- x3d_cpp_nodes
  <- x3d_cpp_authoring_runtime
  <- x3d_cpp_runtime
```

and that:

- `x3d_cpp::authoring` links headers + nodes + authoring runtime, not full runtime;
- `x3d_cpp::x3d_cpp` links all four implementation layers;
- `x3d_cpp::sdk` links `x3d_cpp::x3d_cpp`;
- exported targets contain no source-tree paths and no missing internal dependency;
- a downstream installed consumer can link each façade.

**Step 2: Verify RED**

```bash
uv run pytest tests/test_target_topology.py -q
```

**Step 3: Introduce empty compiled layers**

Create:

```cmake
add_library(x3d_cpp_headers INTERFACE)
add_library(x3d_cpp::headers ALIAS x3d_cpp_headers)

add_library(x3d_cpp_authoring_runtime STATIC runtime/compiled/AuthoringAnchor.cpp)
add_library(x3d_cpp::authoring_runtime ALIAS x3d_cpp_authoring_runtime)

add_library(x3d_cpp_runtime STATIC runtime/compiled/RuntimeAnchor.cpp)
add_library(x3d_cpp::runtime ALIAS x3d_cpp_runtime)
```

Anchor files contain only a namespace-scoped internal symbol and disappear once real sources arrive.

Move include directories and `cxx_std_20` from the circular interface onto `x3d_cpp_headers`. Make generated nodes depend on headers, authoring runtime on headers + nodes, and full runtime on headers + nodes + authoring runtime. Export/install all implementation targets with stable export names.

**Step 4: Verify GREEN and all façade consumers**

```bash
uv run pytest tests/test_target_topology.py -q
cmake -S . -B /tmp/x3d-topology -G Ninja -DX3D_CPP_PER_HEADER_CHECKS=OFF
cmake --build /tmp/x3d-topology --target x3d_behavior_tests -j 16
ctest --test-dir /tmp/x3d-topology -R 'x3d_(sdk_facade|install_embed_smoke)' \
  --output-on-failure
mise run authoring-footprint
```

**Step 5: Commit**

```bash
git add CMakeLists.txt cmake/x3d_cppConfig.cmake.in runtime/compiled \
  tests/cmake/target_topology tests/test_target_topology.py
git commit -m "build: establish acyclic compiled runtime layers"
```

### Task 12: Split FieldValueIO and codec writers into the authoring runtime

**Files:**
- Create: `runtime/codecs/FieldValueIO.cpp`
- Create: `runtime/codecs/XmlWriter.cpp`
- Create: `runtime/codecs/JsonWriter.cpp`
- Create: `runtime/codecs/VrmlWriter.cpp`
- Create: `runtime/codecs/CanonicalXmlWriter.cpp`
- Modify: corresponding `.hpp` files
- Modify: `CMakeLists.txt`
- Modify: codec tests under `runtime/codecs/tests/`

**Step 1: Add a link-boundary regression test**

Create a small consumer TU that includes only `x3d/authoring.hpp`, constructs a document, and calls all four writers plus representative non-template FieldValueIO operations. First link it against headers + nodes only and verify unresolved symbols, proving the test exercises the new compiled boundary.

**Step 2: Move non-template definitions**

- Remove `inline` and bodies from non-template FieldValueIO functions.
- Keep matrix templates and compile-time helpers in the header.
- Move writer public/private non-template method bodies into their `.cpp` files.
- Keep public signatures and class data layout unchanged in this wave.
- Add the sources explicitly to `x3d_cpp_authoring_runtime`; do not use a glob.

**Step 3: Verify**

```bash
cmake --build build --target x3d_codecs_tests x3d_sdk_facade -j 16
ctest --test-dir build -R 'x3d_(codecs_tests|canonicalize|authoring)' \
  --output-on-failure
mise run asset-import
mise run authoring-footprint
ctest --test-dir build -R x3d_install_embed_smoke --output-on-failure
```

**Step 4: Benchmark the codec group**

Compare cumulative time for `x3d_codecs_tests`, writer-using standalone targets, and authoring consumers. Retain only if the wave meets the global threshold.

**Step 5: Commit**

```bash
git add runtime/codecs CMakeLists.txt runtime/codecs/tests
git commit -m "build(codecs): compile writer implementations once"
```

### Task 13: Split parse readers and front-door functions into the full runtime

**Files:**
- Create: `runtime/parse/ClassicVrmlReader.cpp`
- Create: `runtime/parse/Vrml97Reader.cpp`
- Create: `runtime/parse/JsonReader.cpp`
- Create: `runtime/parse/XmlReaderAdapter.cpp`
- Create: `runtime/parse/NodeBuilder.cpp`
- Create: `runtime/parse/X3DParse.cpp`
- Modify: corresponding `.hpp` files
- Modify: `CMakeLists.txt`
- Modify: parse tests under `runtime/parse/tests/`

**Step 1: Add a downstream parse link test**

The test includes `x3d/sdk.hpp`, calls `parseDocument` for XML, JSON, ClassicVRML, and VRML97 fixtures, and verifies the same scene summaries. Verify it fails to link if `x3d_cpp_runtime` is intentionally omitted.

**Step 2: Move implementations**

- Move reader method bodies and dialect hooks out of class definitions.
- Move non-template `NodeBuilder` helpers; retain generic visitor/token templates.
- Move `stripUtf8Bom`, reader construction, parse front doors, and resolver loops from `X3DParse.hpp`.
- Move private parsing tables/state machines into `.cpp` anonymous namespaces where they need not affect class layout.
- Add all sources explicitly to `x3d_cpp_runtime`.

Do not introduce PIMPL in the first pass. After measurement, use PIMPL only for a reader whose private implementation types force most heavy includes to remain public.

**Step 3: Verify**

```bash
cmake --build build --target x3d_parse_tests x3d_parse_reader \
  x3d_classic_vrml_reader x3d_json_reader x3d_vrml97_reader -j 16
ctest --test-dir build -R 'x3d_(parse|classic_vrml|json_reader|vrml97)' \
  --output-on-failure
cmake --build build-ci --target x3d_header_isolation -j 16
```

Run golden drift to confirm generated files remain unchanged.

**Step 4: Benchmark and commit**

Retain only if parse-target cumulative compilation improves materially.

```bash
git add runtime/parse CMakeLists.txt
git commit -m "build(parse): compile reader implementations once"
```

### Task 14: Split document, scene, PROTO, and range authoring bodies

**Files:**
- Create `.cpp` peers for high-cost root runtime headers selected by profiling, including `X3DDocument`, `X3DScene`, `X3DProtoExpand`, and `X3DRangeValidate`
- Modify their `.hpp` files
- Modify: `CMakeLists.txt`
- Modify relevant runtime/codecs/parse tests

**Step 1: Profile and select exact methods**

Use the phase-0 benchmark plus representative `-ftime-trace`/`-ftime-report` output. Write the selected symbols and baseline contribution into the commit message or an ignored benchmark note. Do not move small struct accessors.

**Step 2: Add link-boundary tests**

Cover document construction, route resolution, PROTO expansion, and range collection through both `x3d_cpp::authoring` and `x3d_cpp::sdk`.

**Step 3: Move definitions to the correct layer**

- Document/writer/range functionality needed by authoring goes to `x3d_cpp_authoring_runtime`.
- Parser-only or execution-only functionality goes to `x3d_cpp_runtime`.
- Preserve all public signatures and data-member layout.

**Step 4: Verify**

```bash
mise run build
mise run authoring-footprint
ctest --test-dir build -R 'x3d_(codec|proto|sdk_facade|install_embed)' \
  --output-on-failure
mise run golden
```

**Step 5: Benchmark and commit**

```bash
git add runtime CMakeLists.txt
git commit -m "build(runtime): compile document and proto bodies once"
```

### Task 15: Split extraction leaf algorithms

**Files:**
- Create `.cpp` peers for measured non-template bodies in:
  - `runtime/extract/MaterialSystem.hpp`
  - `runtime/extract/TextureExtract.hpp`
  - `runtime/extract/TextLayout.hpp`
  - `runtime/extract/TextExtract.hpp`
  - `runtime/extract/LightSystem.hpp`
  - `runtime/extract/NurbsEval.hpp`
- Modify: `CMakeLists.txt`
- Modify: `runtime/extract/tests/*`

**Step 1: Add direct link-level tests for each selected public function**

Ensure the tests use declarations from installed-style headers and resolve implementations from `x3d_cpp_runtime`.

**Step 2: Move only measured non-template algorithms**

Keep POD descriptors, seam callbacks, templates, and small math functions inline. Put large tables and reflection walks into `.cpp` anonymous namespaces.

**Step 3: Verify**

```bash
cmake --build build --target x3d_extract_tests -j 16
ctest --test-dir build -R x3d_extract_tests --output-on-failure
scripts/validate-examples.sh
```

**Step 4: Benchmark and commit**

```bash
git add runtime/extract CMakeLists.txt
git commit -m "build(extract): compile shared extraction algorithms once"
```

### Task 16: Split MeshBuilder

**Files:**
- Create: `runtime/extract/MeshBuilder.cpp`
- Modify: `runtime/extract/MeshBuilder.hpp`
- Modify: `CMakeLists.txt`
- Modify mesh-builder tests under `runtime/extract/tests/`

**Step 1: Add a public API/link test**

Exercise analytic, indexed, text, and NURBS branches via `buildLocalMesh`, including `MeshBuildOptions` seams. Verify the test fails to link without `x3d_cpp_runtime`.

**Step 2: Move implementation**

- Keep `GeoSystemDesc`, `GeoProjection`, `MeshBuildOptions`, and public result types in the header.
- Move `mesh_detail` non-template functions and `buildLocalMesh` dispatch/algorithms into the `.cpp`.
- Keep genuinely generic templates in the header or explicitly instantiate only when the supported type set is closed and tested.
- Do not alter tessellation output or floating-point order without updating an explicit behavior test first.

**Step 3: Verify all mesh branches**

```bash
cmake --build build --target x3d_extract_tests -j 16
ctest --test-dir build -R x3d_extract_tests --output-on-failure
cmake -S . -B build-cpuraster -G Ninja \
  -DX3D_CPP_BUILD_CPURASTER=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-cpuraster --target x3d_cpu_raster -j 16
ctest --test-dir build-cpuraster -R x3d_cpuraster --output-on-failure
```

**Step 4: Benchmark and commit**

MeshBuilder is a major fan-out header; require a clear target-group improvement.

```bash
git add runtime/extract/MeshBuilder.hpp runtime/extract/MeshBuilder.cpp \
  runtime/extract/tests CMakeLists.txt
git commit -m "build(extract): compile MeshBuilder once"
```

### Task 17: Split SceneExtractor

**Files:**
- Create: `runtime/extract/SceneExtractor.cpp`
- Modify: `runtime/extract/SceneExtractor.hpp`
- Modify: `CMakeLists.txt`
- Modify scene-extractor tests

**Step 1: Add source/ABI contract tests**

Compile consumers that construct `SceneExtractor`, call `fullSnapshot`, `delta`, camera/lights/background accessors, and inspect public result types. Record `sizeof(SceneExtractor)` only as a diagnostic; do not make it a stable ABI assertion unless the project explicitly adopts ABI stability.

**Step 2: Move method bodies without PIMPL**

Start with out-of-class definitions so source layout and ownership remain obvious. Move traversal helpers and large private algorithms to the `.cpp` anonymous namespace where possible.

If heavy private includes remain dominant after measurement, prepare a separate reviewed PIMPL follow-up; do not combine that ABI/layout change into this task.

**Step 3: Verify**

```bash
cmake --build build --target x3d_extract_tests x3d_events_tests \
  x3d_example_02_extract_render_feed -j 16
ctest --test-dir build -R 'x3d_(extract|events|example_02)' --output-on-failure
scripts/validate-examples.sh
```

**Step 4: Benchmark and commit**

```bash
git add runtime/extract/SceneExtractor.hpp runtime/extract/SceneExtractor.cpp \
  runtime/extract/tests CMakeLists.txt
git commit -m "build(extract): compile SceneExtractor once"
```

### Task 18: Split scene systems

**Files:**
- Create `.cpp` peers for measured bodies in `TransformSystem`, `BoundsSystem`, `PickSystem`, `ViewDependentSystem`, and related scene helpers
- Modify: `CMakeLists.txt`
- Modify: `runtime/scene/tests/*`

**Step 1: Add link-boundary tests**

Cover transform indexing/propagation, bounds cycles/shared subgraphs, picking analytic and mesh geometry, and view-dependent selection.

**Step 2: Move non-template class methods**

Template child walkers remain in headers. Reflection traversal, narrow-phase dispatch, and recomputation bodies move to `.cpp`.

**Step 3: Verify**

```bash
cmake --build build --target x3d_geometry_scene_tests x3d_events_tests -j 16
ctest --test-dir build -R 'x3d_(geometry_scene|events_tests)' --output-on-failure
```

**Step 4: Benchmark and commit**

```bash
git add runtime/scene CMakeLists.txt
git commit -m "build(scene): compile scene-system implementations once"
```

### Task 19: Split event and execution systems

**Files:**
- Create `.cpp` peers for measured bodies in:
  - `X3DEventCascade`
  - `X3DEventGraph`
  - `X3DExecutionContext`
  - `X3DSceneBridge`
  - high-fan-out interaction/animation systems selected by profiling
- Modify: `CMakeLists.txt`
- Modify: `runtime/events/tests/*`

**Step 1: Add link-boundary coverage**

Cover event routing, cascade caps, scene bridge construction, tick sequencing, dynamic fields, pointing/navigation, interpolation/followers, and lifecycle behavior.

**Step 2: Move non-template methods**

Keep abstract `System` interfaces, small event-address values, callback types, and template walkers in headers. Move queues, reflection dispatch, state-machine updates, and registration tables to `.cpp`.

**Step 3: Verify normal and sanitized behavior**

```bash
cmake --build build --target x3d_events_tests x3d_script_system \
  x3d_sim_behavior_test -j 16
ctest --test-dir build -R 'x3d_(events|script|sim_behavior)' --output-on-failure
mise run build-san
```

**Step 4: Benchmark and commit**

```bash
git add runtime/events CMakeLists.txt
git commit -m "build(events): compile execution-system implementations once"
```

### Task 20: Slim public includes after the definition split

**Files:**
- Modify migrated runtime headers
- Modify their `.cpp` files
- Modify tests/consumers that relied on accidental transitive includes

**Step 1: Add explicit-include contract cases**

Use `x3d_header_isolation` as the main regression. For any public header being slimmed, add a minimal downstream consumer only when the intended dependency is not already covered by isolation.

**Step 2: Move implementation-only includes**

Relocate node-specific, parser-state, unordered-container, algorithm, and stream headers from public headers into `.cpp` files when declarations do not require them. Replace with forward declarations only when legal and readable.

Do not remove a transitive include merely to satisfy a metric; every removal must correspond to a real declaration/implementation boundary.

**Step 3: Fix consumers correctly**

If a consumer used a type without including its declaring header, add the explicit include. Do not restore implementation includes to preserve accidental transitivity.

**Step 4: Verify**

```bash
cmake --build build-ci --target x3d_header_isolation -j 16
ctest --test-dir build-ci -R '^x3d_header_isolation$' --output-on-failure
mise run build
ctest --test-dir build -R x3d_install_embed_smoke --output-on-failure
```

**Step 5: Benchmark and commit**

```bash
git add runtime tests examples tools
git commit -m "build: remove implementation-only public includes"
```

### Task 21: Remove anchor sources and validate installed/static-link behavior

**Files:**
- Delete: `runtime/compiled/AuthoringAnchor.cpp`
- Delete: `runtime/compiled/RuntimeAnchor.cpp`
- Modify: `CMakeLists.txt`
- Modify install/embed fixtures as needed

**Step 1: Extend topology tests**

Assert both compiled runtime libraries contain real subsystem objects and no anchor symbol. Assert static-link order works for GCC, Clang, and the installed CMake package.

**Step 2: Remove anchors and verify**

```bash
cmake -S . -B /tmp/x3d-final-topology -G Ninja \
  -DX3D_CPP_PER_HEADER_CHECKS=OFF
cmake --build /tmp/x3d-final-topology --target x3d_behavior_tests -j 16
ctest --test-dir /tmp/x3d-final-topology -L behavior \
  --output-on-failure -j 16
ctest --test-dir /tmp/x3d-final-topology -R x3d_install_embed_smoke \
  --output-on-failure
mise run authoring-footprint
```

**Step 3: Commit**

```bash
git add CMakeLists.txt runtime/compiled tests/test_target_topology.py
git commit -m "build: finalize compiled runtime libraries"
```

### Task 22: Final benchmark, documentation, and CI validation

**Files:**
- Modify: `docs/wiki/architecture.md`
- Modify: `docs/wiki/guides/build-and-mise.md`
- Modify subsystem pages cited by `mise run docs-drift working`
- Modify: `README.md` if public linking instructions changed

**Step 1: Run complete verification**

```bash
uv run pytest -q --durations=30
mise run golden
mise run conformance-gate
mise run coverage-gate
mise run doc-ctest-gate
mise run build
mise run build-ci
mise run build-san
mise run build-fuzz
mise run authoring-footprint
scripts/validate-examples.sh
```

Run every available seam swap job locally where its dependency is already installed. Do not fetch large optional dependencies solely for this task without approval.

**Step 2: Capture final cold measurements**

Use identical phase-0 settings and write ignored JSON for dev, CI behavior, header isolation, and sanitizer builds.

Acceptance targets on the reference 16-core machine:

- default cold build user CPU: reduce from 1081.6 s to at most 650 s;
- default cold wall: reduce from 80.65 s to at most 55 s;
- grouped doctest cumulative compiler time: reduce from roughly 656 s to at most 350 s;
- header contract: one target/test, one TU per header;
- namespace Python tests: no compiler subprocess;
- uncached GitHub sanitizer critical path: target under 15 minutes.

If a target is missed, report the measured result and remaining dominant TUs; do not massage cache state or parallelism to claim success.

**Step 3: Update architecture and build docs**

Document the installed target graph, which implementations remain intentionally header-only, exact measured timings/counts, and the target taxonomy. Explain the authoring/full-runtime split and retain links to the design document.

**Step 4: Run documentation gates**

```bash
mise run docs-drift working
mise run docs-build
```

**Step 5: Validate on GitHub Actions**

Push only after local gates pass. Inspect a genuinely cold PR run and a follow-up warm run. Record per-job durations for Python, behavior, headers, sanitizer, fuzz, and examples in the PR body.

**Step 6: Commit**

```bash
git add docs/wiki docs/sdk README.md
git commit -m "docs(build): record compiled runtime architecture and results"
```

## Final review checklist

- [ ] Every repository-owned target has a validated purpose.
- [ ] Default builds contain no external-corpus-only gate executable.
- [ ] Every normal CTest executable belongs to `x3d_behavior_tests`.
- [ ] Every sanitizer-executed binary belongs to `x3d_sanitizer_tests`.
- [ ] Header isolation is one object target/test and still one TU per header.
- [ ] Python namespace tests do not compile generated C++.
- [ ] Installed SDK and authoring consumers link through exported targets.
- [ ] Authoring footprint remains slim.
- [ ] Generated output is unchanged unless separately justified.
- [ ] Each phase-two wave has before/after cold measurements.
- [ ] Global unity was not enabled for hand-written tests.
- [ ] Templates and compile-time APIs remain in headers.
- [ ] Documentation states current measured facts, not historical estimates.

# Smaller CI Behavior Builds Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Preserve the complete behavior and sanitizer suites while materially reducing their compiler-command count and live build-tree footprint.

**Architecture:** CMake will unity-batch only the five existing multi-source doctest executables, capped at eight sources per generated translation unit and protected by per-source unique identifiers. The sanitizer configuration remains Debug ASan/UBSan but overrides full debug records with level-one debug information suitable for symbolized CI reports.

**Tech Stack:** CMake 3.21+, Ninja, GCC, CTest, pytest, ASan, UBSan, ccache.

---

### Task 1: Unity-batch the grouped behavior suites

**Files:**
- Modify: `tests/test_header_contract_shape.py`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing shape test**

Add a test that parses `CMakeLists.txt` and requires this exact grouped target set:

```python
GROUPED_BEHAVIOR_TARGETS = {
    "x3d_geometry_scene_tests",
    "x3d_codecs_tests",
    "x3d_parse_tests",
    "x3d_extract_tests",
    "x3d_events_tests",
}
```

For every target, require `UNITY_BUILD ON`, `UNITY_BUILD_BATCH_SIZE 8`, and a
non-empty `UNITY_BUILD_UNIQUE_ID`. Also assert that the global
`CMAKE_UNITY_BUILD` switch is not enabled.

**Step 2: Run the test to verify it fails**

Run:

```bash
UV_CACHE_DIR=/tmp/x3d-uv-cache .venv/bin/pytest tests/test_header_contract_shape.py -q
```

Expected: FAIL because the five grouped test targets have no unity properties.

**Step 3: Add the minimal target properties**

After all five grouped targets have been declared, add:

```cmake
set_target_properties(
    x3d_geometry_scene_tests
    x3d_codecs_tests
    x3d_parse_tests
    x3d_extract_tests
    x3d_events_tests
    PROPERTIES
        UNITY_BUILD ON
        UNITY_BUILD_BATCH_SIZE 8
        UNITY_BUILD_UNIQUE_ID X3D_TEST_UNITY_SOURCE_ID)
```

Do not change source lists, executables, CTest names, labels, or production
targets.

**Step 4: Run the shape test and configure**

Run the pytest command from Step 2, then:

```bash
cmake --preset ci
```

Expected: pytest PASS and CMake configure/generate PASS.

**Step 5: Cold-build and measure the normal behavior lane**

Clean the existing CI preset, reset ccache statistics, and build with ccache
disabled:

```bash
cmake --build --preset ci --target clean
ccache --zero-stats
CCACHE_DISABLE=1 /usr/bin/time -v cmake --build --preset ci --target x3d_behavior_tests -j 16
ctest --preset ci -L behavior --output-on-failure -j 16
ninja -C build-ci -t commands x3d_behavior_tests
du -sh build-ci
```

Expected: all 45 behavior CTests PASS, compile-command count is materially below
230, and the live tree is materially below 3.4 GB.

**Step 6: Commit**

```bash
git add CMakeLists.txt tests/test_header_contract_shape.py
git commit -m "build: unity-batch grouped behavior tests"
```

### Task 2: Bound sanitizer debug information and footprint

**Files:**
- Modify: `tests/test_cmake_aggregates.py`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing sanitizer flag test**

Extend the sanitizer configure regression to inspect a sanitizer compile command
and require `-g1` after the build-type `-g` flag. Continue requiring Debug,
ASan/UBSan, behavior-only aggregation, and disabled per-header checks.

**Step 2: Run the test to verify it fails**

Run:

```bash
UV_CACHE_DIR=/tmp/x3d-uv-cache .venv/bin/pytest tests/test_cmake_aggregates.py -q
```

Expected: FAIL because sanitizer project compilation does not yet request
minimal debug records.

**Step 3: Add the sanitizer-only compile flag**

In the existing `X3D_CPP_SAN` block, append the minimal debug level after the
sanitizer flags:

```cmake
add_compile_options(-g1)
```

Keep `CMAKE_BUILD_TYPE=Debug`, frame pointers, ASan, UBSan, and
`-fno-sanitize-recover=all` unchanged.

**Step 4: Run the targeted tests**

Run:

```bash
UV_CACHE_DIR=/tmp/x3d-uv-cache .venv/bin/pytest tests/test_cmake_aggregates.py tests/test_header_contract_shape.py -q
```

Expected: PASS.

**Step 5: Cold-build and measure the sanitizer lane**

Configure a fresh sanitizer tree and build with ccache disabled:

```bash
cmake --preset san
ccache --zero-stats
CCACHE_DISABLE=1 /usr/bin/time -v cmake --build --preset san --target x3d_sanitizer_tests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --preset san -L behavior --output-on-failure -j 16
du -sh build-san
```

Expected: all 45 tests PASS under ASan/UBSan. Leak detection is disabled only
for local execution because LeakSanitizer cannot operate under this runner's
ptrace wrapper; GitHub Actions retains leak detection. Target live-tree size is
at most 10 GB versus the approximately 24 GB baseline.

**Step 6: Commit**

```bash
git add CMakeLists.txt tests/test_cmake_aggregates.py
git commit -m "build(san): minimize CI debug information"
```

### Task 3: Re-run the split CI lane contract

**Files:**
- Modify: `.github/workflows/ci.yml` (already prepared by phase-one Task 9)
- Modify: `CMakePresets.json` (already prepared by phase-one Task 9)
- Modify: `mise.toml` (already prepared by phase-one Task 9)
- Modify: `tests/test_header_contract_shape.py` (already prepared by phase-one Task 9)

**Step 1: Build and run the behavior lane**

```bash
cmake --build --preset ci --target x3d_behavior_tests -j 16
ctest --preset ci -L behavior --output-on-failure -j 16
```

Expected: PASS.

**Step 2: Build and run the compile-contract lane**

```bash
cmake --build --preset ci --target x3d_compile_contracts -j 16
ctest --preset ci -L compile-contract --output-on-failure -j 16
```

Expected: PASS, including header isolation, all-headers, and installed-consumer
coverage.

**Step 3: Run meta-tests and validate the workflow**

```bash
UV_CACHE_DIR=/tmp/x3d-uv-cache .venv/bin/pytest tests/test_header_contract_shape.py tests/test_cmake_aggregates.py tests/test_classify_ci_changes.py -q
git diff --check
```

Parse `.github/workflows/ci.yml` with PyYAML and verify the change classifier's
generator-only and manual-dispatch outputs.

Expected: all checks PASS.

**Step 4: Commit**

```bash
git add .github/workflows/ci.yml CMakePresets.json mise.toml tests/test_header_contract_shape.py
git commit -m "ci: split behavior and header compile contracts"
```

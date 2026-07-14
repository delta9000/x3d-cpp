# Shared Generated-Node Runtime Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Link one generated-node runtime into all consumers by default, retain an explicitly tested static mode, and reduce sanitizer CI storage below 10 GB.

**Architecture:** `x3d_cpp_nodes` selects `SHARED` from a project-specific default-on option and `STATIC` from its opt-out, independent of third-party `BUILD_SHARED_LIBS` settings. Its public target and aliases stay stable; installation exports all artifact kinds, and the generated standalone test source is filtered out before target creation.

**Tech Stack:** CMake 3.20+, Ninja, GCC, CTest, pytest, ASan, UBSan.

---

### Task 1: Pin shared and static target shape

**Files:**
- Modify: `tests/test_cmake_aggregates.py`

**Step 1: Write failing shared-default assertions**

Extend the configured CI test to require `X3D_CPP_SHARED_NODES:BOOL=ON`, a
`libx3d_cpp_nodes.so` target, no `libx3d_cpp_nodes.a` target, and no compile
command whose source is `generated_cpp_bindings/x3d/nodes/test.cpp`.

**Step 2: Write a failing static-opt-out test**

Configure a temporary tree with `-DX3D_CPP_SHARED_NODES=OFF` and require an
archive target rather than a shared-library target.

**Step 3: Run the focused tests**

```bash
UV_CACHE_DIR=/tmp/x3d-uv-cache .venv/bin/pytest tests/test_cmake_aggregates.py -q
```

Expected: FAIL because the current target is unconditionally static and still
contains `nodes/test.cpp`.

### Task 2: Implement the selectable node library

**Files:**
- Modify: `CMakeLists.txt`

**Step 1: Add the project option and source filter**

Add `X3D_CPP_SHARED_NODES` with default `ON`, map it to an internal `SHARED` or
`STATIC` type variable, and remove the exact generated test source from
`X3D_CPP_NODE_SOURCES` before `add_library`.

**Step 2: Configure shared-library properties**

For shared mode, set `VERSION` to `${PROJECT_VERSION}`, `SOVERSION` to
`${PROJECT_VERSION_MAJOR}`, and `WINDOWS_EXPORT_ALL_SYMBOLS ON`. Keep existing
unity batching, aliases, include usage, compile features, and transitive linkage.

**Step 3: Install every artifact kind**

Extend the existing target install rule with `LIBRARY` and `RUNTIME`
destinations while retaining the archive destination for static mode and import
libraries.

**Step 4: Run the focused tests**

Run the pytest command from Task 1.

Expected: PASS.

**Step 5: Commit the target topology**

```bash
git add CMakeLists.txt tests/test_cmake_aggregates.py
git commit -m "build: share the generated node runtime by default"
```

### Task 3: Verify consumers and measure CI

**Files:**
- Modify: `docs/plans/2026-07-13-ci-test-unity-design.md`
- Modify: `docs/plans/2026-07-13-ci-test-unity.md`
- Modify: current architecture/ADR documentation that calls the target static-only

**Step 1: Verify normal shared behavior**

```bash
cmake --preset ci
cmake --build --preset ci --target x3d_behavior_tests -j 16
ctest --preset ci -L behavior --output-on-failure -j 16
ctest --preset ci -R x3d_install_embed_smoke --output-on-failure
```

Expected: all behavior and installed-consumer tests PASS.

**Step 2: Verify static installed consumption**

Configure a temporary CI tree with `-DX3D_CPP_SHARED_NODES=OFF`, build
`x3d_cpp_nodes`, and run `scripts/verify_install_embed.sh` against it.

Expected: the downstream product builds, links, and executes with the archive.

**Step 3: Cold-build and measure sanitizer CI**

Clean and reconfigure the sanitizer preset, then build with ccache disabled:

```bash
cmake --build --preset san --target clean
cmake --preset san
CCACHE_DISABLE=1 /usr/bin/time -v cmake --build --preset san --target x3d_sanitizer_tests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --preset san -L behavior --output-on-failure -j 16
du -sh build-san
```

Expected: 45/45 sanitizer behavior tests PASS and the live tree is below 10 GB,
versus 22 GB with `-g1` static linkage and approximately 24 GB originally.

**Step 4: Complete CI split verification**

Build and run `x3d_compile_contracts` / the `compile-contract` label, run the
CMake/workflow/classifier pytest files, parse the workflow with PyYAML, and run
`git diff --check`.

**Step 5: Update documentation and commit**

Record the rejected test-unity experiment, the shared default/static opt-out,
and measured normal/sanitizer footprints. Supersede static-only wording without
discarding the original C1 performance history.

```bash
git add docs CMakePresets.json mise.toml .github/workflows/ci.yml tests/test_header_contract_shape.py
git commit -m "ci: split behavior and header compile contracts"
```

# Cleanup & Correctness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix a set of packaging/CLI/gate correctness bugs and bring the public-facing surface (naming, claims, install path, CI) in line with what the code actually does — without changing the architecture.

**Architecture:** Four PR-sized units, each independently shippable and independently reviewable. They are ordered by dependency, not by importance.

| PR | Tasks | Surface |
|---|---|---|
| **PR 1** | 1-5 | Correctness and reproducibility: CMake package metadata + ABI, generator CLI, fail-closed verification, golden wording, formatter pin |
| **PR 2** | 6-10 | Identity, claims and packaging: naming split, untrue claims, docs sync, install path, Python package |
| **PR 3** | 11-12 | CMake downstream behavior: dev-tooling isolation, installed-target contract |
| **PR 4** | 13-15 | Public repository surface: docs nav, maintainer files, CI |

**Do not combine PR 1 and PR 2.** That would be ten tasks spanning CMake packaging, CLI behavior, codegen, Python packaging and public documentation — too much conceptual surface for one review even with clean commits.

**The v0.1.0 release is a post-merge operation, not part of any of these PRs.** It is written up at the end of this plan, after Task 15.

**Tech Stack:** CMake 3.21+, C++20, Python 3.11+ (argparse/hatchling/pytest), mise, uv, clang-format 22.1.8, GitHub Actions, MkDocs Material.

## Global Constraints

- **Project version is `0.1.0`** (`CMakeLists.txt:3`). Do not bump it in this work.
- **Naming model, applied verbatim:** product/repository = `x3d-cpp`; Python generator package + console script = `x3d-cpp-gen`; CMake project + export namespace = `x3d_cpp`. Occurrences of `x3d-cpp-gen` may survive **only** where they mean the Python package, the executable, or the generator subsystem.
- **Real repository is `delta9000/x3d-cpp`.** `mkdocs.yml:4` currently points at `sandbrookvt/x3d-cpp-gen` — wrong owner *and* wrong name.
- **`docs/superpowers/` is the historical record — cite it, never edit it.** New plans live in `docs/plans/` (this file).
- **Do not put `Claude-Session:` trailers or `claude.ai/code/session_…` URLs in commits or PR bodies.**
- **Docs are part of the diff.** Run `mise run docs-drift working` before committing code changes under `runtime/`, `tools/`, `include/x3d/`.
- **Never hand-edit generated files:** `docs/conformance/*.md` (edit `findings.yaml`, run `mise run conformance`); `generated_cpp_bindings/` (run `mise run gen`).
- **`mise run gen` leaves a stray gitignored `generated_cpp_bindings/x3d/nodes/test.cpp`** that breaks `build-ci` with `-Werror=unused-function`. `rm` it after regenerating.
- **The golden gate must stay byte-identical.** Any task that touches the emitter or the formatter must end with `scripts/check_golden.sh` passing.
- **Hard gate:** `mise run ci` = tests + golden + conformance-gate + build + cli-gate-regression. The wiki's strict gate is `mise run docs-build`.

---

## Corrections to the incoming review (read before starting)

The review is accurate on most points. Verified against the code on 2026-07-14, these items differ — **do not implement them as written**:

1. **`.github/ISSUE_TEMPLATE/` is NOT absent.** It exists with `work-item.yml` and `config.yml`. Only `CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`, `CHANGELOG.md`, and `.github/PULL_REQUEST_TEMPLATE.md` are genuinely missing. (Task 14.)
2. **"Capability matrix says MovieTexture frames are deferred; MovieDecoder and MovieTexture playback are merged" is FALSE as stated.** Movie *decode* ships (`runtime/io/plmpeg/`, `runtime/io/theora/`, seam `runtime/extract/MovieDecoder.hpp`), but *playback* genuinely does not: `runtime/extract/MovieDecoder.hpp:12-13` — "the SDK never invokes it and never opens a media file"; `runtime/extract/RenderItem.hpp:255` — "descriptor-only". So `docs/sdk/v1-capabilities.md:63` is defensible. The real defect is the **opposite** of the allegation: `v1-capabilities.md` never mentions the MovieDecoder seam at all, so it *undersells* shipped work. (Task 8.)
3. **The installed imported-target set is 7, not 6.** The review's list omits `x3d_cpp::x3d_cpp` (the bare facade, installed at `CMakeLists.txt:306`, the only installed target with no explicit `EXPORT_NAME`). (Task 12.)
4. **Wiki "renderers are not part of this repo" is a wording bug only.** The architectural claim is sound and build-flag-enforced; `examples/*/README.md` already phrase it correctly as "out-of-SDK consumer". Fix the word, not the architecture. (Task 8.)
5. **Wiki "X3D 4.0" vs README "3.0–4.1" is not simply wrong.** Both are true on different axes: bindings are generated from the **4.0 UOM**; the parser reads **3.0–4.1 encodings**. Nothing documents the distinction. Document it rather than picking a side. (Task 8.)

Two traps the review did not anticipate, both load-bearing:

6. **Adding a `.clang-format` naively will break the golden gate permanently.** `src/x3d_cpp_gen/backends/cpp_header.py:212` runs `clang-format -i` with **no `--style` flag**, so clang-format searches upward from each output file. The committed golden is written inside the repo (finds a new `.clang-format`); the golden gate regenerates into `mktemp -d` under `/tmp` (does **not** find it, silently falls back to built-in LLVM style). The two would format differently forever. Task 5 fixes this with `--style=file:<abs path>`.
7. **`cmake_minimum_required(VERSION 3.20)` but `PROJECT_IS_TOP_LEVEL` requires CMake 3.21.** `CMakeLists.txt:407` already uses it, so on CMake 3.20 it silently evaluates empty → `X3D_CPP_BUILD_TESTS` defaults OFF. Task 11 bumps the floor to 3.21.

## What actually leaks to a consumer (measured, not assumed)

An earlier draft of Task 11 asserted that `add_compile_options()`/`add_link_options()` leak upward into targets the parent defines after `add_subdirectory()`. **They do not.** Both are scoped to the current directory *and below*; a child's directory properties never propagate to the parent. Measured on 2026-07-14 with a synthetic parent/child fixture:

```
PARENT_COMPILE_OPTIONS=[]            <- child's add_compile_options(-Wall -Wextra): NO leak
PARENT_LINK_OPTIONS=[]               <- child's add_link_options(-fuse-ld=mold):    NO leak
PARENT_TARGET_COMPILE_OPTIONS=[NOTFOUND]  <- a parent-defined target is unaffected
PARENT_LAUNCHER=[ccache]             <- child's set(... CACHE STRING): LEAKS
```

So the real defects are narrower than the review claimed, and the fixture must target them precisely:

- **`set(CMAKE_CXX_COMPILER_LAUNCHER ... CACHE STRING)` (`CMakeLists.txt:19-23`) genuinely leaks** — it writes the shared cache, so it becomes the default for every target in the whole build tree, including the consumer's. This is the one confirmed cross-project mutation.
- **`set_property(GLOBAL PROPERTY JOB_POOLS ...)` (`CMakeLists.txt:57`) is global by definition** and caps the consumer's build too.
- **Choosing mold/lld and `-Wall -Wextra` for *our own* targets is defensible**, but a dependency silently selecting a linker is still a surprise worth gating.

Consequence for the fixture: **do not** assert on the parent's directory properties — that assertion passes before the fix and is therefore worthless. Assert on cache variables and on a parent-defined target's own properties. See Task 11.

---

# PR 1 — Correctness and reproducibility (Tasks 1-5)

> Ships alone. Do not fold PR 2 into this branch.

### Task 1: CMake package compatibility metadata

The installed package ships compiled shared libraries but its version file declares `ARCH_INDEPENDENT` (which deletes the pointer-size check entirely) and `SameMajorVersion` (which, at major `0`, makes every future 0.x look compatible).

**Files:**
- Modify: `CMakeLists.txt:388-392` (the version file)
- Modify: `CMakeLists.txt:204,254` (`SOVERSION`)
- Create: `tests/cmake/package_version/check_version_file.cmake`
- Modify: `CMakeLists.txt` (register the ctest, next to `x3d_install_embed_smoke` at ~line 1799)

**Interfaces:**
- Consumes: nothing.
- Produces: ctest `x3d_package_version_contract`. The generated `x3d_cppConfigVersion.cmake` gains the pointer-size guard and minor-version pinning.

**Background — what these knobs actually do** (verified in `/usr/share/cmake/Modules/`):
- `ARCH_INDEPENDENT` omits the `CMAKE_SIZEOF_VOID_P` block from the generated file. Without it, CMake bakes the build machine's pointer size in and sets `PACKAGE_VERSION_UNSUITABLE` on mismatch. It exists for header-only/pure-data packages.
- `SameMinorVersion` requires installed ≥ requested **and** major+minor to match.
- `ExactVersion` is implemented via the `SamePatchVersion` template (major+minor+patch must match; tweak stripped) — it is not a string compare.

Decision: **`SameMinorVersion`**. `ExactVersion` would make every 0.1.x patch release a breaking change for pinned consumers without being meaningfully safer, *provided* patch releases hold ABI. That proviso is not free — see Step 7, which makes the soname track the same axis, and Task 14, which scopes the written promise to "intended" because no ABI checker exists yet.

- [ ] **Step 1: Write the failing test**

Create `tests/cmake/package_version/check_version_file.cmake`. This is hermetic and needs no cross-compiler: the generated version file is a plain CMake script that reads `PACKAGE_FIND_VERSION*` and `CMAKE_SIZEOF_VOID_P`, so drive it directly in `cmake -P` script mode.

```cmake
# Contract test for the generated x3d_cppConfigVersion.cmake.
# Invoked as: cmake -DVERSION_FILE=<path> -P check_version_file.cmake
#
# The version file is a script that consumes PACKAGE_FIND_VERSION{,_MAJOR,_MINOR}
# and CMAKE_SIZEOF_VOID_P, and answers via PACKAGE_VERSION_COMPATIBLE /
# PACKAGE_VERSION_UNSUITABLE. Each case runs in a child scope so the variables
# the file sets cannot leak between cases.

if(NOT DEFINED VERSION_FILE)
  message(FATAL_ERROR "check_version_file.cmake: -DVERSION_FILE=<path> is required")
endif()
if(NOT EXISTS "${VERSION_FILE}")
  message(FATAL_ERROR "check_version_file.cmake: no such file: ${VERSION_FILE}")
endif()

set(_failures "")

if(NOT DEFINED BUILD_SIZEOF_VOID_P)
  message(FATAL_ERROR "check_version_file.cmake: -DBUILD_SIZEOF_VOID_P=<n> is required")
endif()
# The "matching" arch is whatever this build actually is -- do not hardcode 8.
# A 32-bit build must still see its own pointer size accepted, and the mismatch
# case must be the OTHER size, whichever that is.
set(_match_void_p "${BUILD_SIZEOF_VOID_P}")
if(_match_void_p EQUAL 8)
  set(_mismatch_void_p 4)
else()
  set(_mismatch_void_p 8)
endif()

function(_probe out_compatible out_unsuitable find_version major minor sizeof_void_p)
  set(PACKAGE_FIND_VERSION "${find_version}")
  set(PACKAGE_FIND_VERSION_MAJOR "${major}")
  set(PACKAGE_FIND_VERSION_MINOR "${minor}")
  set(CMAKE_SIZEOF_VOID_P "${sizeof_void_p}")
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
  set(PACKAGE_VERSION_UNSUITABLE FALSE)
  include("${VERSION_FILE}")
  set(${out_compatible} "${PACKAGE_VERSION_COMPATIBLE}" PARENT_SCOPE)
  set(${out_unsuitable} "${PACKAGE_VERSION_UNSUITABLE}" PARENT_SCOPE)
endfunction()

# Case 1: same minor, matching arch -> compatible.
_probe(c u "0.1.0" 0 1 "${_match_void_p}")
if(NOT c)
  list(APPEND _failures "0.1.0 request should be COMPATIBLE with installed 0.1.0")
endif()
if(u)
  list(APPEND _failures "0.1.0 request on the build's own arch should NOT be UNSUITABLE")
endif()

# Case 2: next minor -> NOT compatible (this is the SameMajorVersion bug).
_probe(c u "0.2.0" 0 2 "${_match_void_p}")
if(c)
  list(APPEND _failures "0.2.0 request must NOT be compatible with installed 0.1.0")
endif()

# Case 3: earlier minor -> NOT compatible.
_probe(c u "0.0.9" 0 0 "${_match_void_p}")
if(c)
  list(APPEND _failures "0.0.9 request must NOT be compatible with installed 0.1.0")
endif()

# Case 4: mismatched pointer size -> UNSUITABLE (this is the ARCH_INDEPENDENT bug).
_probe(c u "0.1.0" 0 1 "${_mismatch_void_p}")
if(NOT u)
  list(APPEND _failures
       "a consumer with pointer size ${_mismatch_void_p} must be UNSUITABLE for a "
       "package built with pointer size ${_match_void_p} (ARCH_INDEPENDENT must not "
       "be set on a package shipping .so files)")
endif()

if(_failures)
  string(REPLACE ";" "\n  - " _msg "${_failures}")
  message(FATAL_ERROR "package version contract FAILED:\n  - ${_msg}")
endif()
message(STATUS "package version contract OK (SameMinorVersion + arch check present)")
```

- [ ] **Step 2: Register the ctest**

In `CMakeLists.txt`, beside the existing `x3d_install_embed_smoke` registration (~line 1799), add:

```cmake
# Contract: the generated package version file must reject a mismatched
# pointer size and must not treat a different MINOR as compatible (the
# package ships compiled .so files, so it is not arch-independent).
add_test(NAME x3d_package_version_contract
         COMMAND "${CMAKE_COMMAND}"
                 "-DVERSION_FILE=${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfigVersion.cmake"
                 "-DBUILD_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P}"
                 -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/package_version/check_version_file.cmake")
```

- [ ] **Step 3: Run the test to verify it FAILS against the current metadata**

```bash
cmake --preset dev && ctest --preset dev -R x3d_package_version_contract --output-on-failure
```

Expected: FAIL. Case 2 and Case 4 both trip — `0.2.0 request must NOT be compatible` (SameMajorVersion at major 0) and `32-bit consumer must be UNSUITABLE` (ARCH_INDEPENDENT).

- [ ] **Step 4: Apply the fix**

`CMakeLists.txt:388-392` — replace:

```cmake
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)
```

with:

```cmake
# The package installs compiled shared libraries, so it is NOT arch-independent:
# ARCH_INDEPENDENT would delete the CMAKE_SIZEOF_VOID_P guard and let a consumer
# with a different pointer size link a mismatched ABI. SameMinorVersion (not
# SameMajorVersion) because PROJECT_VERSION is 0.x: at major 0 SameMajorVersion
# would treat every future 0.y as compatible. Contract: tests/cmake/package_version/.
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/x3d_cppConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)
```

- [ ] **Step 5: Run the test to verify it PASSES**

```bash
cmake --preset dev && ctest --preset dev -R x3d_package_version_contract --output-on-failure
```

Expected: PASS, `1 tests passed, 0 tests failed`.

- [ ] **Step 6: Verify the real consumer still resolves**

```bash
ctest --preset dev -R x3d_install_embed_smoke --output-on-failure
```

Expected: PASS. (`examples/embed_minimal` calls `find_package(x3d_cpp CONFIG REQUIRED)` with no version, which is unaffected by compatibility mode.)

- [ ] **Step 7: Reconcile SOVERSION with the compatibility promise**

`SameMinorVersion` only governs **package selection at configure time**. It does not constrain the **runtime loader**, and it does not prove ABI compatibility — CMake's own docs frame that mode as appropriate *when* compatibility is guaranteed within the minor version. That guarantee has to come from somewhere else, and right now it contradicts itself:

```bash
grep -n 'SOVERSION' CMakeLists.txt
```

Expected: lines 204 and 254 read `SOVERSION "${PROJECT_VERSION_MAJOR}"` — which is **`0` for both 0.1.0 and 0.2.0**. So `find_package` would refuse a 0.2.0 package for a 0.1.0 consumer, while `ld.so` happily loads `libx3d_cpp_runtime.so.0` built from either. The declared promise and the shipped `.so` disagree.

Make the soname track the same axis the package compatibility does. Compute it once, near the top of `CMakeLists.txt` (after `project()`), so both library sites share one rule and the 0.x special case expires by itself:

```cmake
# Pre-1.0 the ABI-breaking axis is the MINOR (SemVer says 0.y is unstable), so
# the soname must track major.minor: a plain ${PROJECT_VERSION_MAJOR} is 0 for
# every 0.x, which would let ld.so load a 0.2 runtime into a 0.1 consumer that
# find_package (SameMinorVersion) had just refused. From 1.0 the major IS the
# ABI axis and the conventional soname is correct -- this branch expires on its
# own at the version bump, with no edit needed.
if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(X3D_CPP_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
else()
    set(X3D_CPP_SOVERSION "${PROJECT_VERSION_MAJOR}")
endif()
```

Then at **both** sites (`CMakeLists.txt:203-204` and `253-254`):

```cmake
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${X3D_CPP_SOVERSION}"
```

- [ ] **Step 8: Verify the soname**

```bash
rm -rf build && cmake --preset dev && cmake --build --preset dev --target x3d_cpp_runtime
objdump -p build/libx3d_cpp_runtime.so* 2>/dev/null | grep SONAME
```

Expected: `SONAME  libx3d_cpp_runtime.so.0.1` (was `.so.0`). Adjust the path/target name to match the actual build output — find it with `find build -name 'libx3d_cpp_runtime.so*'`. If the runtime is built STATIC by default, `X3D_CPP_SHARED_NODES` (`CMakeLists.txt:177`) gates it; configure with it ON for this check.

**Scope the promise honestly.** There is no ABI checker in the tree, so the strongest defensible claim is *intent*, not proof. Task 14's `CONTRIBUTING.md` must say "patch releases are **intended** to preserve API/ABI" — not "do not break ABI" — until a gate exists. File a card: "Add an ABI comparison gate (abidiff/abi-compliance-checker) for the compiled runtime layers".

- [ ] **Step 9: Commit**

```bash
git add CMakeLists.txt tests/cmake/package_version/check_version_file.cmake
git commit -m "fix(cmake): drop ARCH_INDEPENDENT, pin SameMinorVersion, track soname on the minor

The installed package ships compiled shared libraries, so the version file must
keep CMake's pointer-size guard; ARCH_INDEPENDENT deleted it. SameMajorVersion
is also wrong at 0.x, where it makes every future 0.y look compatible.

SOVERSION was ${PROJECT_VERSION_MAJOR} -- 0 for every 0.x -- so ld.so would have
loaded a 0.2 runtime into a 0.1 consumer that find_package had just refused.
Pre-1.0 the ABI-breaking axis is the minor; the soname now says so.

Adds a hermetic contract test that drives the generated version file directly,
deriving the build's own pointer size rather than assuming 64-bit."
```

---

### Task 2: Remove the dead `--namespace` CLI option

`src/x3d_cpp_gen/cli.py:110-115` advertises `--namespace` as controlling the emitted namespace; `cli.py:169` then hardcodes `namespace = "x3d::nodes"` and ignores it.

**Files:**
- Modify: `src/x3d_cpp_gen/cli.py:110-115` (delete the argparse argument)
- Modify: `src/x3d_cpp_gen/cli.py:164-169` (tighten the comment)
- Test: `tests/test_cli_options.py` (create)

**Interfaces:**
- Consumes: nothing.
- Produces: `x3d-cpp-gen --namespace foo` exits non-zero with an argparse "unrecognized arguments" error.

**CRITICAL — scope of the deletion.** Verified: **no test, script, mise task, or CI job passes `--namespace`**. But the `namespace=` *keyword argument* on `generate_cpp_bindings()` is live and must **stay** — `tests/test_version.py:145` calls it with `namespace="x3d::v4_1"` and `tests/test_golden_smoke.py:46` with `namespace="x3d::nodes"`. Delete **only** the argparse option, never the emitter parameter.

- [ ] **Step 1: Write the failing test**

Create `tests/test_cli_options.py`:

```python
"""The generator CLI must not advertise options it ignores.

--namespace was parsed and then overwritten by a hardcoded "x3d::nodes", so
passing it silently did nothing. An ignored compatibility option is worse than
no option: it tells the caller a lie the tool cannot honour.
"""

import subprocess
import sys


def _run(*args):
    return subprocess.run(
        [sys.executable, "-m", "x3d_cpp_gen.cli", *args],
        capture_output=True, text=True,
    )


def test_namespace_option_is_rejected():
    """--namespace must be gone, not silently ignored."""
    result = _run("--namespace", "foo::bar", "--out", "/tmp/should-not-be-written")
    assert result.returncode != 0, (
        "--namespace was accepted; an option the generator ignores must not exist"
    )
    assert "unrecognized arguments" in result.stderr, result.stderr


def test_namespace_absent_from_help():
    result = _run("--help")
    assert result.returncode == 0, result.stderr
    assert "--namespace" not in result.stdout
```

- [ ] **Step 2: Run it to verify it FAILS**

```bash
uv run pytest tests/test_cli_options.py -v
```

Expected: FAIL — `--namespace` is currently accepted, so `returncode` is 0 (or a spec/other error), and `--help` still lists it.

- [ ] **Step 3: Delete the argparse option**

In `src/x3d_cpp_gen/cli.py`, remove this block entirely (lines 110-115):

```python
    parser.add_argument(
        "--namespace", default=None, nargs="?", const="__AUTO__",
        help="Wrap emitted classes in a C++ namespace. Pass a name, or bare "
             "--namespace to auto-derive 'x3d::vX_Y' from the version. Default "
             "OFF, so the 4.0 output stays byte-identical to the golden.",
    )
```

- [ ] **Step 4: Tighten the hardcode comment**

Replace `cli.py:164-169` with:

```python
    # Generated node classes always live in x3d::nodes: the vocabulary
    # value/reflection types are x3d::core and the node classes are x3d::nodes.
    # The layout is fixed so the in-tree and installed include spellings
    # ("x3d/core/...", "x3d/nodes/...") are uniform. generate_cpp_bindings()
    # still takes a `namespace` parameter -- tests/test_version.py exercises it
    # directly -- but the CLI does not expose it, because a CLI option the tool
    # then overrides is a lie to the caller.
    namespace = "x3d::nodes"
```

- [ ] **Step 5: Run the tests to verify they PASS**

```bash
uv run pytest tests/test_cli_options.py -v
```

Expected: PASS, 2 passed.

- [ ] **Step 6: Verify the emitter kwarg is untouched**

```bash
uv run pytest tests/test_version.py tests/test_golden_smoke.py tests/test_nodes_namespace_emit.py -q
```

Expected: PASS. These exercise `namespace=` at the library level; a green run proves only the CLI surface changed.

- [ ] **Step 7: Verify no drift**

```bash
scripts/check_golden.sh
```

Expected: `Golden tree OK: ...`.

- [ ] **Step 8: Commit**

```bash
git add src/x3d_cpp_gen/cli.py tests/test_cli_options.py
git commit -m "fix(gen): remove the dead --namespace CLI option

--namespace was parsed and then overwritten by a hardcoded x3d::nodes, so it
silently did nothing. Delete the option rather than keep an ignored
compatibility flag. The generate_cpp_bindings(namespace=...) parameter is
unaffected and still covered by tests/test_version.py."
```

---

### Task 3: Make generator verification fail closed

`compile_and_run_test()` returns `True` — success — in three cases where verification never happened: empty compiler (`cli.py:39-41`), missing generated `test.cpp` (`cli.py:57-60`), and compiler not on PATH (`cli.py:67-69`). A generator regression that drops `test.cpp` therefore produces a green invocation.

**Files:**
- Modify: `src/x3d_cpp_gen/cli.py:31-88` (`compile_and_run_test`)
- Modify: `src/x3d_cpp_gen/cli.py:125-132` (`--compiler` help)
- Modify: `src/x3d_cpp_gen/cli.py:194-201` (`main` wiring)
- Test: `tests/test_cli_fail_closed.py` (create)

**Interfaces:**
- Consumes: nothing.
- Produces: `compile_and_run_test(test_file_path, output_dir, compiler) -> bool`, which returns `True` only when the test actually compiled and ran green.

**Design — two states, not three.** `--no-test` is the *only* way to skip and succeed. Everything else fails closed.

| Situation | default | `--no-test` |
|---|---|---|
| Compiler present, test passes | exit 0 | skip, exit 0 |
| Compiler present, test fails | **exit 1** | skip, exit 0 |
| Compiler missing / `--compiler ""` | **exit 1** | skip, exit 0 |
| Generated `test.cpp` missing | **exit 1** | skip, exit 0 |

**`--test-if-available` is deliberately NOT implemented**, despite the original review asking for it. Default-fail-closed plus an explicit `--no-test` is already a complete model: nothing in the tree wants a third "succeeded without testing" mode (the only skip consumer, `scripts/check_golden.sh:32`, already passes `--no-test`). Adding one would expand the CLI, its tests and its docs while weakening the exact invariant this task exists to restore. If a real caller ever needs it, add it then — with the rule that it may tolerate a missing *compiler* (environmental) but never a missing generated *main* (a codegen regression).

**Do not break `--no-test`:** it has 5 dependents — `scripts/check_golden.sh:32`, `tests/test_golden_tree.py:39`, `tests/test_decl_def_split.py:42`, `tests/test_nodes_namespace_emit.py:14,36`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_cli_fail_closed.py`:

```python
"""The generator must not report success when verification never happened.

The project thesis is fail-closed: a gate with no inputs must never green.
Skipping is legitimate only when the caller explicitly asked for it (--no-test).
"""

import subprocess
import sys

from x3d_cpp_gen.cli import compile_and_run_test


def test_missing_compiler_fails_closed(tmp_path):
    """A compiler that is not on PATH must FAIL, not silently pass."""
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    (tmp_path / "x3d" / "nodes" / "test.cpp").write_text("int main() { return 0; }\n")
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path),
        compiler="definitely-not-a-real-compiler-xyz",
    )
    assert ok is False, "a missing compiler must fail closed"


def test_missing_generated_main_fails_closed(tmp_path):
    """A missing test.cpp is a codegen regression -- never report success."""
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path), compiler="g++",
    )
    assert ok is False, "a dropped smoke-test main must fail closed"


def test_empty_compiler_fails_closed(tmp_path):
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    (tmp_path / "x3d" / "nodes" / "test.cpp").write_text("int main() { return 0; }\n")
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path), compiler="",
    )
    assert ok is False, "--compiler '' must fail closed; use --no-test to skip"


def test_no_test_still_skips_and_succeeds(tmp_path):
    """--no-test is the ONE sanctioned way to skip. It has 5 dependents."""
    result = subprocess.run(
        [sys.executable, "-m", "x3d_cpp_gen.cli", "--no-test", "--out", str(tmp_path)],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, result.stderr
    assert not (tmp_path / "x3d" / "nodes" / "test.cpp").exists()
```

- [ ] **Step 2: Run it to verify it FAILS**

```bash
uv run pytest tests/test_cli_fail_closed.py -v
```

Expected: FAIL on `test_missing_compiler_fails_closed`, `test_missing_generated_main_fails_closed` and `test_empty_compiler_fails_closed` — all three currently return `True`. `test_no_test_still_skips_and_succeeds` should PASS already; it is a regression guard, not a red test.

- [ ] **Step 3: Rewrite `compile_and_run_test`**

Replace `src/x3d_cpp_gen/cli.py:31-88` with:

```python
def compile_and_run_test(test_file_path: str, output_dir: str,
                         compiler: str = "g++") -> bool:
    """Compile and run the generated smoke test. Fail closed.

    Returns True ONLY when the test actually compiled and ran green. Every path
    where verification did not happen returns False: reporting success when the
    thing under test was never built is exactly the failure this function used
    to have. The caller skips deliberately with --no-test; there is no
    succeed-without-testing path in here.
    """
    if not compiler:
        print("ERROR: no compiler configured, so the smoke test cannot run. "
              "Pass --no-test to skip deliberately.", file=sys.stderr)
        return False

    output_binary = os.path.join(output_dir, "test_exec")
    # Each concrete node's vtable is emitted in the TU that defines its key
    # function (e.g. WorldInfo::nodeTypeName() in WorldInfo.cpp), so the smoke
    # test must LINK every generated .cpp -- linking test.cpp alone leaves every
    # vtable/VTT undefined. test.cpp is itself in x3d/nodes/, so the glob already
    # includes it (don't list it twice or main is multiply defined).
    nodes_dir = os.path.join(output_dir, "x3d", "nodes")
    test_main = os.path.join(nodes_dir, "test.cpp")
    if not os.path.exists(test_main):
        # Not environmental: generate_test_file() ran, so this main should exist.
        # Its absence means the generator dropped it -- never report success.
        print(f"ERROR: generated smoke-test main not found: {test_main}\n"
              "The generator was asked to produce it and did not; this is a "
              "codegen regression, not a missing tool.", file=sys.stderr)
        return False

    sources = sorted(str(p) for p in Path(nodes_dir).glob("*.cpp"))
    compile_command = [compiler, "-std=c++20", "-I", output_dir,
                       *sources, "-o", output_binary]

    try:
        compile_result = subprocess.run(compile_command, capture_output=True, text=True)
    except FileNotFoundError:
        print(f"ERROR: compiler '{compiler}' not found, so the smoke test could "
              f"not run. Pass --no-test to skip deliberately.", file=sys.stderr)
        return False

    if compile_result.returncode != 0:
        print("Compilation failed.", file=sys.stderr)
        if compile_result.stdout:
            print(compile_result.stdout)
        if compile_result.stderr:
            print(compile_result.stderr, file=sys.stderr)
        return False

    print("Compilation successful. Running test...")
    run_result = subprocess.run([output_binary], capture_output=True, text=True)
    if run_result.stdout:
        print(run_result.stdout)
    if run_result.returncode != 0:
        print("Test execution failed.", file=sys.stderr)
        if run_result.stderr:
            print(run_result.stderr, file=sys.stderr)
        return False
    return True
```

- [ ] **Step 4: Fix the `--compiler` help**

`cli.py:125-128` currently says "empty string to skip", which is now false. Replace with:

```python
    parser.add_argument(
        "--compiler", default=os.environ.get("CXX", "g++"),
        help="C++ compiler for the smoke test (env CXX). Use --no-test to skip "
             "the smoke test; an empty value is an error, not a skip.",
    )
```

Leave the `--no-test` argument exactly as it is — it already reads "Skip generating and compiling the smoke test", which is now the literal truth.

- [ ] **Step 5: Wire it in `main`**

`cli.py:194-201` needs no structural change (the `if not args.no_test:` guard is already correct); only the now-simpler call signature:

```python
    if not args.no_test:
        test_file_path = generate_test_file(nodes, str(out_dir),
                                            templates_dir=templates_dir,
                                            enum_defs=enum_defs)
        ok = compile_and_run_test(test_file_path, str(out_dir),
                                  compiler=args.compiler)
        if not ok:
            return 1
```

- [ ] **Step 6: Run the tests to verify they PASS**

```bash
uv run pytest tests/test_cli_fail_closed.py -v
```

Expected: PASS, 4 passed.

- [ ] **Step 7: Verify the 5 `--no-test` dependents still work**

```bash
uv run pytest tests/test_golden_tree.py tests/test_decl_def_split.py tests/test_nodes_namespace_emit.py -q
scripts/check_golden.sh
```

Expected: PASS, and `Golden tree OK: ...`.

- [ ] **Step 8: Verify the default path really is fail-closed end to end**

```bash
uv run x3d-cpp-gen --out /tmp/fc-probe --compiler definitely-not-real-xyz; echo "EXIT=$?"
```

Expected: `EXIT=1` and an `ERROR: compiler ... not found` message. Before this task it printed a warning and exited 0.

- [ ] **Step 9: Commit**

```bash
rm -f /tmp/fc-probe -r
git add src/x3d_cpp_gen/cli.py tests/test_cli_fail_closed.py
git commit -m "fix(gen): fail closed when the smoke test never ran

The generator returned success when the compiler was missing, when --compiler
was empty, and when the generated test.cpp was absent -- so a codegen
regression that dropped the smoke-test main produced a green invocation.

--no-test remains the one sanctioned skip. No succeed-without-testing mode is
added: nothing needs one, and a second such path would weaken the invariant this
restores."
```

---

### Task 4: Correct the golden gate's own description

The gate compares both `*.hpp` and `*.cpp` (`scripts/check_golden.sh:54,63`; `tests/test_golden_tree.py:63`), but its comments, messages, README text and pytest assertion strings all claim headers only. The implementation is right; the words are wrong. This task changes **no behaviour**.

**Files:**
- Modify: `scripts/check_golden.sh` lines 5, 9, 13-14, 34-35, 39, 56, 74
- Modify: `tests/test_golden_tree.py` lines 3, 4-5, 9, 34, 69, 73, 74, 80-82
- Modify: `README.md` lines 178, 189-191, 197, 199-201
- Modify: `mise.toml` line 36
- Modify: `docs/wiki/subsystems/generator.md:107`
- Modify: `docs/wiki/guides/golden-regeneration.md:78`
- Modify: `docs/sdk/v1-capabilities.md:88`

**Interfaces:**
- Consumes: nothing.
- Produces: nothing. Wording only.

**Vocabulary (use consistently):** "generated C++ source tree", "`*.hpp` and `*.cpp`", "generated source files".

**Do not touch** — these are already correct and are the canonical phrasing to copy: `docs/wiki/decisions/0005-golden-files-in-git.md:28,32,36,65,67`, `docs/wiki/decisions/0006-compiled-static-lib.md:46`, `docs/wiki/guides/gate-system.md:24`, `docs/wiki/guides/build-and-mise.md:226`, `docs/wiki/subsystems/templates.md:110`, `docs/wiki/subsystems/generated-bindings.md:146`, `docs/wiki/subsystems/reflection.md:149`. **Never touch** `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md:128` (historical record). `tests/test_golden_smoke.py:1` is genuinely header-only — leave it.

- [ ] **Step 1: Fix the shell gate's final message and header comment**

`scripts/check_golden.sh:74`:

```bash
echo "Golden tree OK: generated *.hpp/*.cpp match generated_cpp_bindings/ byte-for-byte."
```

`scripts/check_golden.sh:2-14` — rewrite the header block:

```bash
# Golden-drift gate.
#
# Regenerates the full generated C++ source tree into a throwaway temp directory
# and diffs every generated source file (*.hpp and *.cpp) against the committed
# generated_cpp_bindings/. Exits 0 when the trees are identical and non-zero
# (with a readable report) on ANY drift.
#
# Codegen changes are therefore opt-in: change a template/emitter, then run
# `uv run x3d-cpp-gen --out generated_cpp_bindings` and COMMIT the new sources.
# This script (and tests/test_golden_tree.py) fail until that is done.
#
# Runnable locally (`scripts/check_golden.sh`) and from CI. The smoke test.cpp
# and test_exec are generation artifacts (gitignored) and are excluded from the
# comparison; every other *.hpp and *.cpp is golden.
```

`scripts/check_golden.sh:34-35`:

```bash
# Compare the FULL generated source tree (*.hpp + *.cpp), in both directions
# (catches added/removed files as well as content drift).
```

`scripts/check_golden.sh:39`:

```bash
# 1. Generated sources present in golden: must exist and match in the regen.
```

`scripts/check_golden.sh:56`:

```bash
# 2. Generated sources present in regen but NOT in golden (uncommitted output).
```

- [ ] **Step 2: Run the gate to confirm behaviour is unchanged**

```bash
scripts/check_golden.sh
```

Expected: `Golden tree OK: generated *.hpp/*.cpp match generated_cpp_bindings/ byte-for-byte.`

- [ ] **Step 3: Fix the Python twin**

`tests/test_golden_tree.py:1-9` — rewrite the module docstring:

```python
"""Full-tree golden-drift test (the pytest twin of scripts/check_golden.sh).

Regenerates the ENTIRE generated C++ source tree into a temp dir and asserts
every generated source file (*.hpp and *.cpp) matches the committed
generated_cpp_bindings/ byte-for-byte (in both directions: no missing, no extra,
no drifted files). To land an intentional codegen change, run
`uv run x3d-cpp-gen --out generated_cpp_bindings`, and commit the new sources.
"""
```

`tests/test_golden_tree.py:34` — this docstring is flatly wrong ("headers only" describes `--no-test`, not the extension set):

```python
    """Run the real CLI to regenerate the full generated source tree (no smoke test)."""
```

`tests/test_golden_tree.py:69,73,74,80-82` — assertion strings:

```python
    assert golden, "no golden sources found"
```
```python
    assert not missing, f"generated sources in golden but not regenerated: {missing}"
    assert not extra, f"generated sources regenerated but not committed to golden: {extra}"
```
```python
    assert not drifted, (
        "regenerated sources differ from committed golden "
        f"(regenerate + commit): {drifted}"
    )
```

- [ ] **Step 4: Run the Python twin**

```bash
uv run pytest tests/test_golden_tree.py -v
```

Expected: PASS.

- [ ] **Step 5: Fix `mise.toml:36`**

```toml
description = "Golden-drift gate: regenerate to a temp dir and diff every generated *.hpp/*.cpp."
```

- [ ] **Step 6: Fix the README**

`README.md:178`:

```
mise run golden        # golden-drift gate (regenerate to a temp dir, diff every *.hpp/*.cpp)
```

`README.md:189-191`:

```
`generated_cpp_bindings/` is **golden**: every generated `*.hpp` and `*.cpp` is
committed and treated as the source of truth for codegen output. The only
generation artifacts that are NOT golden are `test.cpp` / `test_exec` (gitignored).
```

`README.md:197`:

```
3. Review and **commit** the new generated sources.
```

`README.md:199-201`:

```
The golden-drift gate (`scripts/check_golden.sh`, `tests/test_golden_tree.py`,
and the `golden` CI job) regenerates into a temp dir and fails on ANY difference
in the generated `*.hpp`/`*.cpp` tree, so uncommitted codegen drift can never
land silently.
```

- [ ] **Step 7: Fix the three doc pages**

`docs/wiki/subsystems/generator.md:107`:

```
- `uv run pytest tests/test_golden_tree.py` — full-tree golden-drift test: runs the CLI into a temp dir and diffs every generated `*.hpp` and `*.cpp` against `generated_cpp_bindings/` (both directions: no missing, no extra, no drifted files). This is the primary regression gate for any pipeline change.
```

`docs/wiki/guides/golden-regeneration.md:78` — quote the new message:

```
5. On clean: prints `Golden tree OK: generated *.hpp/*.cpp match generated_cpp_bindings/ byte-for-byte.` and exits 0.
```

`docs/sdk/v1-capabilities.md:88` — currently contradicts line 89 of its own table:

```
| Golden | BYTE-IDENTICAL (every generated `*.hpp` + `*.cpp` regenerated == committed) |
```

- [ ] **Step 8: Verify no stale claim survives**

```bash
git grep -n 'only \*\.hpp\|headers only\|only \*\.hpp headers are golden\|diff \*\.hpp\|regenerated \*\.hpp match' -- \
  scripts/ tests/ README.md mise.toml docs/wiki docs/sdk
```

Expected: no output. (`docs/superpowers/` is excluded by design — historical record.)

- [ ] **Step 9: Run the gates**

```bash
scripts/check_golden.sh && uv run pytest tests/test_golden_tree.py -q && mise run docs-build
```

Expected: all pass.

- [ ] **Step 10: Commit**

```bash
git add scripts/check_golden.sh tests/test_golden_tree.py README.md mise.toml \
        docs/wiki/subsystems/generator.md docs/wiki/guides/golden-regeneration.md \
        docs/sdk/v1-capabilities.md
git commit -m "docs: describe the golden gate as covering *.hpp and *.cpp

The gate has compared both extensions since the C1 decl/def split (ADR-0005),
but its comments, messages, README text and pytest assertions all still said
headers only. Behaviour is unchanged; only the wording is corrected."
```

---

### Task 5: Make the golden formatter reproducible

The golden test skips when clang-format is missing, CI installs whatever Ubuntu currently ships, and there is no committed `.clang-format`. Byte-exact generation currently rests on the built-in LLVM default happening to agree across versions.

**Files:**
- Create: `.clang-format`
- Modify: `src/x3d_cpp_gen/backends/cpp_header.py:205-223`
- Modify: `mise.toml` (`[tools]`)
- Modify: `scripts/check_golden.sh` (print + validate the formatter version)
- Modify: `.github/workflows/ci.yml:93-94,109-110`
- Test: `tests/test_formatter_pin.py` (create)

**Interfaces:**
- Consumes: nothing.
- Produces: clang-format **22.1.8** pinned in `mise.toml` as the source of truth, asserted (full version, not just major) by `scripts/check_golden.sh` and `tests/test_formatter_pin.py`.

**CRITICAL — read correction 6 above.** `cpp_header.py:212` runs `clang-format -i` with no `--style`, so clang-format searches upward from each file. The golden is written inside the repo (would find `.clang-format`); the gate regenerates into `mktemp -d` under `/tmp` (would not). Committing a `.clang-format` **without** also passing `--style=file:<abs path>` breaks the gate permanently. Both halves must land together.

**Verified de-risking (already run on 2026-07-14):** a `.clang-format` containing exactly `BasedOnStyle: LLVM`, applied via `--style=file:`, reproduces the committed golden byte-for-byte under clang-format 22.1.8. The golden also passes today under CI's Ubuntu clang-format 18, so LLVM style for this output is stable across 18→22. **Expect zero golden drift.** If drift appears, do not blindly regenerate — the `.clang-format` has diverged from the LLVM default and should be corrected instead.

**Verified:** `mise install clang-format@22.1.8` succeeds via the conda backend (exit 0), and 22.1.8 matches the current dev machine.

- [ ] **Step 1: Write the failing test**

Create `tests/test_formatter_pin.py`:

```python
"""The golden invariant is byte-exact, so the formatter must be pinned.

Without a committed .clang-format the emitter depends on whatever built-in
default the installed clang-format happens to use; without a version pin, an
Ubuntu upgrade silently reformats the golden tree.
"""

import re
import shutil
import subprocess
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent
# The golden tree is BYTE-exact, so the contract is the FULL version, not the
# major: clang-format's default style can shift in any release. Keep this in
# sync with mise.toml [tools] and scripts/check_golden.sh.
EXPECTED_VERSION = "22.1.8"


def test_clang_format_config_is_committed():
    cfg = REPO_ROOT / ".clang-format"
    assert cfg.exists(), ".clang-format must be committed for byte-exact generation"
    assert "BasedOnStyle" in cfg.read_text()


def test_mise_pins_the_formatter_exactly():
    mise_toml = (REPO_ROOT / "mise.toml").read_text()
    assert f'clang-format = "{EXPECTED_VERSION}"' in mise_toml, (
        f"mise.toml [tools] must pin clang-format to exactly {EXPECTED_VERSION}"
    )


def test_emitter_passes_an_explicit_style_file():
    """clang-format must not be left to search upward for a style.

    The golden gate regenerates into a temp dir outside the repo, so an implicit
    search would find no .clang-format there and silently fall back to the
    built-in default -- formatting the temp tree differently from the committed
    one and breaking the gate forever.
    """
    src = (REPO_ROOT / "src/x3d_cpp_gen/backends/cpp_header.py").read_text()
    assert "--style=file:" in src, (
        "cpp_header.py must invoke clang-format with an explicit "
        "--style=file:<abs path>, not rely on an upward search"
    )


@pytest.mark.skipif(shutil.which("clang-format") is None,
                    reason="clang-format not installed")
def test_installed_formatter_matches_the_pin():
    out = subprocess.run(["clang-format", "--version"],
                         capture_output=True, text=True).stdout
    m = re.search(r"version (\d+\.\d+\.\d+)", out)
    assert m, f"could not parse clang-format version from: {out!r}"
    assert m.group(1) == EXPECTED_VERSION, (
        f"clang-format {m.group(1)} != pinned {EXPECTED_VERSION}; the golden "
        f"tree is byte-exact, so the full version is the contract. "
        f"Install the pin with: mise install clang-format@{EXPECTED_VERSION}"
    )
```

- [ ] **Step 2: Run it to verify it FAILS**

```bash
uv run pytest tests/test_formatter_pin.py -v
```

Expected: FAIL on the first three — no `.clang-format`, no mise pin, no `--style=file:`.

- [ ] **Step 3: Commit the style file**

Create `.clang-format`. Keep it minimal and equal to the built-in LLVM default — this is what makes the change zero-drift:

```yaml
# The generated C++ tree is a BYTE-EXACT golden artifact (ADR-0005), so its
# formatting is part of the contract. This file pins the style explicitly rather
# than relying on clang-format's built-in default, which is version-dependent.
#
# It is deliberately equal to the LLVM default: that is what produced the
# committed golden, so pinning it costs zero drift. Changing anything here
# reformats all of generated_cpp_bindings/ and requires a regenerate + commit.
#
# The formatter is pinned to an EXACT version (clang-format 22.1.8) in
# mise.toml [tools] and asserted by scripts/check_golden.sh; see
# tests/test_formatter_pin.py.
---
BasedOnStyle: LLVM
```

- [ ] **Step 4: Pass the style explicitly from the emitter**

In `src/x3d_cpp_gen/backends/cpp_header.py`, the call at line 212 currently reads:

```python
            result = subprocess.run([clang_format, "-i", *output_files],
                                   capture_output=True, text=True)
```

Resolve the repo style file and pass it. Add near the top of the module:

```python
# The repo's committed style. Resolved absolutely and passed explicitly:
# clang-format otherwise searches upward from each output file, which finds
# nothing when the golden gate regenerates into a temp dir outside the repo and
# silently falls back to the built-in default.
_STYLE_FILE = Path(__file__).resolve().parents[3] / ".clang-format"
```

and change the invocation to:

```python
            style_arg = (f"--style=file:{_STYLE_FILE}" if _STYLE_FILE.exists()
                         else "--style=LLVM")
            result = subprocess.run([clang_format, style_arg, "-i", *output_files],
                                   capture_output=True, text=True)
```

Verify `_STYLE_FILE` resolves correctly (`parents[3]` from `src/x3d_cpp_gen/backends/cpp_header.py` → repo root):

```bash
uv run python -c "from pathlib import Path; import x3d_cpp_gen.backends.cpp_header as m; print(m._STYLE_FILE, m._STYLE_FILE.exists())"
```

Expected: `/home/ben/code/x3d-cpp/.clang-format True`. If it prints a wrong path, fix the `parents[N]` index before continuing.

- [ ] **Step 5: Prove zero drift**

```bash
scripts/check_golden.sh
```

Expected: `Golden tree OK: ...`. If this fails, the `.clang-format` is not equal to the LLVM default — fix the style file; do **not** regenerate the golden to paper over it.

- [ ] **Step 6: Pin the formatter in mise**

In `mise.toml` `[tools]`:

```toml
[tools]
# uv provides the Python toolchain / project env. (Listed so `mise install`
# bootstraps it; pinning omitted to use whatever uv resolves from pyproject.)
uv = "latest"
# The generated C++ tree is byte-exact golden, so its formatter is part of the
# contract and is pinned exactly -- the full version, not just the major, since
# the default style can shift in any release. scripts/check_golden.sh asserts the
# running version matches; CI installs this same pin via jdx/mise-action. Bumping
# it reformats generated_cpp_bindings/ and needs a regenerate + commit in the
# same change.
clang-format = "22.1.8"
```

Verify:

```bash
mise install && mise exec -- clang-format --version
```

Expected: `clang-format version 22.1.8`.

- [ ] **Step 7: Print and validate the version in the gate**

In `scripts/check_golden.sh`, after the `GOLDEN_DIR` check (~line 23), insert:

```bash
# The golden tree is byte-exact, so the formatter is part of the contract and
# the FULL version is what we pin: clang-format's default style can shift in any
# release, not just a major. A mismatch would surface as "golden drift" that is
# really a toolchain mismatch -- fail loudly with the real reason instead. Keep
# this in sync with mise.toml [tools] and tests/test_formatter_pin.py.
EXPECTED_CLANG_FORMAT_VERSION=22.1.8
CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"

if ! command -v "${CLANG_FORMAT}" >/dev/null 2>&1; then
  echo "ERROR: '${CLANG_FORMAT}' not found. The golden tree is byte-exact and" >&2
  echo "cannot be verified without the pinned formatter. Install it with:" >&2
  echo "  mise install clang-format@${EXPECTED_CLANG_FORMAT_VERSION}" >&2
  exit 2
fi

CLANG_FORMAT_RAW="$("${CLANG_FORMAT}" --version)"
echo "Formatter: ${CLANG_FORMAT_RAW}"
CLANG_FORMAT_VERSION="$(printf '%s' "${CLANG_FORMAT_RAW}" | sed -n 's/.*version \([0-9]*\.[0-9]*\.[0-9]*\).*/\1/p')"
if [[ "${CLANG_FORMAT_VERSION}" != "${EXPECTED_CLANG_FORMAT_VERSION}" ]]; then
  echo "ERROR: clang-format ${CLANG_FORMAT_VERSION} != pinned ${EXPECTED_CLANG_FORMAT_VERSION}." >&2
  echo "The golden tree is byte-exact, so the full version is the contract;" >&2
  echo "refusing to run rather than report a toolchain mismatch as drift." >&2
  echo "Install the pin with: mise install clang-format@${EXPECTED_CLANG_FORMAT_VERSION}" >&2
  echo "(Run via mise to guarantee it: mise exec -- scripts/check_golden.sh)" >&2
  exit 2
fi
```

- [ ] **Step 8: Verify the gate now refuses a wrong formatter instead of skipping**

```bash
scripts/check_golden.sh                      # expect: Formatter: ... 22.1.8 + Golden tree OK
CLANG_FORMAT=/bin/false scripts/check_golden.sh; echo "EXIT=$?"
```

Expected: the first passes and prints the version; the second exits `2` with the version-mismatch or not-found error — **not** a silent skip.

Note the dev machine already has 22.1.8 on PATH; a machine with a distro clang-format will now fail this gate by design, with `mise install clang-format@22.1.8` as the stated remedy.

- [ ] **Step 9: Pin clang-format in CI**

`.github/workflows/ci.yml:93-94` and `:109-110` currently `apt-get install -y clang-format`, which pulls Ubuntu's default (18.x) — not the pin. Ubuntu's repos do not carry 22, so install it through the same mise pin that developers use. Replace **both** steps with:

```yaml
      - name: Install pinned clang-format (golden formatting is byte-exact)
        uses: jdx/mise-action@v2   # Task 15 replaces this tag with a SHA
        with:
          install_args: clang-format
```

**The input is `install_args`, not `tools`** — verified against `jdx/mise-action@v2`'s `action.yml`, whose inputs are `version`, `mise_dir`, `tool_versions`, `mise_toml`, `install`, `install_args`, `cache`, … There is no `tools` input; passing one is silently ignored, which would install *every* tool in `mise.toml` (harmless here, but not what the step says it does). Omitting `install_args` entirely and letting it install the whole `mise.toml` toolset is also valid.

Either way the version comes from `mise.toml`, so CI and local cannot diverge. If `jdx/mise-action` is unacceptable here, the fallback is the LLVM apt repo pinned to 22 — but do **not** leave plain `apt-get install clang-format`, which is the brittleness this task exists to remove.

- [ ] **Step 10: Run the full pin test + gates**

```bash
uv run pytest tests/test_formatter_pin.py -v && scripts/check_golden.sh && uv run pytest tests/test_golden_tree.py -q
```

Expected: all pass, 4 passed in the pin test.

- [ ] **Step 11: Commit**

```bash
git add .clang-format mise.toml scripts/check_golden.sh tests/test_formatter_pin.py \
        src/x3d_cpp_gen/backends/cpp_header.py .github/workflows/ci.yml
git commit -m "build: pin the golden formatter and commit .clang-format

The golden tree is byte-exact but its formatting depended on clang-format's
built-in default and on whatever version Ubuntu happened to ship. Pin 22.1.8 in
mise and CI, commit a .clang-format equal to the LLVM default (zero drift), and
make the gate print and assert the exact version instead of skipping when the
formatter is absent. The full version is the contract, not the major: the
default style can shift in any release.

The emitter now passes --style=file:<abs path>: clang-format otherwise searches
upward from each output file and would find no style when the gate regenerates
into a temp dir outside the repo."
```

---

# PR 2 — Identity, claims and packaging (Tasks 6-10)

> Ships alone, after PR 1. Tasks 6, 7 and 8 all touch `docs/wiki/architecture.md:17` — whichever lands second must re-read the line rather than apply its diff blind.

### Task 6: Complete the project-identity split

**Files:**
- Modify: `README.md:1`
- Modify: `CMakeLists.txt:5` (DESCRIPTION)
- Modify: `mkdocs.yml:1-4`
- Modify: `NOTICE:1`
- Modify: `mise.toml:1`
- Modify: `docs/wiki/index.md:24`, `docs/wiki/architecture.md:17`
- Test: `tests/test_project_identity.py` (create)

**Interfaces:**
- Consumes: nothing.
- Produces: nothing.

**Naming model:** product/repo `x3d-cpp`; Python package + script `x3d-cpp-gen`; CMake project `x3d_cpp`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_project_identity.py`:

```python
"""x3d-cpp (the product) and x3d-cpp-gen (the Python generator) are distinct.

Surviving 'x3d-cpp-gen' occurrences must refer to the Python package, the
executable, or the generator subsystem -- never to the whole runtime.
"""

from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def test_readme_opens_with_the_product_name():
    first = (REPO_ROOT / "README.md").read_text().splitlines()[0].strip()
    assert first == "# x3d-cpp", f"README opens with {first!r}"


def test_cmake_description_is_not_header_only():
    text = (REPO_ROOT / "CMakeLists.txt").read_text()
    assert "Header-only" not in text.split("LANGUAGES")[0], (
        "the package installs compiled shared libraries; it is not header-only"
    )


def test_mkdocs_points_at_the_real_repository():
    text = (REPO_ROOT / "mkdocs.yml").read_text()
    assert "https://github.com/delta9000/x3d-cpp" in text
    assert "sandbrookvt" not in text, "wrong repository owner"
    assert "x3d-cpp-gen" not in text.split("nav:")[0], (
        "the wiki is the product's knowledge home, not the generator's"
    )


def test_notice_uses_the_product_name():
    first = (REPO_ROOT / "NOTICE").read_text().splitlines()[0].strip()
    assert first == "x3d-cpp", f"NOTICE opens with {first!r}"
```

- [ ] **Step 2: Run it to verify it FAILS**

```bash
uv run pytest tests/test_project_identity.py -v
```

Expected: FAIL on all four.

- [ ] **Step 3: Fix the identity sites**

`README.md:1` → `# x3d-cpp`

`CMakeLists.txt:3-6`:

```cmake
project(x3d_cpp
    VERSION 0.1.0
    DESCRIPTION "Headless, renderer-agnostic X3D domain-runtime SDK: C++ node bindings generated from the X3D Unified Object Model, plus a compiled runtime"
    LANGUAGES CXX C)
```

`mkdocs.yml:1-4`:

```yaml
site_name: x3d-cpp Knowledge Wiki
site_description: Canonical knowledge home for the x3d-cpp X3D domain runtime SDK.
site_author: Ben Sandbrook
repo_url: https://github.com/delta9000/x3d-cpp
```

`NOTICE:1` → `x3d-cpp`

`mise.toml:1` → `# Dev task runner for x3d-cpp.`

`docs/wiki/index.md:24` → `x3d-cpp is a code-generation pipeline and runtime SDK for the X3D 4.0 object model:`

`docs/wiki/architecture.md:17` → open with `x3d-cpp is a headless, renderer-agnostic X3D domain runtime SDK.` (the file-IO and 4.0/4.1 clauses in this same line are fixed in Tasks 7 and 8 — coordinate if those land first).

- [ ] **Step 4: Run the test to verify it PASSES**

```bash
uv run pytest tests/test_project_identity.py -v
```

Expected: PASS, 4 passed.

- [ ] **Step 5: Audit every surviving occurrence**

```bash
git grep -n 'x3d-cpp-gen' -- . ':!docs/superpowers' ':!uv.lock'
```

Review each hit by hand. Keep it **only** where it means the Python package/executable/generator (e.g. `pyproject.toml` `name`/`[project.scripts]`, `uv run x3d-cpp-gen ...` command lines, `docs/wiki/subsystems/generator.md`). Rewrite any hit that means the whole runtime.

- [ ] **Step 6: Verify the docs still build**

```bash
mise run docs-build
```

Expected: PASS (strict: no dead links, no nav orphans).

- [ ] **Step 7: Commit**

```bash
git add README.md CMakeLists.txt mkdocs.yml NOTICE mise.toml \
        docs/wiki/index.md docs/wiki/architecture.md tests/test_project_identity.py
git commit -m "docs: split the product name from the generator name

The repo was simultaneously x3d-cpp, x3d-cpp-gen, a header-only binding project
and a compiled runtime SDK. Settle it: product/repo = x3d-cpp, Python generator
package/command = x3d-cpp-gen, CMake project = x3d_cpp.

Also points the docs site at the real repository (it named the wrong owner and
the wrong repo) and drops the header-only CMake description, which stopped being
true when the runtime shipped as shared libraries."
```

---

### Task 7: Rewrite the claims that are literally untrue

**Files:**
- Modify: `README.md:10-11` and `README.md:118-122`
- Modify: `include/x3d/sdk.hpp:16-18`
- Modify: `docs/wiki/architecture.md:17,110`
- Test: `tests/test_claims.py` (create)

**Interfaces:**
- Consumes: nothing.
- Produces: nothing.

**The two claims.** "The SDK does no file IO" is contradicted by `sdk::parseFile`, which `include/x3d/sdk.hpp:66` exports and `runtime/parse/X3DParse.cpp:183-185` implements with `std::ifstream`. `sdk.hpp:16-18` asserts it ~50 lines above the export, and `sdk.hpp:27` advertises `sdk::parseFile("scene.x3dv")` in the same header. "Every node and field is spec-correct by construction" overstates what the project's own design doc claims: `docs/superpowers/specs/2026-06-13-m3-conformance-design.md:18` says the honest claim is "fidelity-by-construction + a once-per-spec-rev human anchor", not correctness.

`docs/wiki/subsystems/extract-textures.md:22` is scoped to the texture subsystem and is **true** — leave it. `tools/corpus_audit.hpp:23-24` already carves out the parse path correctly — leave it.

- [ ] **Step 1: Write the failing test**

Create `tests/test_claims.py`:

```python
"""Public claims must survive contact with the code.

The SDK exports parseFile(), which opens a file. The node layer is generated
from the UOM, which constrains structure -- not runtime semantics.
"""

from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

STRONG_IO_CLAIM_SITES = [
    "README.md",
    "include/x3d/sdk.hpp",
    "docs/wiki/architecture.md",
]


def test_no_unqualified_no_file_io_claim():
    """parseFile() opens a file; an unqualified 'no file IO' is false."""
    for rel in STRONG_IO_CLAIM_SITES:
        text = (REPO_ROOT / rel).read_text()
        for phrase in ("does **no** file IO", "does no file IO", "does NO file IO"):
            assert phrase not in text, (
                f"{rel} claims {phrase!r}, but the SDK exports parseFile() "
                f"(include/x3d/sdk.hpp:66 -> runtime/parse/X3DParse.cpp opens an ifstream)"
            )


def test_no_spec_correct_by_construction_claim():
    """Generation from the UOM constrains declarations, not runtime semantics."""
    text = (REPO_ROOT / "README.md").read_text()
    assert "spec-correct by construction" not in text, (
        "generation from the UOM does not prove runtime semantics or eliminate "
        "UOM errata; behavioral conformance is tested separately"
    )
```

- [ ] **Step 2: Run it to verify it FAILS**

```bash
uv run pytest tests/test_claims.py -v
```

Expected: FAIL, both tests.

- [ ] **Step 3: Rewrite the UOM claim**

`README.md:10-11`:

```
The C++ node layer is **generated from the official X3D Unified Object Model
(UOM)**: node and field declarations, types and defaults are generated from the
UOM, and behavioral conformance is tested separately. Generation substantially
reduces structural drift — it does not prove runtime semantics or eliminate UOM
errata.
```

- [ ] **Step 4: Rewrite the file-IO claim**

`README.md:118-122`:

```
The simulation and extraction core performs no hidden resource, network, image,
font, media or rendering I/O — those are embedder-supplied **seams**: *ports* in
the ports-and-adapters sense, where the core owns the interface and you supply
the backend (`AssetResolver`, `TextureResolver`, `FontMetrics`, `ScriptEngine`,
…), each proven swappable by a second backend. `parseFile()` above is a
synchronous local-file convenience API — the one deliberate exception. See
[`docs/sdk/`](docs/sdk/).
```

`include/x3d/sdk.hpp:16-18`:

```cpp
// The simulation and extraction core performs no hidden resource, network,
// image, font, media or rendering I/O — that stays in the embedder. parseFile()
// below is a synchronous local-file convenience API, the one deliberate
// exception.
```

`docs/wiki/architecture.md:17` — replace the "does no file IO, no decoding, and no rasterization itself" clause:

```
It parses any of the four X3D encodings into a document model, runs the
event/behavior cascade over a live scene graph, and extracts renderer-ready
descriptors. Beyond the parse path's own local-file reads (`parseFile`), it
performs no hidden resource, network, image, font, media or rendering I/O, and
no rasterization. Those responsibilities live behind explicit seams that the
embedder fills.
```

`docs/wiki/architecture.md:110`:

```
Beyond the parse path's own local-file reads (`parseFile`), the SDK performs no
hidden resource or network I/O, no image/movie decoding, no font rasterization,
no geodesy, and no scripting-language embedding. Each is a typed seam the
embedder fills. This is what makes the runtime renderer-agnostic.
```

- [ ] **Step 5: Run the test to verify it PASSES**

```bash
uv run pytest tests/test_claims.py -v
```

Expected: PASS, 2 passed.

- [ ] **Step 6: Verify the header still compiles and the docs build**

```bash
cmake --build --preset dev --target x3d_cpp_sdk && mise run docs-build
```

Expected: both pass.

- [ ] **Step 7: Commit**

```bash
git add README.md include/x3d/sdk.hpp docs/wiki/architecture.md tests/test_claims.py
git commit -m "docs: correct the no-file-IO and spec-correct-by-construction claims

'The SDK does no file IO' is contradicted by sdk::parseFile, which the facade
exports and implements with an ifstream -- sdk.hpp asserted it ~50 lines above
the export. Scope the claim to the simulation/extraction core and name
parseFile() as the deliberate exception.

'Every node and field is spec-correct by construction' overstates what
generation proves: the UOM constrains declarations, types and defaults;
behavioral conformance is tested separately."
```

---

### Task 8: One public-documentation synchronization pass

Five confirmed contradictions plus one omission. **Read corrections 2, 4 and 5 at the top of this plan first** — three of these are not what the review said they were.

**Files:**
- Modify: `README.md:203-207` (CI trigger)
- Modify: `docs/sdk/README.md:134-136` (seam status)
- Modify: `NOTICE:34-35` (corpus gates)
- Modify: `docs/wiki/index.md:18,24` (renderers, 4.0)
- Modify: `docs/sdk/v1-capabilities.md` (MovieDecoder omission)
- Test: extend `tests/test_claims.py`

**Interfaces:**
- Consumes: Task 7's `tests/test_claims.py`.
- Produces: nothing.

- [ ] **Step 1: Fix the CI-trigger claim**

`README.md:203-207` says the workflow runs "on demand (`workflow_dispatch` …) — re-enable the `push:` / `pull_request:` triggers", but `ci.yml:37-39` already has `pull_request:`. Replace with:

```
## CI

`.github/workflows/ci.yml` runs the fast, hermetic gates on every pull request
(pytest, golden drift, conformance-view drift, wiki strict build, and a
single-compiler C++ build + ctest). The heavy 4-compiler baseline matrix stays
manual (`workflow_dispatch`). Forgejo Actions reads the same file if the repo is
mirrored there:
```

(If Task 15 lands first and adds the `push:` trigger, say "on every pull request and on pushes to `main`".)

- [ ] **Step 2: Fix the seam-status claim**

`docs/sdk/README.md:134-136` says "All seams are **experimental** in v1", but `docs/wiki/seam-status.md:41-48` declares six seams STABLE (ScriptEngine, AssetResolver/IO, Audio, FontMetrics, TextureResolver, MovieDecoder) and `include/x3d/sdk.hpp:20-22` agrees with the matrix. Replace with:

```
The core is IO-free apart from the parse path's own local-file reads. Decoding
and rasterization live in the embedder, supplied as `std::function` callbacks.
Each seam carries its own stability marker: `[STABLE]` seams are frozen pre-v2
(a breaking change is a major bump), `[EXPERIMENTAL]` seams are usable but may
gain fields. See the [seam status matrix](../wiki/seam-status.md) for the
current per-seam state — do not assume a blanket answer.
```

- [ ] **Step 3: Fix the corpus-gate claim**

`NOTICE:34-35` says corpus tests "skip cleanly"; `README.md:220-223` says the corpus differential gates *fail closed*, which is the accurate description and matches the project thesis. Fix NOTICE:

```
checkout at the X3D_CORPUS_DIR environment variable to run the corpus / gate
suites. The RAG and JDK seams skip cleanly when unset; the corpus differential
gates instead fail closed when asked to run without a corpus (a gate with no
inputs must never green) — fetch one with `mise run corpus-fetch`.
```

- [ ] **Step 4: Fix the "renderers not in this repo" wording**

`docs/wiki/index.md:18` — only the word is wrong (`examples/cpu_raster/` and `examples/poc_renderer/` are both in the repo; the architectural claim is sound and enforced by `X3D_CPP_BUILD_POC=OFF`). `examples/poc_renderer/README.md:4-8` already phrases it correctly. Replace "renderers are consumers, not part of this repo" with:

```
**Start here.** This is the single canonical, rendered knowledge home for the x3d-cpp project — a headless, renderer-agnostic X3D domain runtime SDK (renderers are consumers, not part of the SDK: the two in `examples/` are out-of-SDK consumers, firewalled behind their own build flags).
```

- [ ] **Step 5: Document the 4.0-vs-3.0–4.1 distinction**

Both are true on different axes — do **not** just pick one. `docs/wiki/index.md:24`:

```
x3d-cpp is a code-generation pipeline and runtime SDK for X3D. Two version axes
matter and are often confused: the **node layer** is generated from the X3D
**4.0** Unified Object Model, while the **parser** reads X3D **3.0–4.1**
encodings (XML, ClassicVRML, VRML97, JSON). So a 4.1 scene parses, but 4.1-only
nodes that the 4.0 UOM does not define (e.g. `EnvironmentLight`) are not in the
generated node layer — see [v1 capabilities](../sdk/v1-capabilities.md).
```

- [ ] **Step 6: Fix the MovieDecoder omission**

**The review had this backwards** (correction 2). `docs/sdk/v1-capabilities.md:63` — "MultiTexture compositing, MovieTexture frames" deferred — is **correct**: `runtime/extract/MovieDecoder.hpp:12-13` states the SDK never invokes the callback and never opens a media file, and `runtime/extract/RenderItem.hpp:255` marks Movie descriptor-only. Leave line 63 alone.

The real defect: `v1-capabilities.md` never mentions the MovieDecoder seam, so it undersells shipped work (`docs/wiki/seam-status.md:48` calls it STABLE with a CI-gated `x3d_movie_tests`). Add a row to the shipped-seams table:

```
| MovieDecoder seam | STABLE. Two backends (pl_mpeg, libtheora), CI-gated by `x3d_movie_tests`. The SDK never invokes the callback and never opens a media file — the consumer drives decode. MovieTexture *frame playback* remains deferred (row below). |
```

- [ ] **Step 7: Extend the claims test**

Append to `tests/test_claims.py`:

```python
def test_readme_does_not_call_ci_manual_only():
    """ci.yml has a pull_request trigger; the README told readers to enable it."""
    text = (REPO_ROOT / "README.md").read_text()
    assert "re-enable the `push:` / `pull_request:` triggers" not in text
    workflow = (REPO_ROOT / ".github/workflows/ci.yml").read_text()
    assert "pull_request:" in workflow


def test_sdk_guide_does_not_claim_all_seams_experimental():
    """docs/wiki/seam-status.md declares six seams STABLE."""
    text = (REPO_ROOT / "docs/sdk/README.md").read_text()
    assert "All seams are **experimental**" not in text


def test_notice_and_readme_agree_on_corpus_gates():
    notice = (REPO_ROOT / "NOTICE").read_text()
    assert "those tests skip cleanly" not in notice, (
        "the corpus differential gates fail closed; only RAG/JDK skip cleanly"
    )
    assert "fail closed" in notice


def test_wiki_does_not_deny_renderers_are_in_the_repo():
    text = (REPO_ROOT / "docs/wiki/index.md").read_text()
    assert "not part of this repo" not in text, (
        "examples/cpu_raster and examples/poc_renderer are in the repo; "
        "they are out-of-SDK consumers, which is a different claim"
    )
    assert (REPO_ROOT / "examples/cpu_raster").is_dir()
    assert (REPO_ROOT / "examples/poc_renderer").is_dir()


def test_capability_matrix_mentions_the_moviedecoder_seam():
    text = (REPO_ROOT / "docs/sdk/v1-capabilities.md").read_text()
    assert "MovieDecoder" in text, (
        "the MovieDecoder seam ships with two backends and a CI gate; the "
        "capability matrix must not omit it"
    )
```

- [ ] **Step 8: Run the tests + docs gate**

```bash
uv run pytest tests/test_claims.py -v && mise run docs-build
```

Expected: PASS, 7 passed; docs build clean.

- [ ] **Step 9: Commit**

```bash
git add README.md docs/sdk/README.md NOTICE docs/wiki/index.md \
        docs/sdk/v1-capabilities.md tests/test_claims.py
git commit -m "docs: synchronize the public capability claims with the code

Five contradictions: the README told readers to enable a pull_request trigger
that is already enabled; the SDK guide called all seams experimental while the
matrix declares six STABLE; NOTICE said the corpus gates skip cleanly while the
README (correctly) says they fail closed; the wiki said renderers are not in the
repo while two live in examples/; and nothing documented that the node layer is
4.0-UOM while the parser reads 3.0-4.1.

Also adds the MovieDecoder seam to the capability matrix, which omitted it
entirely -- the matrix undersold shipped work. MovieTexture frame playback stays
correctly listed as deferred: the SDK never invokes the decode callback."
```

**Longer term (not this PR):** generate the headline capability table from one structured source (`docs/conformance/findings.yaml` is the existing precedent) so prose cannot fork again. File as a card on the GitHub Project.

---

### Task 9: Put a normal build-and-install path in the README

The front door assumes mise, then jumps to `find_package()` without ever showing how to install the package. `scripts/verify_install_embed.sh` already demonstrates the correct sequence and is CI-gated.

**Files:**
- Modify: `README.md` (new "Build and install" section before the current quickstart; move mise to the contributor section ~line 171)

**Interfaces:**
- Consumes: nothing.
- Produces: nothing.

- [ ] **Step 1: Verify the sequence actually works before documenting it**

```bash
rm -rf /tmp/x3d-install-probe /tmp/x3d-build-probe
cmake -S . -B /tmp/x3d-build-probe -G Ninja -DX3D_CPP_BUILD_TESTS=OFF
cmake --build /tmp/x3d-build-probe
cmake --install /tmp/x3d-build-probe --prefix /tmp/x3d-install-probe
ls /tmp/x3d-install-probe/lib/cmake/x3d_cpp/
```

Expected: `x3d_cppConfig.cmake`, `x3d_cppConfigVersion.cmake`, `x3d_cppTargets.cmake`. **Do not write the README block until this passes** — document the sequence you actually ran.

- [ ] **Step 2: Verify the downstream consumer resolves against that prefix**

```bash
cmake -S examples/embed_minimal -B /tmp/x3d-consumer-probe -G Ninja \
  -DCMAKE_PREFIX_PATH=/tmp/x3d-install-probe
cmake --build /tmp/x3d-consumer-probe
```

Expected: configures and builds clean.

- [ ] **Step 3: Add the section to the README**

Insert before the current "### 1." quickstart:

````markdown
## Build and install

x3d-cpp is a normal CMake package. No project-specific tooling is needed to
build, install, or consume it:

```bash
cmake -S . -B build -G Ninja \
  -DX3D_CPP_BUILD_TESTS=OFF
cmake --build build
cmake --install build --prefix "$PWD/install"
```

Then consume it from a downstream project:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/install
```

```cmake
find_package(x3d_cpp CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE x3d_cpp::sdk)
```

`examples/embed_minimal/` is exactly this: a downstream-style project that does
not depend on the source tree. `scripts/verify_install_embed.sh` builds it
against a throwaway install prefix on every CI run, so the sequence above is
gate-enforced rather than aspirational.

**Requires:** a C++20 compiler and CMake 3.21+. Contributors additionally use
[mise](#dev-tasks-mise) as a task runner — see [Dev tasks](#dev-tasks-mise).
````

- [ ] **Step 4: Demote mise in the quickstart**

Re-read `README.md` lines 1-145. Anywhere the *evaluation* path (build, install, embed) implies `mise run ...` as a prerequisite, restate it in plain CMake and leave the mise form as the contributor convenience. The gallery/demo sections (`mise run cpuraster`, `mise run demos`) may keep mise — they are contributor workflows, not the SDK's front door.

- [ ] **Step 5: Verify the links resolve**

```bash
mise run docs-build && git grep -n "#dev-tasks-mise" README.md
```

Expected: docs build clean; the anchor matches the actual `## Dev tasks (mise)` heading at ~line 171.

- [ ] **Step 6: Clean up and commit**

```bash
rm -rf /tmp/x3d-install-probe /tmp/x3d-build-probe /tmp/x3d-consumer-probe
git add README.md
git commit -m "docs: show a plain cmake build-and-install path in the README

The front door assumed mise and then jumped to find_package() without ever
showing how to install the package. Lead with configure/build/install +
CMAKE_PREFIX_PATH -- the same sequence scripts/verify_install_embed.sh already
gates in CI -- and move mise to the contributor section."
```

---

### Task 10: Give the Python generator its own package README

`pyproject.toml:9` sets `readme = "README.md"`, so PyPI shows the full C++ runtime README — renderer examples, CMake targets, a gallery — to someone who installed a Python code generator. The sdist ships only `src/x3d_cpp_gen`, `pyproject.toml` and that README, and neither LICENSE nor NOTICE is included, despite the package bundling UOM data.

**Files:**
- Create: `README.generator.md`
- Modify: `pyproject.toml:9,44-49`
- Test: `tests/test_package_metadata.py` (create)

**Interfaces:**
- Consumes: nothing.
- Produces: an sdist and wheel containing `LICENSE` and `NOTICE`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_package_metadata.py`:

```python
"""The published Python package must describe itself, and carry its licenses.

pyproject publishes x3d-cpp-gen (a code generator) but used the C++ runtime's
README as its PyPI description. The package also bundles UOM data, so LICENSE
and NOTICE must ship with it.
"""

import shutil
import subprocess
import tarfile
import zipfile
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent


def test_generator_readme_exists_and_is_scoped():
    readme = REPO_ROOT / "README.generator.md"
    assert readme.exists(), "the Python package needs its own README"
    text = readme.read_text()
    assert "x3d-cpp-gen" in text
    # It must not drag in the runtime's front-page material.
    for leaked in ("## Gallery", "cpu_raster", "target_link_libraries"):
        assert leaked not in text, (
            f"{leaked!r} is runtime documentation; a PyPI user installed a "
            f"code generator and did not install that"
        )


def test_pyproject_points_at_the_generator_readme():
    text = (REPO_ROOT / "pyproject.toml").read_text()
    assert 'readme = "README.generator.md"' in text


@pytest.mark.slow
def test_sdist_and_wheel_carry_license_and_notice(tmp_path):
    # uv is a standalone binary (installed via mise here), NOT an importable
    # Python module -- `python -m uv` fails with "No module named uv". Invoke the
    # executable, and say so plainly if it is absent rather than erroring out
    # with a confusing ModuleNotFoundError.
    uv = shutil.which("uv")
    if uv is None:
        pytest.skip("uv not on PATH; run `mise install` (build artifacts need uv)")
    subprocess.run([uv, "build", "--out-dir", str(tmp_path)],
                   cwd=REPO_ROOT, check=True)
    sdist = next(tmp_path.glob("*.tar.gz"))
    wheel = next(tmp_path.glob("*.whl"))

    with tarfile.open(sdist) as tf:
        names = tf.getnames()
    assert any(n.endswith("/LICENSE") for n in names), f"LICENSE missing from sdist: {names}"
    assert any(n.endswith("/NOTICE") for n in names), f"NOTICE missing from sdist: {names}"
    assert any(n.endswith("/README.generator.md") for n in names), names

    with zipfile.ZipFile(wheel) as zf:
        names = zf.namelist()
    assert any(n.endswith("LICENSE") for n in names), f"LICENSE missing from wheel: {names}"
    assert any(n.endswith("NOTICE") for n in names), f"NOTICE missing from wheel: {names}"
```

- [ ] **Step 2: Run it to verify it FAILS**

```bash
uv run pytest tests/test_package_metadata.py -v
```

Expected: FAIL — no `README.generator.md`, `readme` still points at `README.md`, no LICENSE/NOTICE in the artifacts.

- [ ] **Step 3: Write the generator README**

Create `README.generator.md` covering only what a PyPI user gets:

````markdown
# x3d-cpp-gen

Generate spec-faithful C++ node bindings from the X3D Unified Object Model (UOM).

`x3d-cpp-gen` is the code generator for
[x3d-cpp](https://github.com/delta9000/x3d-cpp), a headless, renderer-agnostic
X3D domain-runtime SDK. This package is **only the generator** — installing it
does not install the C++ runtime.

## What it generates

A tree of C++20 sources under `x3d/core/` and `x3d/nodes/`:

- `x3d/core/` — the vocabulary: field value types, enums, and reflection support.
- `x3d/nodes/` — one `.hpp`/`.cpp` pair per X3D node, in namespace `x3d::nodes`,
  plus a node factory and an interface registry.

Node and field declarations, types and defaults are derived from the UOM.
Behavioral conformance is not this package's concern — it is tested in the
runtime repository.

## Install

Not yet published to PyPI — install from a checkout of the
[x3d-cpp](https://github.com/delta9000/x3d-cpp) repository:

```bash
git clone https://github.com/delta9000/x3d-cpp
cd x3d-cpp
uv tool install .        # or: pip install .
```

## Use

```bash
x3d-cpp-gen --out generated_cpp_bindings
```

The X3D 4.0 UOM ships inside the package, so no input file is required. Point it
at another UOM revision with `--spec`:

```bash
x3d-cpp-gen --spec X3dUnifiedObjectModel-4.1.xml --out out/
```

## Supported UOM inputs

Any `X3dUnifiedObjectModel-*.xml` revision. The spec version is auto-detected
from the root element's `version` attribute; override it with `--spec-version`.
The packaged default is 4.0.

## Options that affect verification

| Option | Behavior |
|---|---|
| *(default)* | Generates, then compiles and runs a smoke test. **Fails** if the compiler is missing or the generated main is absent — it will not report success for a test that never ran. |
| `--no-test` | Skips the smoke test and succeeds. The one explicit opt-out. |
| `--compiler` | C++ compiler for the smoke test (env `CXX`, default `g++`). An empty value is an error, not a skip. |
| `--clang-format` | Formatter executable (env `CLANG_FORMAT`). |

Output is formatted with `clang-format` if it is on your PATH (`--clang-format ""`
disables it). The style comes from the repository's `.clang-format`, which is
**not** shipped inside the installed package: when you run the generator from a
checkout it is picked up automatically, and when you run it from an installed
wheel elsewhere the emitter falls back to clang-format's built-in **LLVM** style.

That fallback only matters if you need output byte-identical to the x3d-cpp
repository's committed tree — there, formatting is part of a byte-exact golden
contract and the formatter version is pinned exactly. For generating bindings
for your own use, either style is fine.

## Relation to x3d-cpp

The [x3d-cpp](https://github.com/delta9000/x3d-cpp) repository commits this
generator's output and gates it byte-for-byte, so the generated tree is a
reviewed artifact rather than a build-time surprise. Use this package directly if
you want to generate bindings from a different UOM revision, or to inspect what
the runtime's node layer is derived from.

## License

MIT — see `LICENSE`. The bundled UOM data carries its own attribution; see
`NOTICE`.
````

- [ ] **Step 4: Fix pyproject**

`pyproject.toml:9` → `readme = "README.generator.md"`

Replace the sdist section (lines 44-49) and extend the wheel section so both artifacts carry the licenses:

```toml
[tool.hatch.build.targets.wheel]
# src layout: map the package and ship all non-Python data (templates + XML).
packages = ["src/x3d_cpp_gen"]
artifacts = [
    "src/x3d_cpp_gen/templates/*",
    "src/x3d_cpp_gen/data/*",
]
# The package bundles UOM data, so its attribution must travel with the wheel.
force-include = { "LICENSE" = "x3d_cpp_gen/LICENSE", "NOTICE" = "x3d_cpp_gen/NOTICE" }

[tool.hatch.build.targets.sdist]
include = [
    "src/x3d_cpp_gen",
    "pyproject.toml",
    "README.generator.md",
    "LICENSE",
    "NOTICE",
]
```

- [ ] **Step 5: Register the `slow` marker**

Add to the existing `[tool.pytest.ini_options]` so the build test does not warn:

```toml
markers = ["slow: tests that build distribution artifacts"]
```

- [ ] **Step 6: Run the tests to verify they PASS**

```bash
uv run pytest tests/test_package_metadata.py -v
```

Expected: PASS, 3 passed.

- [ ] **Step 7: Inspect the artifacts by hand**

```bash
rm -rf /tmp/x3d-dist && uv build --out-dir /tmp/x3d-dist
tar tzf /tmp/x3d-dist/*.tar.gz | grep -E 'LICENSE|NOTICE|README'
python -m zipfile -l /tmp/x3d-dist/*.whl | grep -E 'LICENSE|NOTICE'
```

Expected: LICENSE, NOTICE and README.generator.md present in the sdist; LICENSE and NOTICE in the wheel.

- [ ] **Step 8: Commit**

```bash
rm -rf /tmp/x3d-dist
git add README.generator.md pyproject.toml tests/test_package_metadata.py
git commit -m "packaging: give x3d-cpp-gen its own README and ship its licenses

pyproject published the C++ runtime's README as the PyPI description, so a user
who installed a Python code generator saw a gallery, renderer examples and CMake
targets they did not install. Add a generator-scoped README covering what it
generates, install/use, supported UOM inputs, and verification behavior.

The package bundles UOM data, so LICENSE and NOTICE now ship in both the sdist
and the wheel."
```

---

# PR 3 — CMake downstream behavior (Tasks 11-12)

> Ships alone, after PR 2. Both tasks are CMake-only and share one reviewer mindset: what does a consumer actually see?

### Task 11: Stop mutating the consumer's toolchain by default

Adding this project via `add_subdirectory()` writes `CMAKE_CXX_COMPILER_LAUNCHER` into the **shared cache** (`CMakeLists.txt:19-23`), so ccache becomes the default for every target in the consumer's build tree; and it sets a **global** Ninja job pool (`CMakeLists.txt:57`) that caps the consumer's compile parallelism. Both are cross-project mutations a dependency has no business making.

**Files:**
- Modify: `CMakeLists.txt:1` (`cmake_minimum_required`)
- Modify: `CMakeLists.txt:17-60` (ccache, linker, job pool)
- Modify: `CMakeLists.txt:110-122` (warnings)
- Test: `tests/cmake/dev_tooling/` (create)

**Interfaces:**
- Consumes: nothing.
- Produces: option `X3D_CPP_ENABLE_DEV_TOOLING` (default `ON` top-level, `OFF` as a subproject).

**Prerequisite — correction 7.** `cmake_minimum_required(VERSION 3.20)` but `PROJECT_IS_TOP_LEVEL` needs **3.21**. `CMakeLists.txt:407` already uses it, so on 3.20 it silently evaluates empty and `X3D_CPP_BUILD_TESTS` defaults OFF. Bump the floor **first** — this task depends on the variable being real.

**Read "What actually leaks to a consumer" at the top of this plan before writing the fixture.** The measured facts, not the intuition:

- `add_compile_options()` / `add_link_options()` are scoped to the current directory **and below**. They do **not** propagate to the parent. A fixture that checks the *parent's* `COMPILE_OPTIONS` after `add_subdirectory()` sees `[]` **both before and after** the fix — it cannot fail, so it proves nothing.
- The `set(... CACHE STRING)` launcher write **does** reach the parent (measured `PARENT_LAUNCHER=[ccache]`). That, plus the `GLOBAL` job-pool property, is the real bug.

**Scope note — what this task does NOT do.** Do **not** disable `-Wall -Wextra` for x3d-cpp's own targets merely because it is a subproject: those flags are correctly scoped to our directory and below, they harm nobody, and turning them off as a subproject would silently reduce our own warning coverage in a downstream build. Leave them directory-scoped inside our subdirectory. Moving them to `target_compile_options()` on owned targets is the *better* end state but is a ~40-site mechanical change — file it as a follow-up card (Step 10), do not attempt it here.

- [ ] **Step 1: Write the failing test**

Create `tests/cmake/dev_tooling/CMakeLists.txt` — a synthetic parent that adds x3d-cpp as a subproject and asserts it did not reach *outside its own subtree*.

Assert only on things that actually cross the boundary: the **cache** and a **parent-defined target's own properties**. Do not assert on the parent's directory properties — measured, those are empty before the fix too, so such a check can never fail.

```cmake
# Contract: adding x3d_cpp with add_subdirectory() must not mutate state that
# reaches OUTSIDE its own subtree.
#
# Scoping facts this fixture is built on (measured, not assumed):
#   - add_compile_options()/add_link_options() apply to the current directory
#     and BELOW. They never reach the parent, so asserting on the parent's
#     directory properties would pass even with the bug present. Not tested here.
#   - set(... CACHE ...) writes the SHARED cache and DOES reach the parent.
#   - set_property(GLOBAL ...) is global by definition.
# Those last two are what this fixture pins down.
cmake_minimum_required(VERSION 3.21)
project(x3d_dev_tooling_fixture LANGUAGES CXX)

# CONTROL. Created before the subproject, so the cache has not been written yet
# and its CXX_COMPILER_LAUNCHER is baked empty. It must stay empty whether or not
# the bug is present -- it exists to prove the fixture discriminates, not to
# witness the mutation.
add_library(consumer_before STATIC consumer.cpp)

# Consume x3d-cpp exactly as a downstream project would.
add_subdirectory("${X3D_CPP_SOURCE_DIR}" x3d_cpp_sub EXCLUDE_FROM_ALL)

# WITNESS. Created after the subproject, so this is the target that actually
# inherits any cache-level launcher x3d_cpp wrote. This is the one that goes red.
add_library(consumer_after STATIC consumer.cpp)

set(_failures "")

# 1. The cached launcher. This is the confirmed cross-project mutation:
#    set(CMAKE_CXX_COMPILER_LAUNCHER ... CACHE STRING) makes ccache the default
#    for every target in the consumer's tree, not just x3d_cpp's.
if(CMAKE_CXX_COMPILER_LAUNCHER)
  list(APPEND _failures
       "x3d_cpp set CMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER} in the cache as a subproject; a dependency must not choose the consumer's launcher")
endif()
if(CMAKE_C_COMPILER_LAUNCHER)
  list(APPEND _failures
       "x3d_cpp set CMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER} in the cache as a subproject")
endif()

# 2. A parent-defined target must keep its own (empty) launcher. consumer_after
#    is the real witness; consumer_before is the control and must never trip.
foreach(_t consumer_before consumer_after)
  get_target_property(_launcher ${_t} CXX_COMPILER_LAUNCHER)
  if(_launcher)
    list(APPEND _failures
         "parent target ${_t} inherited CXX_COMPILER_LAUNCHER=${_launcher} from x3d_cpp")
  endif()
endforeach()

# 3. The global Ninja job pool caps the CONSUMER's build parallelism too.
get_property(_pools GLOBAL PROPERTY JOB_POOLS)
if(_pools MATCHES "x3d_compile")
  list(APPEND _failures
       "x3d_cpp defined the GLOBAL job pool '${_pools}' as a subproject; this bounds the consumer's own compiles")
endif()

if(_failures)
  string(REPLACE ";" "\n  - " _msg "${_failures}")
  message(FATAL_ERROR "dev-tooling isolation FAILED:\n  - ${_msg}")
endif()
message(STATUS "dev-tooling isolation OK: subproject reached nothing outside its own subtree")
```

Create `tests/cmake/dev_tooling/consumer.cpp`:

```cpp
int consumer() { return 0; }
```

Register it in the root `CMakeLists.txt` beside the other cmake fixtures:

```cmake
# Contract: adding x3d_cpp with add_subdirectory() must not mutate cache or
# global state that reaches the consumer. Configure-only.
#
# -G is REQUIRED: without it the nested configure picks CMake's default (often
# Unix Makefiles), where the JOB_POOLS property is ignored -- the fixture would
# then never exercise the global-pool bug and would pass for the wrong reason.
#
# -DCCACHE_PROGRAM makes the launcher half hermetic: x3d_cpp guards its cache
# write behind find_program(CCACHE_PROGRAM ccache), so on a machine without
# ccache the block is a no-op and the test again passes for the wrong reason.
# Pre-seeding the find_program cache variable with a known-present executable
# forces the write to happen. It is never executed (configure-only), so any real
# path works; /bin/echo is used because it exists everywhere we build.
add_test(NAME x3d_dev_tooling_isolation
         COMMAND "${CMAKE_COMMAND}"
                 -S "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/dev_tooling"
                 -B "${CMAKE_CURRENT_BINARY_DIR}/dev_tooling_fixture"
                 -G "${CMAKE_GENERATOR}"
                 "-DCCACHE_PROGRAM=/bin/echo"
                 "-DX3D_CPP_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
```

- [ ] **Step 2: Run it to verify it FAILS**

```bash
rm -rf build/dev_tooling_fixture
cmake --preset dev && ctest --preset dev -R x3d_dev_tooling_isolation --output-on-failure
```

Expected: FAIL, reporting the cached `CMAKE_CXX_COMPILER_LAUNCHER=/bin/echo`, the `consumer_after` target inheriting it, and the `x3d_compile` global job pool.

**If it passes here, stop.** A green Step 2 means the test is worthless. Diagnose before continuing — the usual causes are the nested configure not inheriting `-G Ninja` (the job-pool assertion silently can't fire under Make) or `CCACHE_PROGRAM` not reaching the child (the launcher assertion can't fire). Both are pre-empted by the `add_test` arguments above; if it still passes, verify them with `cmake -LA build/dev_tooling_fixture | grep -E 'CCACHE_PROGRAM|GENERATOR'` before touching the source.

- [ ] **Step 3: Bump the CMake floor**

`CMakeLists.txt:1`:

```cmake
# 3.21 is the floor because PROJECT_IS_TOP_LEVEL (used to scope dev tooling and
# the test default) was added in 3.21. On 3.20 it silently evaluated empty.
cmake_minimum_required(VERSION 3.21)
```

**Also update the README in this same commit.** PR 2's Task 9 documents
"**Requires:** a C++20 compiler and CMake 3.20+", matching the floor *as declared
at the time*. This task changes that floor, so the README line must move to
3.21+ here or the install instructions start lying. Verify with:

```bash
grep -n 'CMake 3\.' README.md
```

- [ ] **Step 4: Add the guard option**

Immediately after the `project()` call and the `include()`s (~line 10), insert:

```cmake
# ---------------------------------------------------------------------------
# Developer tooling (ccache, fast linker, Ninja job pool, project-wide warning
# is for people building THIS project, and some of it escapes our own subtree:
# the ccache launcher is written to the SHARED CACHE (so it becomes the default
# for every target in the consumer's build tree) and the Ninja job pool is a
# GLOBAL property (so it caps the consumer's compiles too). A consumer that
# merely add_subdirectory()s us must not inherit either. Default ON when we are
# the top-level project, OFF when we are somebody's dependency.
#
# Deliberately NOT gated here: -Wall -Wextra and the sanitizer options. Those are
# directory-scoped (current dir and below), so they never reach a consumer's
# targets -- gating them would only disable OUR OWN warning coverage in a
# vendored build. X3D_CPP_SAN/X3D_CPP_FUZZ default OFF and are opt-in gates.
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
    option(X3D_CPP_ENABLE_DEV_TOOLING
           "Enable developer tooling that escapes this subtree: ccache launcher, fast linker, Ninja job pool" ON)
else()
    option(X3D_CPP_ENABLE_DEV_TOOLING
           "Enable developer tooling that escapes this subtree: ccache launcher, fast linker, Ninja job pool" OFF)
endif()
```

- [ ] **Step 5: Guard the three blocks that escape our subtree**

Wrap each in `if(X3D_CPP_ENABLE_DEV_TOOLING)` … `endif()`, preserving the existing comments:

- **ccache** — `CMakeLists.txt:17-23` (the `find_program` / `set(... CACHE STRING ...)` block). *Measured to leak.*
- **Ninja job pool** — `CMakeLists.txt:54-60` (`set_property(GLOBAL PROPERTY JOB_POOLS ...)`). *Global by definition.*
- **linker** — `CMakeLists.txt:31-39`. Does **not** leak (directory-scoped), but a dependency silently selecting the consumer's linker for its own targets is a surprise worth gating, and it is dev tooling by any reading.

**Do NOT guard the warnings block** (`CMakeLists.txt:117-122`). `add_compile_options(-Wall -Wextra)` is scoped to our directory and below — it reaches no consumer target (measured). Gating it would only disable *our own* warning coverage when someone vendors us, which is a regression, not a courtesy. Leave it directory-scoped where it is.

Leave `X3D_CPP_SAN` and `X3D_CPP_FUZZ` (`133-156`) alone too: both default OFF and are opt-in gates, so they cannot surprise a consumer. Note in the guard's comment that warnings/sanitizers are deliberately excluded and why, so nobody "completes" this later by mistake.

- [ ] **Step 6: Run the test to verify it PASSES**

```bash
rm -rf build/dev_tooling_fixture
cmake --preset dev && ctest --preset dev -R x3d_dev_tooling_isolation --output-on-failure
```

Expected: PASS — `dev-tooling isolation OK: subproject reached nothing outside its own subtree`.

- [ ] **Step 7: Verify the top-level build is unchanged**

```bash
rm -rf build && cmake --preset dev 2>&1 | grep -E 'ccache enabled|linker =|job pool'
cmake --build --preset dev && ctest --preset dev -j "$(nproc)"
```

Expected: all three STATUS lines still print (we are top-level, so tooling stays ON), and the build + ctest pass exactly as before. If ccache silently stops being used at top level, the guard is inverted.

- [ ] **Step 8: Verify the ci preset still promotes warnings**

```bash
rm -rf build-ci && cmake --preset ci 2>&1 | grep 'X3D_CPP_WERROR=ON'
```

Expected: `x3d_cpp: X3D_CPP_WERROR=ON -> warnings treated as errors`. The warnings block is deliberately *outside* the dev-tooling guard, so this must hold regardless of `X3D_CPP_ENABLE_DEV_TOOLING`. If it is silent, the guard was wrapped around the merge gate — back it out.

- [ ] **Step 9: Commit**

```bash
git add CMakeLists.txt tests/cmake/dev_tooling/CMakeLists.txt tests/cmake/dev_tooling/consumer.cpp
git commit -m "build: don't mutate cache or global state as a subproject

add_subdirectory()ing x3d_cpp wrote CMAKE_CXX_COMPILER_LAUNCHER into the shared
cache, making ccache the default for every target in the consumer's build tree,
and defined a GLOBAL Ninja job pool that capped the consumer's own compile
parallelism. Gate those (and the linker choice) behind X3D_CPP_ENABLE_DEV_TOOLING,
default ON top-level and OFF as a dependency.

-Wall -Wextra is deliberately left directory-scoped: add_compile_options applies
to the current directory and below and never reaches a consumer's targets, so
gating it would only reduce our own warning coverage in a vendored build.

Also bumps the CMake floor to 3.21: PROJECT_IS_TOP_LEVEL was already used at
line 407 but was added in 3.21, so on 3.20 it silently evaluated empty and
X3D_CPP_BUILD_TESTS defaulted OFF.

Moving warnings to target_compile_options on owned targets is filed as a
follow-up."
```

- [ ] **Step 10: File the follow-up**

Create a card on the [GitHub Project](https://github.com/users/delta9000/projects/2): "Move -Wall -Wextra from add_compile_options to target_compile_options on owned targets" — referencing `CMakeLists.txt:117` and this commit. Note in the card that this is a **hygiene** improvement, not a leak fix: the directory-scoped form was measured not to reach consumers. The benefit is that vendored builds keep our warning coverage while the flags travel with the targets that want them.

---

### Task 12: Finish the installed-target contract

PR 56 added `EXPORT_NAME authoring`, and `scripts/verify_install_embed.sh` builds both consumers. But `examples/embed_minimal` only links **two** imported targets (`x3d_cpp::sdk`, `x3d_cpp::authoring`); the other five are only pulled in transitively, so a broken `EXPORT_NAME` on any of them would not be caught.

**Files:**
- Create: `tests/cmake/installed_targets/CMakeLists.txt`
- Modify: `scripts/verify_install_embed.sh` (invoke the new fixture against the install prefix)

**Interfaces:**
- Consumes: the install prefix produced by `scripts/verify_install_embed.sh`.
- Produces: an assertion that all **7** imported targets exist.

**The full installed set is 7** (correction 3) — the review's list omits `x3d_cpp::x3d_cpp`. Verified from `CMakeLists.txt`: `EXPORT_NAME` on `x3d_cpp_headers`→`headers` (70), `x3d_cpp_nodes`→`nodes` (199), `x3d_cpp_authoring_runtime`→`authoring_runtime` (226), `x3d_cpp_runtime`→`runtime` (239), `x3d_cpp_authoring`→`authoring` (276), `x3d_cpp_sdk`→`sdk` (295); plus `x3d_cpp` itself (installed at 306, default export name → `x3d_cpp::x3d_cpp`).

**Test all seven, but do not call all seven public API.** Asserting the whole set under the banner "documented imported-target contract" would silently promise `x3d_cpp::nodes`, `x3d_cpp::runtime`, `x3d_cpp::authoring_runtime` and `x3d_cpp::headers` as stable user entry points. They exist because the export set must be closed over the dependency graph — a consumer linking `x3d_cpp::sdk` needs them to resolve — not because anyone should link them directly. Freezing them by accident is how a layer split becomes a breaking change.

Classify explicitly, and let the fixture say which is which:

| Group | Targets | Promise |
|---|---|---|
| **Public consumer entry points** | `x3d_cpp::sdk`, `x3d_cpp::authoring`, `x3d_cpp::x3d_cpp` (facade) | Documented. Renaming one is a breaking change. |
| **Exported implementation targets** | `x3d_cpp::headers`, `x3d_cpp::nodes`, `x3d_cpp::authoring_runtime`, `x3d_cpp::runtime` | Must exist for the export set to resolve. **Not** promised as stable entry points; may be restructured. |

Both groups get asserted — a missing implementation target breaks `find_package` just as hard — but only the first group is API. If the `x3d_cpp::x3d_cpp` facade is *not* intended as a public entry point, move it to the second group and say so in the fixture; that is a real decision this task should settle rather than inherit.

- [ ] **Step 1: Write the failing test**

Create `tests/cmake/installed_targets/CMakeLists.txt`:

```cmake
# Contract: every installed imported target must exist after find_package().
#
# examples/embed_minimal only links x3d_cpp::sdk and x3d_cpp::authoring, so a
# broken EXPORT_NAME on any other installed target would not be caught -- the
# in-tree ALIAS would still resolve while the installed name silently differed.
#
# The two groups below are NOT the same promise. PUBLIC names are documented
# entry points: renaming one is a breaking change. IMPLEMENTATION names exist so
# the export set is closed over the dependency graph (a consumer linking
# x3d_cpp::sdk needs them to resolve); they are asserted because their absence
# breaks find_package, NOT because they are promised as stable entry points.
# Do not cite this file as evidence that the implementation names are API.
cmake_minimum_required(VERSION 3.21)
project(x3d_installed_targets_contract LANGUAGES CXX)

find_package(x3d_cpp CONFIG REQUIRED)

# Documented entry points. Changing these is a breaking change.
set(_public
    x3d_cpp::sdk
    x3d_cpp::authoring
    x3d_cpp::x3d_cpp)

# Required for the export set to resolve. Restructurable; not promised.
set(_implementation
    x3d_cpp::headers
    x3d_cpp::nodes
    x3d_cpp::authoring_runtime
    x3d_cpp::runtime)

set(_missing_public "")
foreach(_t IN LISTS _public)
  if(NOT TARGET ${_t})
    list(APPEND _missing_public ${_t})
  endif()
endforeach()

set(_missing_impl "")
foreach(_t IN LISTS _implementation)
  if(NOT TARGET ${_t})
    list(APPEND _missing_impl ${_t})
  endif()
endforeach()

if(_missing_public)
  string(REPLACE ";" "\n  - " _msg "${_missing_public}")
  message(FATAL_ERROR
    "installed target contract FAILED -- missing PUBLIC entry points:\n  - ${_msg}\n"
    "These are documented names. Each installed target needs an explicit "
    "EXPORT_NAME matching its in-tree ALIAS, or the documented name will not "
    "exist downstream.")
endif()

if(_missing_impl)
  string(REPLACE ";" "\n  - " _msg "${_missing_impl}")
  message(FATAL_ERROR
    "installed target contract FAILED -- missing implementation targets:\n  - ${_msg}\n"
    "These are not public API, but the export set must be closed over them or "
    "find_package() cannot resolve the public entry points that depend on them.")
endif()

message(STATUS "installed target contract OK: 3 public + 4 implementation targets present")
```

- [ ] **Step 2: Prove the test can fail**

Temporarily add a bogus name (e.g. `x3d_cpp::not_a_target`) to `_public`, run Step 4's command, and confirm it reports it as a missing public entry point. Then remove it. A contract test that cannot fail is worthless — and this one has no natural red state, since the targets already exist.

- [ ] **Step 3: Wire it into the install verifier**

In `scripts/verify_install_embed.sh`, after the existing `embed_minimal` build and before the binaries run at lines 74-75, add:

```bash
# Contract: assert every documented imported target exists against the real
# install prefix. embed_minimal only links two of them directly.
echo "Checking installed target contract ..."
cmake -S "${REPO_ROOT}/tests/cmake/installed_targets" \
      -B "${WORK_DIR}/installed_targets" \
      -DCMAKE_PREFIX_PATH="${INSTALL_PREFIX}" >/dev/null
```

Match the surrounding variable names — read the script first; `WORK_DIR`/`INSTALL_PREFIX` above are illustrative and may be spelled differently.

- [ ] **Step 4: Run the install smoke**

```bash
cmake --preset dev && ctest --preset dev -R x3d_install_embed_smoke --output-on-failure
```

Expected: PASS, including `installed target contract OK: 3 public + 4 implementation targets present`.

- [ ] **Step 5: Commit**

```bash
git add tests/cmake/installed_targets/CMakeLists.txt scripts/verify_install_embed.sh
git commit -m "test: assert every installed imported target exists after install

embed_minimal links only x3d_cpp::sdk and x3d_cpp::authoring, so a broken
EXPORT_NAME on any of the other five installed targets would pass CI: the
in-tree ALIAS resolves while the installed name silently differs. Assert all
seven against the real install prefix.

The fixture separates the three public entry points from the four exported
implementation targets: the latter must exist for the export set to resolve, but
asserting them is not a promise that they are stable API."
```

---

# PR 4 — Public repository surface (Tasks 13-15)

> Ships last. The docs move touches internal links and will churn `mise run docs-build`; the CI changes are best verified by the PR's own run.

### Task 13: Separate public product docs from the development record

**Files:**
- Move: `docs/wiki/guides/workflow-subagent-discipline.md`, `docs/wiki/guides/card-to-done-workflow.md` → `docs/contributor/`
- Modify: `mkdocs.yml` (nav)
- Modify: `CLAUDE.md`
- Modify: `docs/wiki/index.md`, `docs/wiki/coverage.md`

**Interfaces:**
- Consumes: nothing.
- Produces: **the subagent/card workflow guides are removed from public navigation.** That is the whole claim — deliberately narrower than "the public surface contains no agent-workflow machinery", which this task does not achieve and should not pretend to.

**Reality check before moving anything.** `docs/wiki/` is 102 files and `docs/superpowers/` is 103; `mkdocs.yml` sets `docs_dir: docs/wiki`, so `docs/superpowers/` is **already excluded from the built site**. The review's proposed tree (`user/ sdk/ contributor/ architecture/ internal-history/`) is a full restructure of a 100-file wiki behind a strict link gate. **Do not do it in one commit.** Do the narrow, high-value subset here — the two workflow guides and `CLAUDE.md`'s stale naming — and file the rest.

**What this task explicitly leaves in place.** Step 2's grep will also surface Knowledge Map / RAG-ingestion / auto-memory material in the public wiki. **Do not chase those hits here.** Demoting them is a judgment call about what the wiki is *for*, not a mechanical move, and mixing it into this task makes the diff unreviewable. Record what the grep found in the Step 8 card so the decision is made deliberately rather than by drift.

- [ ] **Step 1: Establish the baseline**

```bash
mise run docs-build
```

Expected: PASS. Do not start from a red gate.

- [ ] **Step 2: Find what the public nav promotes**

```bash
grep -n "Subagent\|subagent\|card-to-done\|superpowers\|RAG\|MEMORY" mkdocs.yml docs/wiki/index.md docs/wiki/coverage.md
```

Record every hit — these are the pages a first-time visitor is offered. Only the `workflow-subagent-discipline` / `card-to-done` hits get acted on in this task; paste the rest into the Step 8 card verbatim.

- [ ] **Step 3: Move the two contributor guides out of the public nav**

```bash
mkdir -p docs/contributor
git mv docs/wiki/guides/workflow-subagent-discipline.md docs/contributor/
git mv docs/wiki/guides/card-to-done-workflow.md docs/contributor/
```

Remove their `nav:` entries from `mkdocs.yml`. Because `docs_dir: docs/wiki`, moving them to `docs/contributor/` takes them out of the built site automatically.

- [ ] **Step 4: Repair every inbound link**

```bash
git grep -n 'workflow-subagent-discipline\|card-to-done-workflow'
```

Handle the hits by where they live — these are **not** the same fix:

- **`CLAUDE.md` and other repo files outside `docs_dir`:** repoint to the new `docs/contributor/...` path. Correct and simple.
- **Pages still inside `docs/wiki/`:** do **not** rewrite these into `../contributor/...` links that escape the MkDocs tree. A relative link out of `docs_dir` either breaks the strict build or renders as a dead link on the published site. A public page should stop referring to internal material, not point at it from a distance — **remove the reference** (or replace it with a plain-prose sentence carrying no link). If a wiki page's content genuinely depends on the moved guide, that page is itself contributor material and belongs in `docs/contributor/` too; move it rather than link across the boundary.
- **`docs/superpowers/`:** historical record. Leave every hit alone, even if now stale.

- [ ] **Step 5: Run the strict gate**

```bash
mise run docs-build
```

Expected: PASS. A dead link or nav orphan fails it — that is the gate doing its job; fix the link rather than loosening the gate.

- [ ] **Step 6: Fix CLAUDE.md's stale naming**

Update the first line's `x3d-cpp-gen` to `x3d-cpp` per the naming model, and repoint the two moved guides. Keep the operational content: it is genuinely useful and this plan is not the place to relitigate it.

- [ ] **Step 7: Commit**

```bash
git add -A docs/ mkdocs.yml CLAUDE.md
git commit -m "docs: remove the subagent/card workflow guides from public navigation

The canonical wiki promoted 'Workflow / Subagent Discipline' as a normal user
guide, which makes the site read as maintainer tooling rather than an SDK. Move
the two agent-workflow guides to docs/contributor/ (outside docs_dir, so they
leave the built site) and fix CLAUDE.md's stale product name.

Scoped deliberately: the remaining Knowledge Map / RAG / auto-memory material in
the public wiki is a separate judgment call, filed as its own card. The full
docs/ restructure is also filed separately -- docs/wiki is 100+ pages behind a
strict link gate and needs its own plan."
```

- [ ] **Step 8: File the two follow-up cards**

On the GitHub Project:

1. **"Decide what public-wiki material is product docs vs development machinery"** — paste the Step 2 grep output (Knowledge Map, RAG ingestion, auto-memory, `docs/superpowers` references). The question is what the wiki is *for*, not where files sit.
2. **"Restructure docs/ into user/ sdk/ contributor/ architecture/ internal-history/"** — note that `docs/superpowers/` is *already* out of the built site, so the driver is navigability, not exposure. Needs its own plan.

---

### Task 14: Add the minimum human-maintainer surface

**Files:**
- Create: `CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`, `CHANGELOG.md`, `.github/PULL_REQUEST_TEMPLATE.md`

**Interfaces:**
- Consumes: Task 1's `SameMinorVersion` decision (the 0.x stability promise).
- Produces: nothing.

**`.github/ISSUE_TEMPLATE/` already exists** (correction 1) with `work-item.yml` + `config.yml` — do **not** recreate it. Read it first and make the PR template consistent with it.

Small and honest. No governance cathedral.

- [ ] **Step 1: Read what already exists**

```bash
cat .github/ISSUE_TEMPLATE/work-item.yml .github/ISSUE_TEMPLATE/config.yml
```

- [ ] **Step 2: Write CONTRIBUTING.md**

Cover exactly: how to build; how to run the fast gate; how to submit a minimal X3D reproduction; what 0.x promises. Point at `docs/contributor/card-to-done-workflow.md` rather than restating it.

```markdown
# Contributing to x3d-cpp

## Build and run the fast gate

```bash
cmake -S . -B build -G Ninja && cmake --build build
```

Contributors use [mise](https://mise.jdx.dev) as a task runner:

```bash
mise run build   # configure + build + ctest (dev preset)
mise run ci      # the full local gate: tests, golden, conformance, build, cli-gate
```

`mise run ci` mirrors the required CI gates (the workflow runs them as separate
jobs, not as this aggregate). Get it green before opening a PR.

## API stability at 0.x

x3d-cpp is pre-1.0. The promise is deliberately narrow:

- **Patch releases (0.1.0 → 0.1.1) are *intended* to preserve API and ABI.**
  The installed CMake package declares `SameMinorVersion` and the shared
  libraries carry `SOVERSION 0.1`, so a consumer that found 0.1.0 accepts 0.1.1
  and is rejected by 0.2.0 at both configure time and load time. This is intent
  backed by convention, **not** a proof: there is no ABI checker in CI yet. If
  you depend on this, pin exactly.
- **Minor releases (0.1.x → 0.2.0) may break both API and ABI.**
- Symbols in `include/x3d/sdk.hpp` marked `[STABLE]` are frozen pre-v2; those
  marked `[EXPERIMENTAL]` may gain fields. See `docs/wiki/seam-status.md`.

## Reporting a bug with a scene

Attach a **minimal** X3D reproduction: the smallest scene that still shows the
problem, with textures and Inlines removed unless they are the bug. State the
encoding (XML / ClassicVRML / VRML97 / JSON), the X3D version, and what you
expected the parser, the tick, or the extraction to do.

For parser or security problems, see [SECURITY.md](SECURITY.md) — do not open a
public issue.

## PR checklist

See `.github/PULL_REQUEST_TEMPLATE.md`. In short: a conformance claim you can
point at, a test that fails without your change, and the docs updated in the
same diff (`mise run docs-drift working` tells you which pages).
```

- [ ] **Step 3: Write SECURITY.md**

The SDK parses untrusted scene files, so this is a real attack surface — the repo already has `fix/parser-dos-hardening` and `fix/doubling-dag-emission-dos` branches and a libFuzzer harness.

```markdown
# Security Policy

## Scope

x3d-cpp parses untrusted input: X3D scene files in four encodings, gzip-compressed
streams, and (via seams) embedder-supplied assets. Parser crashes, unbounded
memory growth, infinite loops, and path traversal in the file-resolution path are
security bugs, not just correctness bugs.

## Reporting a vulnerability

**Do not open a public issue.** Report privately via GitHub's
[private vulnerability reporting](https://github.com/delta9000/x3d-cpp/security/advisories/new).

Include: the input that triggers it (smallest reproduction possible), the
encoding, what happens (crash / hang / OOM / unexpected file access), and the
commit or release you tested.

## What to expect

This is a small project without a paid security team. You will get an
acknowledgement and an honest assessment of whether and when it can be fixed.
Fixes land with a regression test and a `docs/conformance/findings.yaml` entry
where the behavior is spec-relevant.

## Supported versions

Pre-1.0: only the latest release is supported. There are no backports.
```

- [ ] **Step 4: Write CODE_OF_CONDUCT.md**

Use Contributor Covenant 2.1 verbatim, with the maintainer's contact filled in. Do not hand-roll one.

- [ ] **Step 5: Write CHANGELOG.md**

```markdown
# Changelog

Notable changes to x3d-cpp. Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning is [SemVer](https://semver.org) with the 0.x caveats in
[CONTRIBUTING.md](CONTRIBUTING.md#api-stability-at-0x).

## [Unreleased]

### Fixed
- The installed CMake package no longer declares `ARCH_INDEPENDENT` and now uses
  `SameMinorVersion`: it ships compiled shared libraries, so a mismatched
  architecture must be rejected, and at 0.x `SameMajorVersion` treated every
  future 0.y as compatible.
- Shared library `SOVERSION` now tracks `major.minor` instead of `major`, which
  was `0` for every 0.x — the loader would have accepted a 0.2 runtime for a 0.1
  consumer that `find_package` had just refused.
- The generator no longer reports success when the smoke test never ran (missing
  compiler, empty `--compiler`, missing generated main). `--no-test` remains the
  one explicit skip.
- Removed the `--namespace` generator option, which was parsed and then ignored.

### Changed
- Adding x3d-cpp via `add_subdirectory()` no longer writes the ccache launcher
  into the shared cache or defines a global Ninja job pool, so it no longer
  reaches the consumer's own targets. Gated behind `X3D_CPP_ENABLE_DEV_TOOLING`,
  off for subprojects. (`-Wall -Wextra` is unaffected: it is directory-scoped and
  never reached consumers.)
- CMake floor raised to 3.21 (`PROJECT_IS_TOP_LEVEL`).
- The golden formatter is pinned exactly (clang-format 22.1.8) with a committed
  `.clang-format`; the golden gate now fails on a version mismatch instead of
  skipping.
```

- [ ] **Step 6: Write .github/PULL_REQUEST_TEMPLATE.md**

```markdown
## What and why

<!-- One paragraph. Link the card/issue. -->

## Conformance claim

<!-- What does this make true that wasn't? If it changes spec-visible behavior,
     link the docs/conformance/findings.yaml entry. If it makes no conformance
     claim, say so -- "none: packaging metadata only" is a complete answer. -->

## Checklist

- [ ] `mise run ci` is green locally
- [ ] A test fails without this change — **or** N/A with a reason:
      <!-- Docs-only, packaging-metadata and repo-policy changes have no runtime
           surface to test. Say which, e.g. "N/A: wording only, no behavior
           change" or "N/A: CI config; verified by the run on this PR". Do not
           tick this box for a change that COULD have had a test. -->
- [ ] Docs updated in this diff (`mise run docs-drift working` lists candidates)
- [ ] If a third-party dep/backend was added: root `NOTICE` updated
- [ ] If symbols moved: `mise run code-ingest` / `mise run docs-ingest` refreshed
```

The N/A path is not a loophole — it is load-bearing. Several commits in *this very plan* (the golden wording sweep, the naming split, the README install path) legitimately have no test that could fail, and a checklist that forces a lie on its first use trains people to ignore it.

- [ ] **Step 7: Verify**

```bash
mise run docs-build && git grep -n 'sandbrookvt' -- CONTRIBUTING.md SECURITY.md CHANGELOG.md
```

Expected: docs build clean; no wrong-owner URLs (the security link must point at `delta9000/x3d-cpp`).

- [ ] **Step 8: Commit**

```bash
git add CONTRIBUTING.md SECURITY.md CODE_OF_CONDUCT.md CHANGELOG.md \
        .github/PULL_REQUEST_TEMPLATE.md
git commit -m "docs: add the minimum maintainer surface

CONTRIBUTING (build, fast gate, minimal X3D repro, the 0.x stability promise),
SECURITY (the parser eats untrusted input; report privately), CODE_OF_CONDUCT
(Contributor Covenant 2.1), CHANGELOG, and a PR checklist tied to the existing
conformance/docs/NOTICE gates.

.github/ISSUE_TEMPLATE/ already existed and is unchanged."
```

---

### Task 15: Make CI visible and reproducible

**Files:**
- Modify: `.github/workflows/ci.yml:37-39` (triggers), all `uses:` lines (SHA pins)
- Modify: `mise.toml:17` (`uv = "latest"`)
- Modify: `README.md:1-2` (badges)

**Interfaces:**
- Consumes: Task 5's clang-format pin, Task 8's CI-trigger wording.
- Produces: a trustworthy CI badge for `main`.

- [ ] **Step 1: Add the push trigger**

`.github/workflows/ci.yml:37-39`:

```yaml
on:
  push:
    branches: [main]
  pull_request:
  workflow_dispatch:
```

The existing `concurrency` group cancels superseded in-flight runs, so rapid pushes to a PR branch don't each pay full freight. It does **not** make this free: a post-merge run on `main` is genuine additional work per merge. It buys a trustworthy badge and a tested head, which is worth the spend — but do not tell yourself it costs nothing.

Update the file's header comment (line 3) to say "On every PR, on pushes to main, and on manual dispatch".

- [ ] **Step 2: Pin uv — in BOTH places**

Pinning `mise.toml` alone **does not reach CI.** CI installs uv with `astral-sh/setup-uv` (`ci.yml:96,112,126,140`), which does not read `mise.toml`: its inputs are `version`, `version-file`, `python-version`, … With no `version`, it installs latest, so `mise.toml` would pin developers while CI floated — the two would drift silently, which is worse than both floating.

First find the version actually in use (do not invent one):

```bash
uv --version
```

At the time of writing this is `uv 0.10.4`. Then pin `mise.toml:17`, replacing `uv = "latest"` and its now-wrong comment:

```toml
# uv provides the Python toolchain / project env. Pinned: `latest` means a silent
# uv upgrade can change resolution and break a previously-green checkout. CI
# pins the SAME version via setup-uv's `version:` input -- setup-uv does not read
# this file, so both sites must be updated together.
uv = "0.10.4"
```

And pin **every** `astral-sh/setup-uv` step in `.github/workflows/ci.yml`:

```yaml
      - name: Install uv
        uses: astral-sh/setup-uv@<sha>  # v5
        with:
          version: "0.10.4"
```

The alternative single-source-of-truth is `setup-uv`'s `version-file` input pointed at a `required-version` in `pyproject.toml`/`uv.toml`; that removes the duplication but adds a config surface. Either is defensible — **two unpinned-in-CI sites is not.** If you take the two-site route, add the cross-reference comment above so the next person updates both.

Verify:

```bash
mise install && uv --version && uv sync && uv run pytest -q
```

Expected: `uv 0.10.4`, and the suite passes.

- [ ] **Step 3: Pin every action by commit SHA**

For each `uses:` in `.github/workflows/ci.yml` (`actions/checkout@v4`, `astral-sh/setup-uv@v5`, `actions/cache@v4`, and the `jdx/mise-action@v2` added in Task 5), resolve the tag to a commit SHA and pin it with the tag in a trailing comment:

```bash
gh api repos/actions/checkout/git/ref/tags/v4 --jq .object.sha
gh api repos/astral-sh/setup-uv/git/ref/tags/v5 --jq .object.sha
gh api repos/actions/cache/git/ref/tags/v4 --jq .object.sha
gh api repos/jdx/mise-action/git/ref/tags/v2 --jq .object.sha
```

If a tag resolves to a tag object rather than a commit, dereference it:

```bash
gh api repos/actions/checkout/git/tags/<sha> --jq .object.sha
```

Apply as:

```yaml
      - uses: actions/checkout@<sha>  # v4
```

- [ ] **Step 4: Verify no floating refs survive**

A `grep` for `@v[0-9]` only catches `@v4`-style tags. It passes clean on `@main`, `@master`, `@stable` or any arbitrary tag while claiming "every ref is a SHA" — the assertion has to be **positive** (reject anything that is not 40 hex), not a blocklist of the shapes you thought of:

```bash
uv run python - <<'PY'
import re, sys
from pathlib import Path

wf = Path(".github/workflows/ci.yml")
# Match the ref of every `uses:` line, ignoring local (./) and docker:// refs.
pat = re.compile(r'^\s*-?\s*uses:\s*([^\s#]+)', re.M)
bad = []
for ref in pat.findall(wf.read_text()):
    if ref.startswith(("./", "docker://")):
        continue
    _, sep, version = ref.partition("@")
    if not sep or not re.fullmatch(r"[0-9a-f]{40}", version):
        bad.append(ref)

if bad:
    print("UNPINNED actions (must be a 40-hex commit SHA):", file=sys.stderr)
    for b in bad:
        print(f"  {b}", file=sys.stderr)
    sys.exit(1)
print("All actions pinned to 40-hex commit SHAs.")
PY
```

Expected: `All actions pinned to 40-hex commit SHAs.`

Sanity-check the check itself: temporarily revert one `uses:` to `@v4` and confirm it exits 1 naming that action. A verifier that cannot fail is the same trap as a contract test that cannot fail.

- [ ] **Step 5: Add badges**

`README.md`, immediately under the `# x3d-cpp` title:

```markdown
[![CI](https://github.com/delta9000/x3d-cpp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/delta9000/x3d-cpp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
```

The CI badge is only honest **after** Step 1 — without a push trigger it reports nothing for `main`. The release badge is deliberately not here: it is added in the POST-MERGE section, once a release actually exists. A badge reading "no releases" is worse than no badge.

- [ ] **Step 6: Verify the workflow still parses**

```bash
gh workflow view ci.yml 2>&1 | head -5
```

Expected: the workflow lists without a parse error. Push the branch and confirm the run is triggered and green before merging — a SHA pin typo fails at job start.

- [ ] **Step 7: Commit**

```bash
git add .github/workflows/ci.yml mise.toml README.md
git commit -m "ci: run on main, pin every action by SHA, pin uv

The workflow ran only on pull_request and workflow_dispatch, so main's actual
head was never tested and a badge could not be trusted. Add a push trigger for
main -- this costs one extra run per merge, and buys a tested head -- pin every
action to a commit SHA rather than a floating tag, pin uv in both mise.toml and
setup-uv (which does not read mise.toml), and add CI, license and C++20 badges."
```

---

---

# POST-MERGE — publish v0.1.0

**Not part of any of the four PRs.** A release is an operation on a merged, green `main`, not a task inside a cleanup branch. Do this only after PR 4 is merged.

The ordering matters and is easy to get backwards: **the tag must point at a commit whose `CHANGELOG.md` already says `0.1.0`.** Tagging first and then rewriting the changelog leaves the published tag pointing at a tree that still says `[Unreleased]` — the one artifact people read to find out what a release contains.

- [ ] **Step 1: Finalize the changelog first**

In `CHANGELOG.md`, rename the `## [Unreleased]` heading to `## [0.1.0] - 2026-07-<dd>` (the real date) and open a fresh empty `## [Unreleased]` above it.

- [ ] **Step 2: Commit and merge that**

```bash
git add CHANGELOG.md
git commit -m "docs: cut the 0.1.0 changelog section"
```

Open it as a small PR, merge it, and let CI go green on `main`.

- [ ] **Step 3: Verify main is green at the exact commit you will tag**

```bash
git checkout main && git pull
gh run list --branch main --limit 1
git log -1 --oneline
```

Expected: the most recent `main` run is `completed / success`, and `HEAD` is the changelog-finalizing commit. Do not tag a commit whose CI has not finished.

- [ ] **Step 4: Tag that exact commit**

```bash
git tag -a v0.1.0 -m "x3d-cpp 0.1.0"
git push origin v0.1.0
```

- [ ] **Step 5: Create the release from the finalized section**

```bash
gh release create v0.1.0 --title "x3d-cpp 0.1.0" \
  --notes "$(sed -n '/^## \[0.1.0\]/,/^## \[/p' CHANGELOG.md | sed '$d')"
```

Verify the rendered notes on the release page before announcing it — the `sed` range is fragile if the heading format drifts.

- [ ] **Step 6: Add the release badge**

Now that a release exists, add it beside the badges from Task 15 Step 5:

```markdown
[![Release](https://img.shields.io/github/v/release/delta9000/x3d-cpp)](https://github.com/delta9000/x3d-cpp/releases/latest)
```

A badge reading "no releases" is worse than no badge, which is why this waits until here.

---

## Verification: the whole thing

After each phase:

```bash
mise run ci          # tests + golden + conformance-gate + coverage-gate + build + cli-gate-regression
mise run docs-build  # strict: dead links, nav orphans
mise run validate-examples   # the two out-of-SDK renderer consumers `mise run ci` never compiles
```

`validate-examples` is **not** in the `mise run ci` aggregate (tracked as BLD-3) — Tasks 6, 7 and 11 change public surface (naming, `sdk.hpp`, CMake options) that those consumers depend on, so run it by hand before opening the PR.

Then, per the project's anti-drift discipline:

```bash
mise run docs-drift working   # review CITES hits first
mise run code-ingest && mise run docs-ingest   # only if symbols moved
```

## Self-review notes

- **Deliberately not doing:** the full `docs/` restructure (Task 13 does the narrow subset and files the rest); moving `-Wall -Wextra` to `target_compile_options` (Task 11 files it — measured *not* to be a leak, so it is hygiene, not a fix); generating the capability table from a structured source (Task 8 files it); `--test-if-available` (Task 3 explains why not); an ABI checker (Task 1 files it).
- **Task order matters in three places:** Task 11 must bump the CMake floor before relying on `PROJECT_IS_TOP_LEVEL`; Task 5's `.clang-format` and `--style=file:` must land in the same commit or the golden gate breaks permanently; the release must finalize `CHANGELOG.md` *before* tagging, not after.
- **Tasks 6, 7 and 8 all touch `docs/wiki/architecture.md:17`.** Whichever lands second must re-read the line rather than apply its diff blind.
- **Two pins have two sites each.** clang-format: `mise.toml` + `check_golden.sh` + `tests/test_formatter_pin.py`. uv: `mise.toml` + every `setup-uv` step (it does not read `mise.toml`). Updating one site and not the other is the failure mode both pins exist to prevent.
- **Three fixtures have no natural red state** (Task 12's target contract, Task 1's version contract at Case 1, Task 11's isolation check if ccache is absent). Each carries an explicit step to prove it can fail. Do not skip those steps: a green contract test that cannot go red is worse than no test, because it is cited as evidence.

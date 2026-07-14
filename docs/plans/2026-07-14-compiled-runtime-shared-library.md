# Compiled Runtime Shared Library Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Compile the high-fan-out hand-written codec, parser, and mesh implementations once into installed shared runtime layers while preserving the public facades, behavior, and all-static compatibility.

**Architecture:** Keep the public `headers -> nodes -> authoring_runtime -> runtime` graph acyclic. Both compiled runtime layers follow the existing shared-node mode (shared by default, all three layers static when `X3D_CPP_SHARED_NODES=OFF`). Move non-template bodies out of public headers subsystem by subsystem, retain templates and declaration-required layout in headers, and stop only after the retained waves exceed the measured 10% affected-group threshold.

**Tech Stack:** C++20, CMake 3.20+, Ninja, GCC/Clang, pytest, doctest, ASan/UBSan, libFuzzer.

---

### Task 1: Pin the configured and installed runtime topology

**Files:**
- Create: `tests/test_runtime_library_topology.py`
- Create: `examples/embed_minimal/authoring.cpp`
- Modify: `examples/embed_minimal/CMakeLists.txt`
- Modify: `scripts/verify_install_embed.sh`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing configured-graph test**

Add a module-scoped CI configuration and helpers mirroring
`tests/test_cmake_aggregates.py`. Assert the default Ninja target inventory has
all three shared artifacts and no static implementation artifact:

```python
def test_compiled_runtime_layers_are_shared_by_default(configured_ci: Path) -> None:
    artifacts = ninja_targets(configured_ci)
    assert any("libx3d_cpp_nodes.so" in target for target in artifacts)
    assert any("libx3d_cpp_authoring_runtime.so" in target for target in artifacts)
    assert any("libx3d_cpp_runtime.so" in target for target in artifacts)
    assert not any("libx3d_cpp_runtime.a" in target for target in artifacts)
```

Configure a second build with `-DX3D_CPP_SHARED_NODES=OFF` and assert all three
implementation artifacts are `.a`, with no `.so` artifact.

Inspect `ninja -t commands x3d_sdk_facade` and assert its final link includes
`libx3d_cpp_runtime`; inspect a new authoring link-contract target and assert it
includes `libx3d_cpp_authoring_runtime` but not `libx3d_cpp_runtime`.

**Step 2: Add the installed authoring consumer**

Create `examples/embed_minimal/authoring.cpp`:

```cpp
#include "x3d/authoring.hpp"

int main() {
  x3d::authoring::X3DDocument doc;
  doc.profile = x3d::authoring::Profile::Interchange;
  auto box = std::make_shared<x3d::nodes::Box>();
  doc.scene.rootNodes.push_back(box);
  return x3d::authoring::XmlWriter{}.writeDocument(doc).empty() ? 1 : 0;
}
```

Add `x3d_embed_authoring`, linked only to `x3d_cpp::authoring`, and make
`verify_install_embed.sh` execute it after the SDK consumer.

**Step 3: Run RED**

Run:

```bash
uv run pytest tests/test_runtime_library_topology.py -q
```

Expected: FAIL because the two runtime artifacts and target linkage do not yet
exist. The installed authoring consumer itself may still link because writer
bodies are currently inline; the graph assertions are the intentional red.

**Step 4: Add the empty acyclic targets**

Create temporary anchor sources under `runtime/compiled/`, then add explicit
targets using the same `_x3d_node_library_type` selected by
`X3D_CPP_SHARED_NODES`:

```cmake
add_library(x3d_cpp_authoring_runtime ${_x3d_node_library_type}
    runtime/compiled/AuthoringAnchor.cpp)
add_library(x3d_cpp::authoring_runtime ALIAS x3d_cpp_authoring_runtime)
target_link_libraries(x3d_cpp_authoring_runtime PUBLIC x3d_cpp::nodes)

add_library(x3d_cpp_runtime ${_x3d_node_library_type}
    runtime/compiled/RuntimeAnchor.cpp)
add_library(x3d_cpp::runtime ALIAS x3d_cpp_runtime)
target_link_libraries(x3d_cpp_runtime PUBLIC x3d_cpp::authoring_runtime)
```

Set `EXPORT_NAME`, C++20, and shared `VERSION`/`SOVERSION`/
`WINDOWS_EXPORT_ALL_SYMBOLS` consistently with `x3d_cpp_nodes`. Install/export
both. Rewire `x3d_cpp_authoring` to authoring runtime and `x3d_cpp` to full
runtime. Add both production targets to the purpose inventory.

**Step 5: Run GREEN and installed consumers**

Run:

```bash
uv run pytest tests/test_runtime_library_topology.py tests/test_cmake_aggregates.py -q
cmake --build build-ci --target x3d_sdk_facade
ctest --test-dir build-ci -R x3d_sdk_facade --output-on-failure
ctest --test-dir build-ci -R x3d_install_embed_smoke --output-on-failure
```

Expected: all pass; the default shared graph and all-static graph both configure.

**Step 6: Commit**

```bash
git add CMakeLists.txt runtime/compiled tests/test_runtime_library_topology.py \
  examples/embed_minimal scripts/verify_install_embed.sh
git commit -m "build: establish compiled runtime shared layers"
```

### Task 2: Compile FieldValueIO and codec writers once

**Files:**
- Create: `runtime/codecs/FieldValueIO.cpp`
- Create: `runtime/codecs/XmlWriter.cpp`
- Create: `runtime/codecs/JsonWriter.cpp`
- Create: `runtime/codecs/VrmlWriter.cpp`
- Create: `runtime/codecs/CanonicalXmlWriter.cpp`
- Modify: corresponding `.hpp` files
- Modify: `runtime/codecs/X3DCodecs.hpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/test_runtime_library_topology.py`

**Step 1: Extend the failing source/link contract**

Read `compile_commands.json` and assert each new codec source is compiled exactly
once into `x3d_cpp_authoring_runtime`. Assert the installed authoring link line
contains that library. Run the focused pytest and observe failure because the
sources are absent.

**Step 2: Split FieldValueIO**

Move every non-template function body from `FieldValueIO.hpp` to
`FieldValueIO.cpp`, removing `inline` from declarations. Keep only
`fmtMatrixF`, `fmtMatrixD`, `parseMatrixF`, and `parseMatrixD` templates inline.
Preserve exception strings, parsing order, numeric formatting, and switch order
byte-for-byte.

**Step 3: Split each writer without changing layout**

For `XmlWriter`, `JsonWriter`, `VrmlWriter`, and `CanonicalXmlWriter`:

1. leave the public class, data members, and full private method declarations in
   the existing header;
2. turn in-class method bodies into declarations;
3. define them as `Writer::method(...)` in the corresponding `.cpp`;
4. move implementation-only includes to the `.cpp` only when the header remains
   self-contained;
5. keep all serialization order and formatting unchanged.

Do not add PImpl and do not merge the four writers into one translation unit.
List all five sources explicitly on `x3d_cpp_authoring_runtime`, then delete
`AuthoringAnchor.cpp` from the target and repository.

**Step 4: Run codec and authoring verification**

Run:

```bash
uv run pytest tests/test_runtime_library_topology.py -q
cmake --build build-ci --target x3d_codecs_tests x3d_sdk_facade
ctest --test-dir build-ci -R 'x3d_(codecs_tests|canonicalize|sdk_facade)' \
  --output-on-failure
ctest --test-dir build-ci -R x3d_install_embed_smoke --output-on-failure
```

Also run `mise run authoring-footprint` after the normal build exists.

**Step 5: Measure and commit**

Record cumulative codec compiler time and the authoring runtime library size.
If codec compilation does not improve, inspect remaining public includes before
retaining the wave.

```bash
git add CMakeLists.txt runtime/codecs tests/test_runtime_library_topology.py
git commit -m "build(codecs): compile writer implementations once"
```

### Task 3: Compile parser front doors and readers once

**Files:**
- Create: `runtime/parse/X3DParse.cpp`
- Create: `runtime/parse/ClassicVrmlReader.cpp`
- Create: `runtime/parse/Vrml97Reader.cpp`
- Create: `runtime/parse/JsonReader.cpp`
- Create: `runtime/parse/XmlReaderAdapter.cpp`
- Create: `runtime/parse/NodeBuilder.cpp`
- Modify: corresponding `.hpp` files
- Modify: `CMakeLists.txt`
- Modify: `tests/test_runtime_library_topology.py`

**Step 1: Write the failing parse link/source assertions**

Extend the topology test so all six parser sources must compile exactly once in
`x3d_cpp_runtime`. Add an assertion that the SDK facade link includes the full
runtime. Run the focused pytest and observe the missing-source failure.

**Step 2: Split the parse front door first**

Move `stripUtf8Bom`, `makeReader`, confinement state, local resolvers,
`parseDocument`, and `parseFile` to `X3DParse.cpp`. Keep only the declarations
and default arguments in `X3DParse.hpp`. Remove concrete reader, filesystem,
stream, algorithm, and inflate includes from the public header when they are no
longer required for declarations.

Compile and run `x3d_sdk_facade`, `x3d_parse_reader`, and parse-front-door tests
before continuing.

**Step 3: Split concrete reader methods**

Move non-template method bodies for `ClassicVrmlReader`, `Vrml97Reader`,
`JsonReader`, and `XmlReaderAdapter` into their `.cpp` peers. Preserve virtual
hooks and class data layout. Forward-declare parser-state types where legal;
retain a header include when a base class, value member, or inline template
requires completeness.

Move non-template `NodeBuilder` helpers to `NodeBuilder.cpp`; retain generic
token/visitor templates inline. Add all six sources explicitly to
`x3d_cpp_runtime`, then remove `RuntimeAnchor.cpp`.

**Step 4: Verify every encoding and the fuzz link**

Run:

```bash
cmake --build build-ci --target x3d_parse_tests x3d_parse_reader \
  x3d_classic_vrml_reader x3d_json_reader x3d_vrml97_reader x3d_sdk_facade
ctest --test-dir build-ci -R \
  'x3d_(parse|classic_vrml|json_reader|vrml97|sdk_facade)' \
  --output-on-failure
cmake --preset fuzz
cmake --build build-fuzz --target x3d_parse_fuzz
```

Expected: identical parser behavior and a fuzz executable linked to
`x3d_cpp_runtime` rather than compiling the front door into the harness.

**Step 5: Measure and commit**

Measure the parse group and the direct SDK probe. Commit only after the SDK
preprocessed-line count and/or affected compilation CPU falls materially.

```bash
git add CMakeLists.txt runtime/parse tests/test_runtime_library_topology.py
git commit -m "build(parse): compile reader implementations once"
```

### Task 4: Compile MeshBuilder once

**Files:**
- Create: `runtime/extract/MeshBuilder.cpp`
- Modify: `runtime/extract/MeshBuilder.hpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/test_runtime_library_topology.py`

**Step 1: Write the failing extraction source contract**

Assert `MeshBuilder.cpp` appears exactly once in the runtime compile commands.
Existing mesh tests already exercise analytic, indexed, elevation-grid, text,
texture-coordinate, external-geometry, and NURBS branches; the new test pins
link ownership rather than duplicating behavior fixtures.

Run the focused test and observe failure because the source is absent.

**Step 2: Split declarations from implementation**

Keep `GeoSystemDesc`, `GeoProjection`, `MeshBuildOptions`, `MeshData`,
`recognizedGeometryType`, and `buildLocalMesh` declarations public. Move
non-template `mesh_detail` helpers, tessellators, normal/UV algorithms, and the
dispatch body to `MeshBuilder.cpp`.

Keep `emitHeightGrid` or another genuinely generic helper inline only if its
template parameter cannot be eliminated cleanly. Preserve operation order and
floating-point expressions exactly; this task must not change mesh bytes.

Move generated concrete-node includes and implementation-only standard headers
to the `.cpp`. Add the source explicitly to `x3d_cpp_runtime`.

**Step 3: Verify extraction, picking, and consumers**

Run:

```bash
cmake --build build-ci --target x3d_extract_tests x3d_geometry_scene_tests \
  x3d_sdk_facade
ctest --test-dir build-ci -R \
  'x3d_(extract_tests|geometry_scene|sdk_facade)' --output-on-failure
scripts/validate-examples.sh
```

Expected: every mesh fixture and picking path is unchanged; CPU raster and PoC
both compile and run.

**Step 4: Measure and commit**

Measure the extraction group and SDK probe. Require a clear reduction because
`MeshBuilder.hpp` is the largest public runtime header.

```bash
git add CMakeLists.txt runtime/extract/MeshBuilder.hpp \
  runtime/extract/MeshBuilder.cpp tests/test_runtime_library_topology.py
git commit -m "build(extract): compile MeshBuilder once"
```

### Task 5: Slim public includes and prove header isolation

**Files:**
- Modify: migrated codec, parse, and extraction headers
- Modify: affected runtime/tests/examples/tools consumers
- Modify: `tests/test_header_contract_shape.py` if an explicit invariant is needed

**Step 1: Run the isolation target before include cleanup**

Run `cmake --build build-ci --target x3d_header_isolation`. Fix no code yet;
this is the post-split self-containment baseline.

**Step 2: Remove implementation-only includes one header at a time**

For each migrated header, remove only dependencies no longer required by its
declarations or retained inline templates. After each small batch, rebuild
`x3d_header_isolation`. If a consumer depended on an accidental transitive
include, add its declaring header to that consumer.

Do not restore a heavy include to conceal a consumer bug and do not forward
declare standard-library types.

**Step 3: Verify and commit**

Run:

```bash
cmake --build build-ci --target x3d_compile_contracts
ctest --test-dir build-ci -L compile-contract --output-on-failure
uv run pytest tests/test_header_contract_shape.py \
  tests/test_runtime_library_topology.py -q
```

```bash
git add runtime tests examples tools
git commit -m "build(runtime): remove implementation-only public includes"
```

### Task 6: Measure the retained waves and make the stop decision

**Files:**
- Create: `docs/plans/2026-07-14-compiled-runtime-shared-library-results.md`

**Step 1: Produce a fresh after build**

Configure a fresh CI build directory and run the same command used for the
baseline:

```bash
env CCACHE_DISABLE=1 /usr/bin/time -v \
  cmake --build <fresh-build> --target x3d_behavior_tests
```

Extract per-object elapsed time from `.ninja_log` using the same grouping rules.
Repeat the syntax/preprocessing probes for SDK, parse, codecs, extraction, and
execution headers.

**Step 2: Record library and disk costs**

Record sizes of both new shared libraries, the node library, normal CI tree,
and sanitizer tree. Compare against the 734 MiB clean normal baseline and the
previous 4.6 GiB sanitizer tree.

**Step 3: Apply the stop condition**

Compute cumulative compiler time across codecs + parse + extraction. Retain the
branch only if those affected groups improve at least 10% without worse peak
RSS. If the threshold is met, defer SceneExtractor/events/scene bodies to a
follow-up PR on the proven boundary. If it is not met, profile one additional
wave (SceneExtractor first) and add it only with its own red link contract.

**Step 4: Commit results**

```bash
git add docs/plans/2026-07-14-compiled-runtime-shared-library-results.md
git commit -m "docs: record compiled runtime measurements"
```

### Task 7: Full verification and publication

**Files:**
- Modify only if a verification failure reveals a scoped defect

**Step 1: Run complete local gates**

Run:

```bash
uv run pytest
cmake --build build-ci --target x3d_behavior_tests x3d_compile_contracts
ctest --test-dir build-ci -L behavior --output-on-failure
ctest --test-dir build-ci -L compile-contract --output-on-failure
cmake --preset san
cmake --build build-san --target x3d_sanitizer_tests
ctest --test-dir build-san -L behavior --output-on-failure
scripts/validate-examples.sh
```

Run the all-static configuration with `X3D_CPP_SHARED_NODES=OFF`, then build and
run SDK, parser, extraction, and installed-consumer gates.

**Step 2: Review the complete diff**

Run `git diff --check`, inspect every public signature change, confirm generated
bindings are untouched, and verify the worktree is clean. Apply
`superpowers:requesting-code-review` within the available collaboration policy.

**Step 3: Publish and monitor**

Push `perf/runtime-shared-library`, open a draft PR against `main`, include
before/after measurements and the compatibility graph, and monitor all GitHub
Actions checks to completion. Fix only failures attributable to this branch;
record unrelated external failures distinctly.

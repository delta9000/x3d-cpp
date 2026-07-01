# Asset-Import Consumer + `x3d_cpp::authoring` Slim Target — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship a headless write-side canonical consumer (`x3d_asset_import`) that converts models (via an `ImportSource` seam; assimp backend) into standards-compliant X3D 4.0, linking a new footprint-gated `x3d_cpp::authoring` slim target.

**Architecture:** A linear pipeline — `ImportSource` backend → `ImportScene` POD IR → `emit` (IR→`X3DDocument`, links *only* `x3d_cpp::authoring`) → self-validate (range-check + profile-fit) → serialize (+ consumer-side texture pipeline writing `assets/`). Backends and the texture pipeline never touch the slim target.

**Tech Stack:** C++20, CMake (Ninja), doctest, mise tasks. Node model = `x3d::nodes` generated bindings (shared_ptr graph). Writers = `x3d::codec::*Writer`. Textures = stb decode seam (`x3d_stb`) + vendored `stb_image_write.h`. Import breadth = assimp (gated, OFF by default). Profile-fit reuses the CLI's UOM-derived tables.

## Global Constraints

- **Design spec:** `docs/superpowers/specs/2026-07-01-asset-import-authoring-target-design.md` — this plan implements it; read §4 (mapping table), §6/§6.1 (slim target + 19777-4 relationship), §11 (scope/deferrals).
- **X3D version:** emit `version="4.0"`. Encodings via `x3d::codec::{XmlWriter,JsonWriter,VrmlWriter,CanonicalXmlWriter}` — each `writer{}.writeDocument(doc)` → `std::string` (no free function).
- **Namespaces:** nodes `x3d::nodes`; value types `x3d::core` (`SFVec3f{x,y,z}`, `SFColor{r,g,b}`, `SFRotation{x,y,z,angle}`, `MFInt32`=`std::vector<int>`, `SFNode`=`std::shared_ptr<nodes::X3DNode>`, `MFNode`=`std::vector<SFNode>`); document/scene/head `x3d::runtime`; codecs `x3d::codec`.
- **Node construction:** `std::make_shared<x3d::nodes::Type>()`; typed setters `setX(const T&)`. **`initializeOnly` fields expose only `setXUnchecked(...)`** (e.g. `IndexedTriangleSet::setIndexUnchecked(const MFInt32&)`, `X3DComposedGeometryNode::setCcwUnchecked/setSolidUnchecked`). If a plain `setX` fails to compile, grep the node header under `generated_cpp_bindings/x3d/nodes/` and use the `...Unchecked` sibling.
- **DEF/USE = shared_ptr identity:** assigning the *same* `shared_ptr` into two slots makes writers emit `DEF` then `USE` automatically.
- **The `emit` translation unit MUST include only `x3d/authoring.hpp`** (no `x3d/sdk.hpp`, no parse/extract/execution headers). This is enforced by the footprint symbol scan (Task 12).
- **Naming guardrails (spec §6.1):** do NOT use `X3DCSAIL`/`X3DCPPSAIL`, `C`/`CX3D` class prefixes, `X3DLib`, `Concretes.h`/`Abstracts.h`, or `web3d::`/`org.web3d.x3d`. Our namespace/target names stay `x3d::authoring` / `x3d_cpp::authoring`.
- **Non-disparagement:** in any committed artifact (code comments, docs, commit messages), describe assimp/19777-4/other projects factually; no judgmental adjectives.
- **NOTICE is DoD:** adding assimp (and confirming `stb_image_write`) requires updating the root `NOTICE` in the same change (Task 14).
- **Test naming:** every ctest is `add_test(NAME x3d_...)`; the asset-import group prefix is `x3d_assetimport_`.
- **Build dirs:** dedicated `build-asset-import/` (mirrors `build-cpuraster/`), keeping the dev `build/` ccache warm.
- **Commit conventions:** no `Claude-Session:` trailers or session URLs; tool-agnostic messages.

---

## File Structure

**New library surface**
- `include/x3d/authoring.hpp` — slim umbrella header (authoring subset only).
- top `CMakeLists.txt` — new options `X3D_CPP_BUILD_ASSET_IMPORT`, `X3D_CPP_BUILD_ASSIMP`; `x3d_cpp::authoring` INTERFACE target; `add_subdirectory(examples/asset_import)`; footprint smoke target/test.

**Profile-fit promotion**
- `tools/x3d-cli/profile_fit.hpp` — extracted `namespace profile_fit` (was inline in `tools/x3d_cli.cpp`), `#include`s the existing `*.gen.inc` tables.
- `tools/x3d_cli.cpp` — refactored to `#include "x3d-cli/profile_fit.hpp"` (behavior-preserving).

**The consumer** — `examples/asset_import/`
- `import_source.hpp` — IR structs + abstract `ImportSource` (header-only).
- `fixture_source.hpp` / `.cpp` — dependency-free synthetic backend.
- `assimp_source.hpp` / `.cpp` — assimp backend (compiled only when `X3D_CPP_BUILD_ASSIMP=ON`).
- `emit.hpp` / `.cpp` — `ImportScene` → `X3DDocument` (links only `x3d_cpp::authoring`).
- `texture_pipeline.hpp` / `.cpp` — decode/encode/dedup/write `assets/` + URL rewrite.
- `main.cpp` — CLI orchestration (backend select, emit, profile-fit, range-validate, write, `--verify`).
- `third_party/stb_image_write.h` — vendored PNG encoder (consumer-owned).
- `tests/` — `emit_test.cpp`, `texture_pipeline_test.cpp`, `fixture_source_test.cpp`, `roundtrip_test.cpp`, `assimp_source_test.cpp` (gated).
- `assets/fixtures/` — tiny committed `.obj` (+ `.mtl`) for the assimp test; canonical-form goldens.
- `footprint/authoring_smoke.cpp` — minimal main including only `x3d/authoring.hpp`.
- `CMakeLists.txt` — target wiring.

**Gate + docs**
- `scripts/authoring-footprint.sh` — build smoke, `nm` symbol scan, size-baseline check.
- `examples/asset_import/footprint/baseline.tsv` — committed footprint baseline.
- `scripts/validate-examples.sh` — extended to build+probe asset_import (with/without assimp).
- `mise.toml` — `asset-import`, `authoring-footprint` tasks; `authoring-footprint` added to `ci` deps.
- `docs/wiki/subsystems/asset-import.md`, `docs/wiki/coverage.md`, `mkdocs.yml`, `docs/wiki/decisions/NNNN-authoring-target.md`, `docs/sdk/v1-capabilities.md`, `NOTICE`.

---

## Task 1: `x3d_cpp::authoring` slim target + umbrella header

**Files:**
- Create: `include/x3d/authoring.hpp`
- Create: `examples/asset_import/footprint/authoring_smoke.cpp`
- Modify: `CMakeLists.txt` (add INTERFACE target + smoke exe + test, near the seam-lib block ~line 556 and test region)

**Interfaces:**
- Produces: CMake target `x3d_cpp::authoring` (INTERFACE, links `x3d_cpp::nodes`, adds authoring include dirs). Umbrella header exposing `x3d::codec::{XmlWriter,JsonWriter,VrmlWriter,CanonicalXmlWriter}`, `x3d::runtime::{X3DDocument,Scene,Head,Component,Unit,Meta,Route,Profile}`, `x3d::nodes::*`, `x3d::core::*`, and `collectRangeWarnings` (from `runtime/X3DRangeValidate.hpp`).

- [ ] **Step 1: Write the umbrella header**

`include/x3d/authoring.hpp`:
```cpp
// include/x3d/authoring.hpp
// Slim authoring surface: construct an X3D scene graph, range-validate it, and
// serialize it. Deliberately excludes parse, execution context, extraction,
// physics, script, sound, and IO seams. See
// docs/superpowers/specs/2026-07-01-asset-import-authoring-target-design.md §6.
#ifndef X3D_AUTHORING_HPP
#define X3D_AUTHORING_HPP

#include "X3DDocument.hpp"          // x3d::runtime — X3DDocument/Scene/Head/Profile/Route
#include "X3DRangeValidate.hpp"     // ::collectRangeWarnings + x3d::core::RangeDiagnostic
#include "codecs/X3DCodecs.hpp"     // x3d::codec — Xml/Json/Vrml/CanonicalXml writers
#include "x3d/nodes/X3DNodeFactory.hpp" // x3d::nodes — every node type + factory

namespace x3d::authoring {
// Re-export the curated authoring surface (same symbols as x3d::sdk, minus runtime).
using x3d::runtime::X3DDocument;
using x3d::runtime::Scene;
using x3d::runtime::Head;
using x3d::runtime::Component;
using x3d::runtime::Unit;
using x3d::runtime::Meta;
using x3d::runtime::Route;
using x3d::runtime::Profile;
using x3d::codec::XmlWriter;
using x3d::codec::JsonWriter;
using x3d::codec::VrmlWriter;
using x3d::codec::CanonicalXmlWriter;
} // namespace x3d::authoring

#endif // X3D_AUTHORING_HPP
```
Note: confirm the exact include path for the nodes umbrella (grep `generated_cpp_bindings/x3d/nodes/` for a header that pulls all node types, or include `x3d/nodes/X3DNodeFactory.hpp` plus specific node headers as needed). Node headers are reachable bare-name via the `x3d_cpp` include dirs.

- [ ] **Step 2: Write the footprint smoke source**

`examples/asset_import/footprint/authoring_smoke.cpp`:
```cpp
// Minimal authoring embed: builds a one-Shape doc and serializes it, using ONLY
// x3d/authoring.hpp. The footprint gate (Task 12) scans this binary's symbols.
#include "x3d/authoring.hpp"
#include <iostream>

int main() {
  namespace a = x3d::authoring;
  a::X3DDocument doc;
  doc.version = "4.0";
  doc.profile = a::Profile::Interchange;
  auto shape = std::make_shared<x3d::nodes::Shape>();
  shape->setGeometry(std::make_shared<x3d::nodes::Box>());
  doc.scene.addRootNode(shape);
  const std::string xml = a::XmlWriter{}.writeDocument(doc);
  std::cout << xml.size() << "\n";
  return xml.empty() ? 1 : 0;
}
```

- [ ] **Step 3: Add the CMake target + smoke exe + test**

In top `CMakeLists.txt`, after the `x3d_cpp_nodes` target (~line 151), add:
```cmake
# ── Authoring slim target: node reflection + writers + range-validate only ──
add_library(x3d_cpp_authoring INTERFACE)
add_library(x3d_cpp::authoring ALIAS x3d_cpp_authoring)
target_link_libraries(x3d_cpp_authoring INTERFACE x3d_cpp::nodes)
target_include_directories(x3d_cpp_authoring INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime"
    "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings")
target_compile_features(x3d_cpp_authoring INTERFACE cxx_std_20)
install(TARGETS x3d_cpp_authoring EXPORT x3d_cppTargets)
```
Then, guarded by the example flag (added in Task 11, but the smoke test can live under `X3D_CPP_BUILD_ASSET_IMPORT`), register the smoke test:
```cmake
if(X3D_CPP_BUILD_ASSET_IMPORT)
  add_executable(x3d_authoring_smoke
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/asset_import/footprint/authoring_smoke.cpp")
  target_link_libraries(x3d_authoring_smoke PRIVATE x3d_cpp::authoring)
  add_test(NAME x3d_assetimport_authoring_smoke COMMAND x3d_authoring_smoke)
endif()
```
(For this task, temporarily add `option(X3D_CPP_BUILD_ASSET_IMPORT "Build the asset-import consumer" OFF)` near the other options ~line 254; Task 11 finalizes the subdirectory wiring.)

- [ ] **Step 4: Configure + build + run the smoke test**

Run:
```bash
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON
cmake --build build-asset-import --target x3d_authoring_smoke
ctest --test-dir build-asset-import -R x3d_assetimport_authoring_smoke --output-on-failure
```
Expected: builds clean, test PASS (prints a nonzero byte count). If a nodes umbrella include path is wrong, fix the include in `authoring.hpp` and rebuild.

- [ ] **Step 5: Commit**

```bash
git add include/x3d/authoring.hpp examples/asset_import/footprint/authoring_smoke.cpp CMakeLists.txt
git commit -m "feat(authoring): add x3d_cpp::authoring slim target + umbrella header"
```

---

## Task 2: Promote `profile_fit` into a reusable header

**Files:**
- Create: `tools/x3d-cli/profile_fit.hpp`
- Modify: `tools/x3d_cli.cpp` (replace the inline `namespace profile_fit { ... }` with an include)

**Interfaces:**
- Produces: header `x3d-cli/profile_fit.hpp` exposing `namespace profile_fit`: `struct ProfileDef`, `using ComponentUsage = std::map<std::string,int>`, `ComponentUsage sceneComponentUsage(const x3d::runtime::Scene&)`, `const ProfileDef* findMinimalProfile(const ComponentUsage&)`. Reused by `main.cpp` (Task 11).

- [ ] **Step 1: Extract the namespace into the header**

Move the entire `namespace profile_fit { ... }` block (currently `tools/x3d_cli.cpp:288-437`, including the `#include "x3d-cli/node_component_table.gen.inc"` / `profile_defs.gen.inc` usages) verbatim into `tools/x3d-cli/profile_fit.hpp`, wrapped in an include guard, with the needed standard headers (`<map>`, `<string>`, `<vector>`, `<unordered_map>`, `<unordered_set>`) and `#include "X3DDocument.hpp"` for `x3d::runtime::Scene`/`Profile`. Keep the `.gen.inc` includes pointing at their current relative paths (co-located under `tools/x3d-cli/`), so `mise run gen` output paths are unchanged.

- [ ] **Step 2: Replace the inline block in the CLI with an include**

In `tools/x3d_cli.cpp`, delete the moved block and add near the other includes:
```cpp
#include "x3d-cli/profile_fit.hpp"
```
Leave all call sites (`profile_fit::sceneComponentUsage`, `profile_fit::findMinimalProfile`, etc.) unchanged.

- [ ] **Step 3: Build the CLI + run the CLI differential gate (behavior-preserving)**

Run:
```bash
mise run build            # builds x3d_cli among targets
mise run cli-gate-regression
mise run golden
```
Expected: all PASS with no divergence (pure refactor — no verdict/profile output should change).

- [ ] **Step 4: Commit**

```bash
git add tools/x3d-cli/profile_fit.hpp tools/x3d_cli.cpp
git commit -m "refactor(cli): promote profile_fit into a reusable header"
```

---

## Task 3: `ImportScene` IR + abstract `ImportSource`

**Files:**
- Create: `examples/asset_import/import_source.hpp`
- Create: `examples/asset_import/tests/fixture_source_test.cpp` (compile-only stub this task; fleshed out in Task 4)

**Interfaces:**
- Produces: the POD IR + `struct ImportSource { virtual ImportScene load(const std::string& path) = 0; virtual ~ImportSource() = default; };`. Consumed by every backend (Tasks 4, 10) and `emit` (Tasks 5-9).

- [ ] **Step 1: Write the IR header**

`examples/asset_import/import_source.hpp`:
```cpp
#ifndef X3D_ASSET_IMPORT_IMPORT_SOURCE_HPP
#define X3D_ASSET_IMPORT_IMPORT_SOURCE_HPP
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace x3d::asset_import {

struct Vec2 { float x = 0, y = 0; };
struct Vec3 { float x = 0, y = 0, z = 0; };
struct Vec4 { float x = 0, y = 0, z = 0, w = 0; };
// Column-major 4x4 (backend converts into this convention). Identity default.
struct Mat4 { std::array<float, 16> m{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; };

enum class AlphaMode { Opaque, Mask, Blend };

struct PbrParams { Vec4 baseColor{1,1,1,1}; float metallic = 1.0f; float roughness = 1.0f; };

// A texture reference: either an external path (relative to the model dir) OR an
// index into ImportScene::embedded. Exactly one is set.
struct TextureRef {
  std::optional<std::string> externalPath;
  std::optional<int> embeddedIndex;
};
struct TextureSlots {
  std::optional<TextureRef> baseColor;   // diffuse / baseColor
  std::optional<TextureRef> normal;
  std::optional<TextureRef> emissive;
  std::optional<TextureRef> occlusion;
  std::optional<TextureRef> metallicRoughness; // PBR
  std::optional<TextureRef> specular;          // Phong
};

struct ImportMesh {
  std::vector<Vec3> positions, normals;
  std::vector<Vec2> uv;
  std::vector<Vec4> colors;
  std::vector<std::uint32_t> indices;   // triangles (multiples of 3)
  int materialIndex = -1;
};
struct ImportMaterial {
  std::string name;
  Vec3 diffuse{0.8f, 0.8f, 0.8f}, emissive{0, 0, 0}, specular{0, 0, 0};
  float shininess = 0.0f;    // assimp SHININESS (0..~1000)
  float opacity = 1.0f;
  AlphaMode alpha = AlphaMode::Opaque;
  std::optional<PbrParams> pbr;
  TextureSlots textures;
};
struct ImportNode {
  std::string name;
  Mat4 localTransform;
  std::vector<int> meshIndices;
  std::vector<int> childIndices;
};
struct ImportCamera { Mat4 world; float yfov = 0.7854f, znear = 0.1f, zfar = 1000.0f; };
struct ImportLight {
  enum class Kind { Dir, Point, Spot } kind = Kind::Dir;
  Vec3 color{1, 1, 1}, position{0, 0, 0}, direction{0, 0, -1};
  float intensity = 1.0f, cutOffAngle = 0.785398f, beamWidth = 1.570796f, radius = 100.0f;
  Vec3 attenuation{1, 0, 0};
};
struct EmbeddedTexture { std::string key, hintExt; std::vector<std::uint8_t> bytes; };

struct ImportScene {
  std::vector<ImportNode> nodes;
  int rootNode = -1;
  std::vector<ImportMesh> meshes;
  std::vector<ImportMaterial> materials;
  std::vector<ImportCamera> cameras;
  std::vector<ImportLight> lights;
  std::vector<EmbeddedTexture> embedded;
};

struct ImportSource {
  virtual ImportScene load(const std::string& path) = 0;
  virtual ~ImportSource() = default;
};

} // namespace x3d::asset_import
#endif
```

- [ ] **Step 2: Write a compile-only test stub**

`examples/asset_import/tests/fixture_source_test.cpp`:
```cpp
#include "import_source.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;

TEST_CASE("ir_default_scene_is_empty") {
  ImportScene s;
  CHECK(s.rootNode == -1);
  CHECK(s.meshes.empty());
}
```

- [ ] **Step 3: Wire a temporary test target + build**

Add to `examples/asset_import/CMakeLists.txt` (create it; expanded in Task 11):
```cmake
add_executable(x3d_assetimport_fixture_source
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support/doctest_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixture_source_test.cpp")
target_include_directories(x3d_assetimport_fixture_source PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/test_support")
target_compile_features(x3d_assetimport_fixture_source PRIVATE cxx_std_20)
add_test(NAME x3d_assetimport_fixture_source COMMAND x3d_assetimport_fixture_source)
```
And in top `CMakeLists.txt` replace the temporary smoke block with `add_subdirectory(examples/asset_import)` under `if(X3D_CPP_BUILD_ASSET_IMPORT)` (move the smoke target into the subdir CMake).

Run:
```bash
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON
ctest --test-dir build-asset-import -R x3d_assetimport_fixture_source --output-on-failure
```
Expected: PASS.

- [ ] **Step 4: Commit**

```bash
git add examples/asset_import/import_source.hpp examples/asset_import/tests/fixture_source_test.cpp examples/asset_import/CMakeLists.txt CMakeLists.txt
git commit -m "feat(asset-import): ImportScene IR + ImportSource seam"
```

---

## Task 4: `FixtureSource` backend

**Files:**
- Create: `examples/asset_import/fixture_source.hpp`, `examples/asset_import/fixture_source.cpp`
- Modify: `examples/asset_import/tests/fixture_source_test.cpp`
- Modify: `examples/asset_import/CMakeLists.txt`

**Interfaces:**
- Consumes: `ImportSource`, `ImportScene` (Task 3).
- Produces: `class FixtureSource : public ImportSource`. `load("cube")`, `load("hierarchy")`, `load("lit")` return synthetic scenes. Used by `main` (`fixture:<name>`) and all emit tests.

- [ ] **Step 1: Write the failing test**

Replace `fixture_source_test.cpp` body:
```cpp
#include "fixture_source.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;

TEST_CASE("fixture_cube_has_one_mesh_and_material") {
  FixtureSource src;
  ImportScene s = src.load("cube");
  CHECK(s.meshes.size() == 1);
  CHECK(s.meshes[0].indices.size() == 36);      // 12 triangles
  CHECK(s.meshes[0].positions.size() == 24);     // 6 faces * 4 verts
  CHECK(s.materials.size() == 1);
  CHECK(s.rootNode >= 0);
}
TEST_CASE("fixture_lit_has_camera_and_lights") {
  FixtureSource src;
  ImportScene s = src.load("lit");
  CHECK(s.cameras.size() >= 1);
  CHECK(s.lights.size() >= 1);
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `ctest --test-dir build-asset-import -R x3d_assetimport_fixture_source --output-on-failure`
Expected: FAIL (compile error — `fixture_source.hpp` missing).

- [ ] **Step 3: Implement the backend**

`fixture_source.hpp`:
```cpp
#ifndef X3D_ASSET_IMPORT_FIXTURE_SOURCE_HPP
#define X3D_ASSET_IMPORT_FIXTURE_SOURCE_HPP
#include "import_source.hpp"
namespace x3d::asset_import {
class FixtureSource : public ImportSource {
public:
  ImportScene load(const std::string& name) override; // "cube" | "hierarchy" | "lit"
};
} // namespace x3d::asset_import
#endif
```
`fixture_source.cpp` — build a unit cube (24 verts, per-face normals, 36 indices, one diffuse material) for `"cube"`; a two-node parent/child with two materials for `"hierarchy"`; and a cube + one `ImportCamera` + one directional `ImportLight` for `"lit"`. Populate `positions/normals/uv/indices`, set `rootNode`, one `ImportNode` whose `meshIndices={0}`. Throw `std::runtime_error("unknown fixture: " + name)` on unknown names. (Standard unit-cube data — 6 faces, CCW winding.)

- [ ] **Step 4: Wire the source into the test target + build + pass**

Add `fixture_source.cpp` to the `x3d_assetimport_fixture_source` sources in the subdir CMake.
Run: `cmake --build build-asset-import && ctest --test-dir build-asset-import -R x3d_assetimport_fixture_source --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add examples/asset_import/fixture_source.hpp examples/asset_import/fixture_source.cpp examples/asset_import/tests/fixture_source_test.cpp examples/asset_import/CMakeLists.txt
git commit -m "feat(asset-import): FixtureSource synthetic backend"
```

---

## Task 5: `emit` — geometry core (IR → X3DDocument)

**Files:**
- Create: `examples/asset_import/emit.hpp`, `examples/asset_import/emit.cpp`
- Create: `examples/asset_import/tests/emit_test.cpp`
- Modify: `examples/asset_import/CMakeLists.txt`

**Interfaces:**
- Consumes: `ImportScene` (Task 3), `x3d/authoring.hpp` (Task 1).
- Produces: `x3d::runtime::X3DDocument emit(const ImportScene&, const EmitOptions&)`. `struct EmitOptions { bool includeTextures = true; };`. Later tasks extend the same function.

- [ ] **Step 1: Write the failing test**

`emit_test.cpp`:
```cpp
#include "emit.hpp"
#include "fixture_source.hpp"
#include "x3d/authoring.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;

TEST_CASE("emit_cube_produces_shape_with_indexed_triangle_set") {
  ImportScene s = FixtureSource{}.load("cube");
  auto doc = emit(s, {});
  CHECK(doc.version == "4.0");
  CHECK_FALSE(doc.scene.rootNodes.empty());
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(doc);
  CHECK(xml.find("<Shape") != std::string::npos);
  CHECK(xml.find("<IndexedTriangleSet") != std::string::npos);
  CHECK(xml.find("<Coordinate") != std::string::npos);
  CHECK(xml.find("<Normal") != std::string::npos);
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `ctest --test-dir build-asset-import -R x3d_assetimport_emit --output-on-failure`
Expected: FAIL (compile error — `emit.hpp` missing).

- [ ] **Step 3: Implement the geometry path**

`emit.hpp`:
```cpp
#ifndef X3D_ASSET_IMPORT_EMIT_HPP
#define X3D_ASSET_IMPORT_EMIT_HPP
#include "import_source.hpp"
#include "x3d/authoring.hpp"
namespace x3d::asset_import {
struct EmitOptions { bool includeTextures = true; };
x3d::runtime::X3DDocument emit(const ImportScene& scene, const EmitOptions& opts);
} // namespace x3d::asset_import
#endif
```
`emit.cpp` — for each `ImportMesh`, build a `Shape`: geometry = `IndexedTriangleSet` with `setIndexUnchecked(MFInt32{indices...})`, `setCoord(Coordinate)` (`setPoint(MFVec3f{...})`), `setNormal(Normal)` when `normals` present, `setTexCoord(TextureCoordinate)` when `uv` present, `setColor(ColorRGBA)` when `colors` present. Set `setSolidUnchecked(false)`, `setCcwUnchecked(true)` (grep the header for exact names). Wrap each root `ImportNode` in a `Transform` holding its mesh Shapes as children; `doc.scene.addRootNode(rootTransform)`. Set `doc.version="4.0"`, `doc.profile=Profile::Interchange`. Convert IR `Vec3` → `x3d::core::SFVec3f{v.x,v.y,v.z}` etc.
```cpp
// emit.cpp (sketch of the geometry helper)
#include "emit.hpp"
using namespace x3d::core; using namespace x3d::nodes;
static SFNode buildMesh(const ImportMesh& m) {
  auto its = std::make_shared<IndexedTriangleSet>();
  MFInt32 idx(m.indices.begin(), m.indices.end());
  its->setIndexUnchecked(idx);
  auto coord = std::make_shared<Coordinate>();
  MFVec3f pts; pts.reserve(m.positions.size());
  for (auto& p : m.positions) pts.push_back(SFVec3f{p.x, p.y, p.z});
  coord->setPoint(pts);
  its->setCoord(coord);
  if (!m.normals.empty()) { auto n = std::make_shared<Normal>();
    MFVec3f nv; for (auto& v : m.normals) nv.push_back(SFVec3f{v.x, v.y, v.z});
    n->setVector(nv); its->setNormal(n); }
  // texCoord + color analogous (TextureCoordinate::setPoint(MFVec2f), ColorRGBA::setColor(MFColorRGBA))
  auto shape = std::make_shared<Shape>();
  shape->setGeometry(its);
  return shape;
}
```
(Confirm setter names by grepping `generated_cpp_bindings/x3d/nodes/{Normal,TextureCoordinate,ColorRGBA,IndexedTriangleSet}.hpp`.)

- [ ] **Step 4: Wire the emit test target + build + pass**

Add an `x3d_assetimport_emit` doctest exe (sources: `doctest_main.cpp`, `emit_test.cpp`, `emit.cpp`, `fixture_source.cpp`; link `x3d_cpp::authoring`; include the example dir + `test_support`). Register `add_test(NAME x3d_assetimport_emit ...)`.
Run: `cmake --build build-asset-import && ctest --test-dir build-asset-import -R x3d_assetimport_emit --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add examples/asset_import/emit.hpp examples/asset_import/emit.cpp examples/asset_import/tests/emit_test.cpp examples/asset_import/CMakeLists.txt
git commit -m "feat(asset-import): emit geometry core (IR to IndexedTriangleSet)"
```

---

## Task 6: `emit` — materials (Phong + PBR)

**Files:**
- Modify: `examples/asset_import/emit.cpp`
- Modify: `examples/asset_import/tests/emit_test.cpp`

**Interfaces:**
- Produces: internal `SFNode buildAppearance(const ImportMaterial&)` attached to each Shape.

- [ ] **Step 1: Write the failing tests**

Add to `emit_test.cpp`:
```cpp
TEST_CASE("emit_phong_material_when_no_pbr") {
  ImportScene s = FixtureSource{}.load("cube");     // cube material has no pbr
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, {}));
  CHECK(xml.find("<Material") != std::string::npos);
  CHECK(xml.find("<PhysicalMaterial") == std::string::npos);
}
TEST_CASE("emit_physical_material_when_pbr_present") {
  ImportScene s = FixtureSource{}.load("cube");
  s.materials[0].pbr = PbrParams{{0.2f,0.4f,0.8f,1.0f}, 0.0f, 0.5f};
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, {}));
  CHECK(xml.find("<PhysicalMaterial") != std::string::npos);
}
```

- [ ] **Step 2: Run to verify failure**

Run: `ctest --test-dir build-asset-import -R x3d_assetimport_emit --output-on-failure`
Expected: FAIL (no Appearance/Material emitted yet).

- [ ] **Step 3: Implement material mapping**

In `emit.cpp`, add `buildAppearance`: create `Appearance`; if `mat.pbr` present, `PhysicalMaterial` (`setBaseColor(SFColor{r,g,b})`, `setMetallic(SFFloat)`, `setRoughness(SFFloat)`, `setEmissiveColor`, `setTransparency(1 - baseColor.a)`); else `Material` (`setDiffuseColor`, `setSpecularColor`, `setEmissiveColor`, `setShininess(clamp(shininess/128, 0, 1))`, `setTransparency(1 - opacity)`). `appearance->setMaterial(mat)`. `shape->setAppearance(appearance)`. Use checked setters (values are in-range); clamp shininess/transparency to `[0,1]` before setting to avoid throw.

- [ ] **Step 4: Build + pass**

Run: `cmake --build build-asset-import && ctest --test-dir build-asset-import -R x3d_assetimport_emit --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add examples/asset_import/emit.cpp examples/asset_import/tests/emit_test.cpp
git commit -m "feat(asset-import): emit Phong + PBR materials"
```

---

## Task 7: `emit` — hierarchy, TRS decomposition, DEF/USE instancing

**Files:**
- Modify: `examples/asset_import/emit.cpp` (+ a `decomposeTRS` helper)
- Modify: `examples/asset_import/tests/emit_test.cpp`

**Interfaces:**
- Produces: internal `struct Trs { x3d::core::SFVec3f translation, scale; x3d::core::SFRotation rotation; }; Trs decomposeTRS(const Mat4&);` and recursive node emission with shared-mesh `DEF`/`USE`.

- [ ] **Step 1: Write the failing tests**

Add:
```cpp
TEST_CASE("emit_nested_hierarchy_nests_transforms") {
  ImportScene s = FixtureSource{}.load("hierarchy"); // parent node with a child node
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, {}));
  // A Transform containing another Transform.
  auto first = xml.find("<Transform");
  auto second = xml.find("<Transform", first + 1);
  CHECK(second != std::string::npos);
}
TEST_CASE("emit_shared_mesh_uses_def_use") {
  ImportScene s = FixtureSource{}.load("cube");
  s.nodes.push_back(s.nodes[s.rootNode]);   // second node reuses mesh 0
  s.nodes[s.rootNode].childIndices.push_back((int)s.nodes.size() - 1);
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, {}));
  CHECK(xml.find("USE=") != std::string::npos);
}
```

- [ ] **Step 2: Run to verify failure**

Run: `ctest --test-dir build-asset-import -R x3d_assetimport_emit --output-on-failure`
Expected: FAIL (flat emission, no nesting/USE yet).

- [ ] **Step 3: Implement recursion + TRS + instancing**

Add `decomposeTRS` (translation = column 3; scale = length of columns 0/1/2; rotation = normalize columns → 3x3 → quaternion → `SFRotation{axis, angle}`). Emit nodes recursively via `childIndices`, each as a `Transform` with `setTranslation/setRotation/setScale` from `decomposeTRS(node.localTransform)`. **Cache built mesh Shapes by mesh index** in a `std::unordered_map<int, SFNode>`: the first node using mesh `i` gets a `Shape` with `setDEF("Mesh" + i)`; subsequent nodes reuse the *same* `shared_ptr` (writer emits `USE`). Standard quaternion→axis-angle:
```cpp
// q = (w,x,y,z) normalized; angle = 2*acos(w); s = sqrt(1-w*w)
float angle = 2.0f * std::acos(std::clamp(w, -1.0f, 1.0f));
SFRotation r = (s < 1e-6f) ? SFRotation{0,0,1,0}
                           : SFRotation{x/s, y/s, z/s, angle};
```

- [ ] **Step 4: Build + pass**

Run: `cmake --build build-asset-import && ctest --test-dir build-asset-import -R x3d_assetimport_emit --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add examples/asset_import/emit.cpp examples/asset_import/tests/emit_test.cpp
git commit -m "feat(asset-import): emit hierarchy, TRS decomposition, DEF/USE instancing"
```

---

## Task 8: `emit` — cameras + lights

**Files:**
- Modify: `examples/asset_import/emit.cpp`
- Modify: `examples/asset_import/tests/emit_test.cpp`

- [ ] **Step 1: Write the failing tests**

```cpp
TEST_CASE("emit_camera_and_lights") {
  ImportScene s = FixtureSource{}.load("lit");
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, {}));
  CHECK(xml.find("<Viewpoint") != std::string::npos);
  CHECK(xml.find("Light") != std::string::npos);   // DirectionalLight/PointLight/SpotLight
}
```

- [ ] **Step 2: Run to verify failure**

Run: `ctest --test-dir build-asset-import -R x3d_assetimport_emit --output-on-failure`
Expected: FAIL.

- [ ] **Step 3: Implement**

Prepend to `doc.scene.rootNodes` (before the root Transform): one `Viewpoint` per `ImportCamera` (`setPosition`, `setOrientation` from `world` matrix rotation via `decomposeTRS`, `setFieldOfView(yfov)`); first camera bound. One light per `ImportLight`: `DirectionalLight`(`setDirection`,`setColor`,`setIntensity`,`setGlobal(true)`) / `PointLight`(`setLocation`,`setColor`,`setIntensity`,`setAttenuation`,`setRadius`,`setGlobal(true)`) / `SpotLight`(+`setDirection`,`setCutOffAngle`,`setBeamWidth`). Directions/locations are already world-space in the IR.

- [ ] **Step 4: Build + pass**

Run: `cmake --build build-asset-import && ctest --test-dir build-asset-import -R x3d_assetimport_emit --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add examples/asset_import/emit.cpp examples/asset_import/tests/emit_test.cpp
git commit -m "feat(asset-import): emit Viewpoints + lights"
```

---

## Task 9: Texture pipeline + wire `ImageTexture` slots

**Files:**
- Create: `examples/asset_import/texture_pipeline.hpp`, `examples/asset_import/texture_pipeline.cpp`
- Create: `examples/asset_import/third_party/stb_image_write.h` (copy from `examples/cpu_raster/third_party/stb_image_write.h`)
- Create: `examples/asset_import/tests/texture_pipeline_test.cpp`
- Modify: `examples/asset_import/emit.cpp` (wire texture slots when `opts.includeTextures`)
- Modify: `examples/asset_import/CMakeLists.txt`

**Interfaces:**
- Produces: `struct TexturePlan { std::unordered_map<std::string, std::string> urlByKey; };` and `TexturePlan planTextures(const ImportScene&, const std::string& outDir, const std::string& modelDir, bool recompress);` — decodes (via `x3d_stb` seam) + re-encodes (stb_image_write) + dedups by content hash + writes `<outDir>/assets/*` + returns per-texture relative URL. Emit reads `TexturePlan` (passed via `EmitOptions`) to set `ImageTexture.url`.

- [ ] **Step 1: Write the failing test**

`texture_pipeline_test.cpp`:
```cpp
#include "texture_pipeline.hpp"
#include "doctest/doctest.h"
#include <filesystem>
using namespace x3d::asset_import;

TEST_CASE("texture_pipeline_dedups_identical_embedded_textures") {
  ImportScene s;
  EmbeddedTexture t; t.key = "a"; t.hintExt = "png";
  t.bytes = /* a tiny valid 1x1 PNG byte vector */ makeTinyPng();
  s.embedded = {t, t};             // two identical
  auto out = std::filesystem::temp_directory_path() / "x3d_tex_test";
  TexturePlan plan = planTextures(s, out.string(), "", /*recompress=*/false);
  // Both embedded refs collapse to one file on disk.
  int files = 0; for (auto& e : std::filesystem::directory_iterator(out / "assets")) { (void)e; ++files; }
  CHECK(files == 1);
}
```
(Provide `makeTinyPng()` as a small static helper in the test returning a known 1×1 PNG.)

- [ ] **Step 2: Run to verify failure**

Run: `ctest --test-dir build-asset-import -R x3d_assetimport_texture --output-on-failure`
Expected: FAIL (missing header).

- [ ] **Step 3: Implement the pipeline**

Copy `stb_image_write.h` into `examples/asset_import/third_party/`. `texture_pipeline.cpp`: for each referenced texture, resolve bytes (external file read or `embedded[index]`); content-hash (e.g. FNV-1a over bytes); if web-safe (`png`/`jpeg`) and `!recompress`, write bytes through; else decode with the `x3d_stb` seam factory (`x3d::runtime::io::stb::makeStbTextureResolver()` → decode to RGBA) and re-encode PNG via `stbi_write_png`; write to `<outDir>/assets/<hash>.<ext>`; map key→`"assets/<hash>.<ext>"`. Dedup by hash so identical content writes once.

- [ ] **Step 4: Wire into emit**

Extend `EmitOptions { bool includeTextures = true; const TexturePlan* textures = nullptr; }`. In `buildAppearance`, when `opts.textures` and a slot's `TextureRef` resolves to a URL, build an `ImageTexture` (`setUrl(MFString{url})`, `setRepeatS(true)`, `setRepeatT(true)`) and assign to the material's slot setter (`Material::setDiffuseTexture` / `PhysicalMaterial::setBaseTexture`, plus normal/emissive/occlusion/metallicRoughness/specular per `TextureSlots`).

- [ ] **Step 5: Build + pass**

Add the `x3d_assetimport_texture` doctest target (link `x3d_cpp::authoring` + `x3d_stb`; `-DX3D_CPP_BUILD_STB=ON` required — guard with `if(NOT TARGET x3d_stb) message(FATAL_ERROR ...)`).
Run: `cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON && cmake --build build-asset-import && ctest --test-dir build-asset-import -R x3d_assetimport_texture --output-on-failure`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add examples/asset_import/texture_pipeline.hpp examples/asset_import/texture_pipeline.cpp examples/asset_import/third_party/stb_image_write.h examples/asset_import/tests/texture_pipeline_test.cpp examples/asset_import/emit.cpp examples/asset_import/CMakeLists.txt
git commit -m "feat(asset-import): texture pipeline (decode/encode/dedup) + ImageTexture wiring"
```

---

## Task 10: `AssimpSource` backend (gated)

**Files:**
- Create: `examples/asset_import/assimp_source.hpp`, `examples/asset_import/assimp_source.cpp`
- Create: `examples/asset_import/assets/fixtures/tri.obj` (+ `tri.mtl`) — a tiny committed triangle
- Create: `examples/asset_import/tests/assimp_source_test.cpp`
- Modify: `examples/asset_import/CMakeLists.txt` (gate under `X3D_CPP_BUILD_ASSIMP`)

**Interfaces:**
- Produces: `class AssimpSource : public ImportSource` (only compiled when `X3D_CPP_BUILD_ASSIMP=ON`).

- [ ] **Step 1: Add the assimp option + find_package**

Top `CMakeLists.txt`: `option(X3D_CPP_BUILD_ASSIMP "Build the assimp ImportSource backend for asset_import" OFF)`. In `examples/asset_import/CMakeLists.txt`, when `ON`: `find_package(assimp CONFIG REQUIRED)` (FATAL_ERROR message pointing at the flag if not found), add `assimp_source.cpp` to the CLI target, link `assimp::assimp`, and `target_compile_definitions(... PRIVATE X3D_ASSET_IMPORT_HAVE_ASSIMP=1)`.

- [ ] **Step 2: Write the gated failing test**

`assimp_source_test.cpp` (compiled only when the def is set):
```cpp
#include "assimp_source.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;
TEST_CASE("assimp_loads_triangle_obj") {
  AssimpSource src;
  ImportScene s = src.load(FIXTURE_OBJ);   // -D FIXTURE_OBJ=".../tri.obj"
  CHECK(s.meshes.size() == 1);
  CHECK(s.meshes[0].indices.size() == 3);
}
```

- [ ] **Step 3: Run to verify failure**

Run (with assimp available): `cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_ASSIMP=ON && ctest --test-dir build-asset-import -R x3d_assetimport_assimp --output-on-failure`
Expected: FAIL (missing `assimp_source.*`). If assimp is not installed, this task's test is skipped in CI's no-assimp job (Task 13); note that explicitly.

- [ ] **Step 4: Implement**

`assimp_source.cpp`: `Assimp::Importer`; `ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices?`—no, keep hierarchy); walk `aiScene->mMeshes` → `ImportMesh` (positions/normals/uv[0]/colors[0]/indices, `materialIndex`), `aiScene->mMaterials` → `ImportMaterial` (DIFFUSE/SPECULAR/EMISSIVE/SHININESS/OPACITY; `aiTextureType_BASE_COLOR`/`DIFFUSE`/`NORMALS`/`EMISSIVE`/`METALNESS`/`DIFFUSE_ROUGHNESS`→ `TextureSlots`; PBR via `AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_*` or `aiShadingMode_PBR_BRDF` → `PbrParams`), `aiScene->mRootNode` tree → `ImportNode` (convert `aiMatrix4x4` row-major → our column-major `Mat4`), `aiScene->mCameras`/`mLights` → `ImportCamera`/`ImportLight`, and `aiScene->mTextures` (embedded) → `EmbeddedTexture`. Convert texture references that are `*N` embedded indices to `TextureRef::embeddedIndex`.

- [ ] **Step 5: Build + pass**

Run: `cmake --build build-asset-import && ctest --test-dir build-asset-import -R x3d_assetimport_assimp --output-on-failure`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add examples/asset_import/assimp_source.hpp examples/asset_import/assimp_source.cpp examples/asset_import/assets/fixtures/tri.obj examples/asset_import/assets/fixtures/tri.mtl examples/asset_import/tests/assimp_source_test.cpp examples/asset_import/CMakeLists.txt
git commit -m "feat(asset-import): assimp ImportSource backend (gated)"
```

---

## Task 11: `main.cpp` CLI + self-validation + `--verify` + final CMake wiring

**Files:**
- Create: `examples/asset_import/main.cpp`
- Modify: `examples/asset_import/CMakeLists.txt` (final target `x3d_asset_import`)
- Modify: top `CMakeLists.txt` (ensure `add_subdirectory` + options finalized)
- Create: `examples/asset_import/tests/roundtrip_test.cpp`

**Interfaces:**
- Consumes: `FixtureSource`, `AssimpSource` (gated), `emit`, `planTextures`, `profile_fit::{sceneComponentUsage,findMinimalProfile}` (Task 2), `collectRangeWarnings`.
- Produces: the `x3d_asset_import` executable.

- [ ] **Step 1: Write the failing round-trip test**

`roundtrip_test.cpp` (links `x3d_cpp::x3d_cpp` so it may use the *parser* to re-read output — verification only, not part of emit):
```cpp
#include "emit.hpp"
#include "fixture_source.hpp"
#include "x3d/authoring.hpp"
#include "X3DParse.hpp"        // sdk parser, verification-only
#include "doctest/doctest.h"
using namespace x3d::asset_import;
TEST_CASE("emitted_cube_reparses_clean") {
  auto doc = emit(FixtureSource{}.load("cube"), {});
  doc.profile = x3d::runtime::Profile::Interchange;
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(doc);
  auto reparsed = x3d::codec::parseDocument(xml, x3d::codec::Encoding::XML);
  CHECK(reparsed.rangeWarnings.empty());
  CHECK_FALSE(reparsed.scene.rootNodes.empty());
}
```

- [ ] **Step 2: Run to verify failure**

Run: `ctest --test-dir build-asset-import -R x3d_assetimport_roundtrip --output-on-failure`
Expected: FAIL (target not wired).

- [ ] **Step 3: Implement `main.cpp`**

CLI per spec §7: parse args (`<input>`, `-o`, `--format xml|json|vrml|canonical`, `--assets-dir`, `--no-textures`, `--recompress`, `--profile auto|interchange|immersive|full`, `--verify`, `--stats`). Backend: `fixture:<name>` → `FixtureSource`; else `AssimpSource` (guarded by `X3D_ASSET_IMPORT_HAVE_ASSIMP`, else error "rebuild with -DX3D_CPP_BUILD_ASSIMP=ON"). Flow: `load` → `planTextures` (unless `--no-textures`) → `emit(scene, {.textures=&plan})` → set `doc.profile` via `--profile` or `profile_fit::findMinimalProfile(profile_fit::sceneComponentUsage(doc.scene))` → `collectRangeWarnings` over roots for `--stats`/nonzero-exit-on-hard-invalid → select writer, write `-o` file + `assets/`. `--verify`: re-parse via `x3d::codec::parseFile` and print node/route counts.

- [ ] **Step 4: Finalize CMake target**

`examples/asset_import/CMakeLists.txt`: `add_executable(x3d_asset_import main.cpp emit.cpp fixture_source.cpp texture_pipeline.cpp $<assimp>)`, link `x3d_cpp::authoring x3d_stb` (+`assimp::assimp` gated), include example dir + `third_party` + `../../tools` (for `x3d-cli/profile_fit.hpp`) + parse headers for `--verify`. Register `x3d_assetimport_roundtrip` doctest target. Ensure top `CMakeLists.txt` has the finalized `if(X3D_CPP_BUILD_ASSET_IMPORT) add_subdirectory(examples/asset_import) endif()`.

- [ ] **Step 5: Build + run CLI + tests**

Run:
```bash
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-asset-import
./build-asset-import/examples/asset_import/x3d_asset_import fixture:cube -o /tmp/cube.x3d --stats --verify
ctest --test-dir build-asset-import -R x3d_assetimport --output-on-failure
```
Expected: writes `/tmp/cube.x3d` (+ `/tmp/assets/` if textured), prints stats + verify counts, all ctests PASS.

- [ ] **Step 6: Commit**

```bash
git add examples/asset_import/main.cpp examples/asset_import/tests/roundtrip_test.cpp examples/asset_import/CMakeLists.txt CMakeLists.txt
git commit -m "feat(asset-import): CLI, self-validation, --verify round-trip"
```

---

## Task 12: Footprint gate (symbol scan + size baseline)

**Files:**
- Create: `scripts/authoring-footprint.sh`
- Create: `examples/asset_import/footprint/baseline.tsv`
- Modify: `mise.toml` (add `authoring-footprint` task)

**Interfaces:**
- Consumes: `x3d_authoring_smoke` (Task 1) built with `--gc-sections`.
- Produces: a gate that fails if the smoke binary contains excluded-subsystem symbols or exceeds the size baseline.

- [ ] **Step 1: Add gc-sections flags to the smoke target**

In the subdir CMake, add to `x3d_authoring_smoke`: `target_compile_options(... PRIVATE -ffunction-sections -fdata-sections)` and `target_link_options(... PRIVATE -Wl,--gc-sections -Wl,-Map=$<TARGET_FILE:x3d_authoring_smoke>.map)`.

- [ ] **Step 2: Write the gate script**

`scripts/authoring-footprint.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON >/dev/null
cmake --build build-asset-import --target x3d_authoring_smoke
BIN=build-asset-import/examples/asset_import/x3d_authoring_smoke
# 1) Symbol scan: no excluded-subsystem symbols may be linked in.
FORBIDDEN='SceneExtractor|X3DExecutionContext|parseDocument|parseFile|PhysicsWorld|ScriptSystem|MeshBuilder'
if nm -C "$BIN" | grep -Eq "$FORBIDDEN"; then
  echo "FAIL: authoring smoke pulled in excluded subsystem symbols:"; nm -C "$BIN" | grep -E "$FORBIDDEN"; exit 1
fi
# 2) Size baseline (text section of the stripped binary).
SIZE=$(size -A "$BIN" | awk '/\.text/{print $2; exit}')
BASE=$(awk -F'\t' '/^text_bytes/{print $2}' examples/asset_import/footprint/baseline.tsv)
echo "authoring smoke .text = $SIZE (baseline $BASE)"
# Allow 10% growth; --write-baseline refreshes.
if [[ "${1:-}" == "--write-baseline" ]]; then printf 'text_bytes\t%s\n' "$SIZE" > examples/asset_import/footprint/baseline.tsv; echo "baseline written"; exit 0; fi
if (( SIZE > BASE * 110 / 100 )); then echo "FAIL: .text grew >10% over baseline"; exit 1; fi
echo "PASS"
```
`chmod +x scripts/authoring-footprint.sh`.

- [ ] **Step 3: Generate the baseline + verify the scan**

Run:
```bash
bash scripts/authoring-footprint.sh --write-baseline
bash scripts/authoring-footprint.sh
```
Expected: first writes `baseline.tsv`; second prints `PASS` and the symbol scan finds nothing forbidden. If the scan *does* find an excluded symbol, that's the coupling finding — trace it (a writer/scene header transitively including parse) and either narrow `authoring.hpp` or file it as a follow-up (do not weaken `FORBIDDEN`).

- [ ] **Step 4: Add the mise task**

`mise.toml`:
```toml
[tasks.authoring-footprint]
description = "Authoring slim-target footprint gate: symbol scan (no excluded subsystems) + .text size baseline."
run = "scripts/authoring-footprint.sh"
```

- [ ] **Step 5: Commit**

```bash
git add scripts/authoring-footprint.sh examples/asset_import/footprint/baseline.tsv examples/asset_import/CMakeLists.txt mise.toml
git commit -m "feat(asset-import): authoring footprint gate (symbol scan + size baseline)"
```

---

## Task 13: CI wiring

**Files:**
- Modify: `scripts/validate-examples.sh` (build + probe asset_import, with and without assimp)
- Modify: `mise.toml` (`asset-import` task; add `authoring-footprint` to `ci` deps)

- [ ] **Step 1: Add the `asset-import` mise task**

```toml
[tasks.asset-import]
description = "Configure + build + ctest the out-of-SDK asset-import consumer (no assimp; fixture backend). Add -DX3D_CPP_BUILD_ASSIMP=ON locally for real formats."
run = """
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-asset-import
ctest --test-dir build-asset-import -R x3d_assetimport --output-on-failure
"""
```

- [ ] **Step 2: Extend `validate-examples.sh`**

Append: configure `build-asset-import` with `-DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON`, build `x3d_asset_import`, run `"$AI" fixture:cube -o "$TMP/cube.x3d" --verify` (assert exit 0 + file non-empty), and run the footprint gate `bash scripts/authoring-footprint.sh`. Guard the assimp path: if `find_package(assimp)` would succeed (probe `pkg-config --exists assimp` or a cmake configure with the flag), additionally build with `-DX3D_CPP_BUILD_ASSIMP=ON` and convert `tri.obj`; otherwise `echo "assimp not available; skipping assimp job"` (never fail on its absence).

- [ ] **Step 3: Wire footprint into `ci`**

In `[tasks.ci]`, add `"authoring-footprint"` to the `depends` list (it needs no assimp and is fast). Leave `validate-examples` as the separate examples-gate (mirrors cpuraster/poc).

- [ ] **Step 4: Run the gates**

Run:
```bash
mise run asset-import
mise run authoring-footprint
bash scripts/validate-examples.sh
```
Expected: all PASS (assimp job skipped if unavailable, with a clear message).

- [ ] **Step 5: Commit**

```bash
git add scripts/validate-examples.sh mise.toml
git commit -m "ci(asset-import): mise task + validate-examples + footprint in ci"
```

---

## Task 14: Docs + NOTICE (docs-are-part-of-diff)

**Files:**
- Create: `docs/wiki/subsystems/asset-import.md`
- Create: `docs/wiki/decisions/NNNN-authoring-target.md` (next contiguous ADR number)
- Modify: `docs/wiki/coverage.md` (add subsystem + ADR rows, bump counts)
- Modify: `mkdocs.yml` (nav entry)
- Modify: `docs/sdk/v1-capabilities.md` (authoring/export capability + slim target)
- Modify: `NOTICE` (assimp; confirm `stb_image_write`)

- [ ] **Step 1: Write the subsystem page**

`docs/wiki/subsystems/asset-import.md`: overview (write-side consumer, the pipeline diagram from the spec), the `ImportSource` seam + backends, the X3D 4.0 mapping table (§4), the texture pipeline, the `x3d_cpp::authoring` slim target + footprint gate, **the discovered minimal footprint** (the actual source/symbol set the emit path pulls, from the Task 12 map file), CLI usage, and the §6.1 relationship-to-19777-4 summary (neutral tone). Link the spec + ADR.

- [ ] **Step 2: Write the ADR**

`docs/wiki/decisions/NNNN-authoring-target.md`: decision = introduce `x3d_cpp::authoring` as the slim authoring surface; the emit-only boundary + footprint gate; the deliberate divergence from ISO/IEC 19777-4 and the naming guardrails (§6.1). Neutral, factual.

- [ ] **Step 3: Update coverage + nav + capabilities + NOTICE**

Add the subsystem row + ADR row to `docs/wiki/coverage.md` (bump counts, keep ADR numbering contiguous — `coverage-gate` enforces this). Add the nav entry to `mkdocs.yml`. Add the authoring/export capability to `docs/sdk/v1-capabilities.md`. Add assimp to `NOTICE` (name, license, URL) and confirm `stb_image_write` is already listed (add if not).

- [ ] **Step 4: Run the doc gates + drift**

Run:
```bash
mise run docs-build          # strict: dead links / nav orphans
mise run coverage-gate
mise run docs-drift working
```
Expected: `docs-build` and `coverage-gate` PASS; review `docs-drift` output and address any flagged stale pages.

- [ ] **Step 5: Commit**

```bash
git add docs/wiki/subsystems/asset-import.md docs/wiki/decisions/*-authoring-target.md docs/wiki/coverage.md mkdocs.yml docs/sdk/v1-capabilities.md NOTICE
git commit -m "docs(asset-import): subsystem page, ADR, coverage, capabilities, NOTICE"
```

---

## Final verification

- [ ] Run the full local pipeline + example gates:
```bash
mise run ci                       # now includes authoring-footprint
bash scripts/validate-examples.sh # cpu_raster + poc + asset_import
mise run asset-import
```
Expected: all green. If `mise run gen` was ever run during development, remove the stray generated `test.cpp` before `build-ci` (`rm` the gitignored generated test.cpp — known `-Werror=unused-function` trap).

- [ ] Sanity-convert a real model (if assimp is available locally):
```bash
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_ASSIMP=ON
cmake --build build-asset-import
./build-asset-import/examples/asset_import/x3d_asset_import model.glb -o out.x3d --stats --verify
```
Expected: `out.x3d` + `assets/` produced, `--verify` re-parses clean.

---

## Self-Review notes (author)

- **Spec coverage:** geometry+hierarchy (T5,T7), Phong+PBR (T6), textures→assets (T9), cameras+lights (T8), XML-default+profile-fit+self-validate (T11), slim target (T1), footprint gate (T12), assimp-behind-seam + fixture second backend (T3,T4,T10), 19777-4 relationship + guardrails (T14 ADR/page), NOTICE (T14). Native glTF backend + X3DJSAIL differential gate are spec **fast-follows** — intentionally out of this plan.
- **Deferred, documented (not tasks):** skeletal/morph animation, USD backend, shear transforms beyond TRS, MultiTexture, unit inference beyond source-declared. Belongs in the subsystem page's "deferred" section (T14) / `docs/conformance/findings.yaml` if behavioral.
- **Known verification caveat:** several exact setter names (`setSolidUnchecked`, `setVector` on `Normal`, `TextureCoordinate::setPoint`, `ColorRGBA::setColor`, `PhysicalMaterial::setMetallic`) must be confirmed by grepping the node headers during T5/T6/T9 — the plan flags this at each site rather than guessing silently.

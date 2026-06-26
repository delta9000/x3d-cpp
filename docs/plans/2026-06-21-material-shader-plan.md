# Material & Shader System Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Reshape `MaterialDesc` from flat superset to discriminated union, add the `ShaderUniformVocabulary` + `ShaderBindingPlan` author-shader cut-above, land `EnvironmentLight` IBL, and extend the PoC consumer to four programs + author path.

**Architecture:** Six phases, each its own commit boundary. Phase 1 is the breaking `MaterialDesc` restructure (flat → discriminated). Phase 2 adds `texCoordMapping`. Phase 3 ships the vocabulary + binding plan. Phase 4 adds `EnvironmentLight`. Phase 5 extends the PoC consumer. Phase 6 is docs + drift + gates. TDD throughout: existing tests migrate first (compile-fail → implement → pass), new behaviors get new tests.

**Tech Stack:** C++17, CMake/Ninja, ccache, ctest. No new deps. Assert-based tests (no framework — see `runtime/extract/tests/material_system_test.cpp` for the pattern).

**Design doc:** `docs/superpowers/specs/2026-06-21-material-shader-design.md` (approved, commit `5e10bec`).

---

## Pre-flight

**Step 0a: Confirm clean working tree**

Run: `git status --short`
Expected: clean (the spec commit `5e10bec` is HEAD).

**Step 0b: Confirm baseline build + tests are green**

Run: `mise run build`
Expected: configures + builds + runs ctest; all pass (including `x3d_material_system`, `x3d_light_system`).

If anything is red before you start, stop and report — the baseline isn't sound.

---

# Phase 1: MaterialDesc discriminated union

**Spec reference:** §2 of the design doc.

**Files:**
- Modify: `runtime/extract/RenderItem.hpp` (lines 293-348 — `MaterialModel` enum + `MaterialDesc` struct)
- Modify: `runtime/extract/MaterialSystem.hpp` (lines 158-237 — `materialOf()` body)
- Modify: `runtime/extract/tests/material_system_test.cpp` (migrate assertions to new field paths)
- Modify: `examples/poc_renderer/main.cpp` (lines 1144-1200 — bind block field reads)
- Modify: `CMakeLists.txt` (add new test targets)

## Task 1.1: Add per-variant structs + discriminator

**Step 1: Add `PhongParams` and `PhysicalParams` to `RenderItem.hpp`**

Insert before `struct MaterialDesc` (line 313), after the `AlphaMode` enum:

```cpp
struct PhongParams {
  SFColor  diffuse           {0.8f, 0.8f, 0.8f};
  SFColor  specular          {0.0f, 0.0f, 0.0f};
  float    ambientIntensity  = 0.2f;
  float    shininess         = 0.2f;
  float    occlusionStrength = 1.0f;
};
struct PhysicalParams {
  SFColor  baseColor         {1.0f, 1.0f, 1.0f};
  float    metallic          = 1.0f;
  float    roughness         = 1.0f;
  float    occlusionStrength = 1.0f;
};
```

Change `enum class MaterialModel { Phong, Physical, Unlit };` to `enum class MaterialModel : uint8_t { Phong, Physical, Unlit };` (add `#include <cstdint>` if not present).

**Step 2: Verify it compiles**

Run: `mise run build`
Expected: builds (the new structs aren't used yet; `MaterialDesc` still has its flat fields).

**Step 3: Commit**

```bash
git add runtime/extract/RenderItem.hpp
git commit -m "feat(material): add PhongParams/PhysicalParams per-variant structs"
```

## Task 1.2: Add extension seam structs + X3DFieldValue forward decl

**Step 1: Add to `RenderItem.hpp`** (before `MaterialDesc`):

```cpp
struct MaterialExtensionField {
  std::string name;
  X3DFieldType type;
  X3DFieldValue value;
};
struct MaterialExtensionDesc {
  std::string typeName;
  std::vector<MaterialExtensionField> fields;
};
```

Note: `X3DFieldValue` is defined in Phase 3 (`X3DFieldValue.hpp`). For now, add a forward declaration or a stub in `X3Dtypes.hpp`:

```cpp
// In X3Dtypes.hpp — temporary stub, replaced by full definition in Phase 3:
struct X3DFieldValue;
```

If a forward decl won't compile (variant needs complete type), defer `MaterialExtensionField` to Phase 3 and just add `MaterialExtensionDesc` with a `std::any` placeholder for field value. Adjust based on compile result.

**Step 2: Verify it compiles**

Run: `mise run build`

**Step 3: Commit**

```bash
git add runtime/extract/RenderItem.hpp runtime/extract/X3Dtypes.hpp
git commit -m "feat(material): add MaterialExtensionDesc seam + X3DFieldValue stub"
```

## Task 1.3: Reshape MaterialDesc to discriminated union

**This is the breaking change.** The existing test will fail to compile — that's expected and is the TDD signal.

**Step 1: Replace `MaterialDesc` in `RenderItem.hpp`** (lines 313-348) with the discriminated union from spec §2:

```cpp
struct MaterialDesc {
  MaterialModel model = MaterialModel::Phong;

  // X3DOneSidedMaterialNode (shared):
  SFColor  emissive      {0.0f, 0.0f, 0.0f};
  float    normalScale   = 1.0f;
  float    transparency  = 0.0f;

  // X3DMaterialNode + Appearance (shared):
  AlphaMode alphaMode   = AlphaMode::Opaque;
  float     alphaCutoff = 0.5f;
  bool      doubleSided = false;

  // per-variant (only the block for `model` is valid):
  PhongParams     phong;
  PhysicalParams  physical;

  std::vector<TextureRef> textures;
  std::optional<MaterialDesc> backMaterial;
  std::vector<MaterialExtensionDesc> extensions;

  // Composes the per-model RGB surface with alpha = 1 - transparency.
  SFColorRGBA toRGBA() const;
};
```

**Step 2: Implement `toRGBA()`** (replaces the inline version at line 344):

```cpp
inline SFColorRGBA MaterialDesc::toRGBA() const {
  float a = 1.0f - transparency;
  switch (model) {
    case MaterialModel::Phong:
      return SFColorRGBA{phong.diffuse.r, phong.diffuse.g, phong.diffuse.b, a};
    case MaterialModel::Physical:
      return SFColorRGBA{physical.baseColor.r, physical.baseColor.g, physical.baseColor.b, a};
    case MaterialModel::Unlit:
      return SFColorRGBA{emissive.r, emissive.g, emissive.b, a};
  }
  return SFColorRGBA{0.8f, 0.8f, 0.8f, a};
}
```

**Step 3: Run build — expect compile failures in `material_system_test.cpp` and `main.cpp`**

Run: `mise run build`
Expected: FAIL — `m.baseColor`, `m.metallic`, `m.roughness`, `m.specular`, `m.shininess`, `m.ambientIntensity`, `m.unlit` no longer exist.

**Step 4: Commit the header change (tests still broken — next tasks fix them)**

```bash
git add runtime/extract/RenderItem.hpp
git commit -m "refactor(material)!: reshape MaterialDesc to discriminated union

BREAKING CHANGE: MaterialDesc fields move to per-variant structs.
- metallic/roughness/baseColor → PhysicalParams
- specular/shininess/ambientIntensity → PhongParams
- unlit bool dropped (use model == Unlit)
- backMaterial/doubleSided/extensions added
- toRGBA() reads per-model fields

Existing consumers must migrate field reads. See spec §2."
```

## Task 1.4: Update MaterialSystem::materialOf()

**Step 1: Update `materialOf()` in `MaterialSystem.hpp`** (lines 158-237).

For the `PhysicalMaterial` branch, replace:
```cpp
m.metallic = geombounds::getField<float>(*material, "metallic", 1.0f);
m.roughness = geombounds::getField<float>(*material, "roughness", 1.0f);
m.occlusionStrength = geombounds::getField<float>(*material, "occlusionStrength", 1.0f);
m.baseColor = SFColorRGBA{base.r, base.g, base.b, 1.0f - m.transparency};
```
with:
```cpp
m.physical.baseColor = base;
m.physical.metallic = geombounds::getField<float>(*material, "metallic", 1.0f);
m.physical.roughness = geombounds::getField<float>(*material, "roughness", 1.0f);
m.physical.occlusionStrength = geombounds::getField<float>(*material, "occlusionStrength", 1.0f);
```

For the `UnlitMaterial` branch, replace `m.baseColor = SFColorRGBA{...}` with just setting `m.emissive = emis;` (toRGBA reads emissive for Unlit).

For the `Material` (Phong) branch, replace:
```cpp
m.baseColor = SFColorRGBA{diffuse.r, diffuse.g, diffuse.b, 1.0f - m.transparency};
m.specular = ...;
m.shininess = ...;
m.ambientIntensity = ...;
m.occlusionStrength = ...;
```
with:
```cpp
m.phong.diffuse = diffuse;
m.phong.specular = geombounds::getField<SFColor>(*material, "specularColor", SFColor{0,0,0});
m.phong.shininess = geombounds::getField<float>(*material, "shininess", 0.2f);
m.phong.ambientIntensity = geombounds::getField<float>(*material, "ambientIntensity", 0.2f);
m.phong.occlusionStrength = geombounds::getField<float>(*material, "occlusionStrength", 1.0f);
```

For null Appearance / null material, remove `m.unlit = true` and `m.baseColor = ...` (toRGBA handles it via emissive).

**Step 2: Run build — expect `material_system_test.cpp` and `main.cpp` still broken (field reads)**

Run: `mise run build`
Expected: FAIL — test still references old field paths.

**Step 3: Commit**

```bash
git add runtime/extract/MaterialSystem.hpp
git commit -m "refactor(material): materialOf fills per-variant structs"
```

## Task 1.5: Migrate material_system_test.cpp

**Step 1: Update test assertions** to use new field paths:

- `m.baseColor.r` → `m.toRGBA().r` (or `m.phong.diffuse.r` / `m.physical.baseColor.r`)
- `m.metallic` → `m.physical.metallic`
- `m.roughness` → `m.physical.roughness`
- `m.specular` → `m.phong.specular`
- `m.shininess` → `m.phong.shininess`
- `m.ambientIntensity` → `m.phong.ambientIntensity`
- `m.unlit` → `(m.model == MaterialModel::Unlit)`

**Step 2: Add a new assertion** pinning the discriminated-union invariant (spec §2 golden/test impact):

```cpp
// Phong material has physical.metallic == 1.0f (default, in Physical block,
// IGNORED on Phong — discriminator selects model, not field presence).
{
  auto mat = createX3DNode("Material");
  auto app = createX3DNode("Appearance");
  setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));
  MaterialDesc m = materialOf(app.get());
  assert(m.model == MaterialModel::Phong);
  assert(feq(m.physical.metallic, 1.0f));  // default, but meaningless for Phong
  assert(feq(m.phong.shininess, 0.2f));    // the field that actually matters
}
```

**Step 3: Run build — expect PASS**

Run: `mise run build`
Expected: `x3d_material_system` passes. `main.cpp` (PoC) still broken — next task.

**Step 4: Commit**

```bash
git add runtime/extract/tests/material_system_test.cpp
git commit -m "test(material): migrate to discriminated union field paths"
```

## Task 1.6: Migrate PoC consumer (main.cpp)

**Step 1: Update field reads** in `drawItem` lambda (lines 1144-1200):

- `c = mat.toRGBA()` — no change (toRGBA still works)
- `mat.specular.r` → `mat.phong.specular.r`
- `mat.shininess` → `mat.phong.shininess`
- `mat.ambientIntensity` → `mat.phong.ambientIntensity`
- `mat.metallic`/`mat.roughness` (if present) → `mat.physical.metallic`/`mat.physical.roughness`

**Step 2: Build the PoC**

Run: `mise run poc`
Expected: builds successfully.

**Step 3: Commit**

```bash
git add examples/poc_renderer/main.cpp
git commit -m "refactor(poc): migrate material field reads to discriminated union"
```

## Task 1.7: Regen golden

**Step 1: Run golden gate**

Run: `mise run golden`
Expected: reports drift on `RenderItem.hpp`-dependent generated headers.

**Step 2: If drift is expected (the header changed), regen**

Run: `mise run gen`

**Step 3: Verify golden passes now**

Run: `mise run golden`
Expected: PASS (no drift).

**Step 4: Commit the regen**

```bash
git add generated_cpp_bindings/
git commit -m "chore(golden): regen bindings after MaterialDesc restructure"
```

## Task 1.8: backMaterial + doubleSided

**Step 1: Write the failing test** — append to `material_system_test.cpp`:

```cpp
// --- backMaterial (closes MAT-006) --------------------------------------
{
  auto frontMat = createX3DNode("Material");
  setF(frontMat, "diffuseColor", std::any(SFColor{0.2f, 0.4f, 0.6f}));
  auto backMat = createX3DNode("Material");
  setF(backMat, "diffuseColor", std::any(SFColor{0.6f, 0.4f, 0.2f}));
  auto app = createX3DNode("Appearance");
  setF(app, "material", std::any(std::shared_ptr<X3DNode>(frontMat)));
  setF(app, "backMaterial", std::any(std::shared_ptr<X3DNode>(backMat)));

  MaterialDesc m = materialOf(app.get());
  assert(m.backMaterial.has_value());
  assert(m.doubleSided == true);
  assert(m.backMaterial->model == MaterialModel::Phong);
  assert(feq(m.backMaterial->phong.diffuse.r, 0.6f));
}
```

**Step 2: Run test — expect FAIL**

Run: `mise run build`
Expected: `x3d_material_system` FAIL — `backMaterial` not populated.

**Step 3: Implement backMaterial read** in `materialOf()`:

```cpp
// After reading the front material + textures:
if (auto backNode = geombounds::getNode(*appearance, "backMaterial")) {
  // Recurse: treat backNode as a material under a synthetic null Appearance.
  // The constraint (same model + same textures) is flagged, not enforced.
  auto backApp = createX3DNode("Appearance");
  setF(backApp, "material", std::any(std::move(backNode)));
  m.backMaterial = materialOf(backApp.get());
}
```

**Step 4: Compute `doubleSided`** — this needs `solid` from geometry, which `materialOf` doesn't see. Document that `SceneExtractor` sets it when assembling the `RenderItem`. For the test, set `doubleSided` based on `backMaterial.has_value()` as a partial derivation.

**Step 5: Run test — expect PASS**

Run: `mise run build`
Expected: PASS.

**Step 6: Commit**

```bash
git add runtime/extract/MaterialSystem.hpp runtime/extract/tests/material_system_test.cpp
git commit -m "feat(material): backMaterial read + doubleSided derivation (closes MAT-006)"
```

---

# Phase 2: TextureRef multi-UV (texCoordMapping)

**Spec reference:** §3 of the design doc.

## Task 2.1: Add texCoordMapping field

**Step 1: Add to `TextureRef` in `RenderItem.hpp`** (after `channel`, line 245):

```cpp
SFString texCoordMapping;  // X3D v4 xxxTextureMapping label; empty = UV set 0.
```

**Step 2: Build — expect PASS** (field is unused, just added)

Run: `mise run build`

**Step 3: Commit**

```bash
git add runtime/extract/RenderItem.hpp
git commit -m "feat(texture): add texCoordMapping field to TextureRef"
```

## Task 2.2: Populate texCoordMapping in texturesOf()

**Step 1: Write the failing test** — new file `runtime/extract/tests/texture_mapping_test.cpp`:

```cpp
#include "MaterialSystem.hpp"
#include "X3DNodeFactory.hpp"
#include <any>
#include <cassert>
#include <memory>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

int main() {
  // PhysicalMaterial with baseTextureMapping="uv1"
  {
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "url", std::any(MFString{"u.png"}));
    auto mat = createX3DNode("PhysicalMaterial");
    setF(mat, "baseTexture", std::any(std::shared_ptr<X3DNode>(tex)));
    setF(mat, "baseTextureMapping", std::any(SFString{"uv1"}));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));

    MaterialDesc m = materialOf(app.get());
    assert(!m.textures.empty());
    assert(m.textures[0].slot == TextureRef::Slot::BaseColor);
    assert(m.textures[0].texCoordMapping == "uv1");
  }
  // Empty texCoordMapping => UV set 0
  {
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "url", std::any(MFString{"u.png"}));
    auto mat = createX3DNode("PhysicalMaterial");
    setF(mat, "baseTexture", std::any(std::shared_ptr<X3DNode>(tex)));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));

    MaterialDesc m = materialOf(app.get());
    assert(!m.textures.empty());
    assert(m.textures[0].texCoordMapping.empty());
  }
  return 0;
}
```

**Step 2: Register in `CMakeLists.txt`** (after the `x3d_material_system` block, ~line 737):

```cmake
    # MAT-007: TextureRef::texCoordMapping (X3D v4 xxxTextureMapping).
    add_executable(x3d_texture_mapping
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/texture_mapping_test.cpp")
    target_link_libraries(x3d_texture_mapping PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_texture_mapping COMMAND x3d_texture_mapping)
```

**Step 3: Run test — expect FAIL**

Run: `mise run build`
Expected: `x3d_texture_mapping` FAIL — `texCoordMapping` empty even when `baseTextureMapping="uv1"`.

**Step 4: Implement** — update `matsys::appendSlot()` in `MaterialSystem.hpp` to read the mapping field. Add a helper:

```cpp
inline SFString mappingOf(const X3DNode &materialNode, const char *textureFieldName) {
  // The mapping field is named <textureFieldName> + "Mapping" by X3D v4 convention:
  // baseTexture → baseTextureMapping, diffuseTexture → diffuseTextureMapping, etc.
  std::string mappingName = std::string(textureFieldName) + "Mapping";
  return geombounds::getField<SFString>(materialNode, mappingName.c_str(), SFString{});
}
```

In `appendSlot()`, after constructing `ref`, read the mapping from the material node (requires threading the material node through — adjust signature).

**Step 5: Run test — expect PASS**

Run: `mise run build`

**Step 6: Commit**

```bash
git add runtime/extract/MaterialSystem.hpp runtime/extract/tests/texture_mapping_test.cpp CMakeLists.txt
git commit -m "feat(texture): populate texCoordMapping from xxxTextureMapping (MAT-007)"
```

## Task 2.3: ORM channel-packing documentation + color_space

**Step 1: Write the failing test** — new file `runtime/extract/tests/texture_orm_test.cpp`:

```cpp
#include "MaterialSystem.hpp"
#include "X3DNodeFactory.hpp"
#include <any>
#include <cassert>
#include <memory>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

int main() {
  // PhysicalMaterial with metallicRoughnessTexture → single slot, Linear.
  {
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "url", std::any(MFString{"orm.png"}));
    auto mat = createX3DNode("PhysicalMaterial");
    setF(mat, "metallicRoughnessTexture", std::any(std::shared_ptr<X3DNode>(tex)));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));

    MaterialDesc m = materialOf(app.get());
    // Exactly one MetallicRoughness slot (not separate Metallic + Roughness).
    int mrCount = 0;
    for (const auto &t : m.textures)
      if (t.slot == TextureRef::Slot::MetallicRoughness) ++mrCount;
    assert(mrCount == 1);
  }
  return 0;
}
```

**Step 2: Register in `CMakeLists.txt`** (same pattern as Task 2.2).

**Step 3: Run test — expect PASS already** (the existing `texturesOf()` already emits one `MetallicRoughness` slot per spec §3).

If PASS, the test pins the behavior. If FAIL, fix `texturesOf()`.

**Step 4: Document the color_space convention** in the `TextureRef::Slot` enum comment in `RenderItem.hpp`.

**Step 5: Commit**

```bash
git add runtime/extract/tests/texture_orm_test.cpp runtime/extract/RenderItem.hpp CMakeLists.txt
git commit -m "test(texture): pin ORM single-slot + document channel-packing (MAT-008)"
```

---

# Phase 3: Vocabulary + Binding Plan (the cut-above)

**Spec reference:** §4 of the design doc. This is the core of the design.

## Task 3.1: X3DFieldValue variant type

**Step 1: Create `runtime/extract/X3DFieldValue.hpp`** with the full variant from spec §4.5:

```cpp
#ifndef X3D_RUNTIME_EXTRACT_X3D_FIELD_VALUE_HPP
#define X3D_RUNTIME_EXTRACT_X3D_FIELD_VALUE_HPP

#include "RenderItem.hpp"  // TextureRef
#include "X3Dtypes.hpp"    // SF* value types
#include <variant>

namespace x3d::runtime::extract {

struct X3DFieldValue {
  X3DFieldType type = X3DFieldType::SFString;
  std::variant<std::monostate, float, int, bool, SFColor, SFColorRGBA,
               SFVec2f, SFVec3f, SFVec4f, SFMatrix3f, SFMatrix4f,
               TextureRef> value;
};

} // namespace x3d::runtime::extract
#endif
```

**Step 2: Remove the stub from `X3Dtypes.hpp`** (added in Task 1.2).

**Step 3: Build**

Run: `mise run build`

**Step 4: Commit**

```bash
git add runtime/extract/X3DFieldValue.hpp runtime/extract/X3Dtypes.hpp runtime/extract/RenderItem.hpp
git commit -m "feat(shader): add X3DFieldValue variant type (Annex I Table I.3 coverage)"
```

## Task 3.2: ShaderProgramDesc + ShaderStageDesc + ShaderFieldBinding

**Step 1: Add to `RenderItem.hpp`** (after `MaterialDesc`, before `LightDesc`):

```cpp
struct ShaderStageDesc {
  enum class Stage { Vertex, Fragment, Geometry, TessControl, TessEval, Compute };
  Stage stage = Stage::Fragment;
  std::string source;
  std::string entryPoint = "main";
};

struct ShaderFieldBinding {
  std::string name;
  X3DFieldType type;
  X3DFieldValue value;
  AccessType access;
};

struct ShaderProgramDesc {
  std::vector<ShaderStageDesc> stages;
  std::vector<ShaderFieldBinding> fields;
  bool isSelected = false;
  bool isValid = false;
  std::string lastError;
  std::vector<std::string> attributeBindings;
};
```

Note: `AccessType` must be visible — check if it's in `X3Dtypes.hpp` or `X3DReflection.hpp`; include as needed.

**Step 2: Add `shaderProgram` to `RenderItem`** in `SceneExtractor.hpp` (after `material`, line 99):

```cpp
std::optional<ShaderProgramDesc> shaderProgram;  // absent = fixed-function path
```

**Step 3: Build**

Run: `mise run build`

**Step 4: Commit**

```bash
git add runtime/extract/RenderItem.hpp runtime/extract/SceneExtractor.hpp
git commit -m "feat(shader): add ShaderProgramDesc + stages + field bindings"
```

## Task 3.3: ShaderUniformVocabulary.hpp

**Step 1: Create `runtime/extract/ShaderUniformVocabulary.hpp`** with the full table from spec §4.1. Start with the critical entries (matrices, material, lights, environment) and expand:

```cpp
#ifndef X3D_RUNTIME_EXTRACT_SHADER_UNIFORM_VOCABULARY_HPP
#define X3D_RUNTIME_EXTRACT_SHADER_UNIFORM_VOCABULARY_HPP

#include "X3Dtypes.hpp"  // X3DFieldType
#include <string_view>

namespace x3d::runtime::extract::vocab {

enum class UniformSource : uint8_t {
  ModelViewMatrix, ProjectionMatrix, NormalMatrix, ModelViewProjectionMatrix,
  ModelMatrix, ViewMatrix, TextureMatrix,
  NumLights, LightType, LightColor, LightDirection, LightLocation,
  LightAttenuation, LightRadius, LightAmbientIntensity,
  LightBeamWidth, LightCutOffAngle, LightGlobal, LightScopeRoot,
  EnvDiffuse, EnvSpecular, EnvSH, BrdfLUT, EnvIntensity, EnvRotation,
  BaseColor, DiffuseColor, SpecularColor, EmissiveColor,
  Metallic, Roughness, Shininess, AmbientIntensity,
  OcclusionStrength, NormalScale, Transparency, AlphaMode, AlphaCutoff,
  BaseColorTex, DiffuseTex, SpecularTex, EmissiveTex, NormalTex,
  OcclusionTex, MetallicRoughnessTex, ShininessTex, AmbientTex,
  HasBaseColorTex, HasDiffuseTex, HasSpecularTex, HasEmissiveTex,
  HasNormalTex, HasOcclusionTex, HasMetallicRoughnessTex,
  HasShininessTex, HasAmbientTex,
  FogColor, FogType, FogRange, FogVisibilityRange,
  Time, ViewportSize, NearFar,
  NumClipPlanes, ClipPlane,
  Unrecognized,
};

struct VocabEntry {
  std::string_view name;
  UniformSource source;
  X3DFieldType glsl_type;
  bool is_array;
  std::string_view doc;
};

inline constexpr VocabEntry kVocabulary[] = {
  {"modelViewMatrix",     UniformSource::ModelViewMatrix,     X3DFieldType::SFMatrix4f, false,
   "RenderItem.worldTransform * CameraDesc.viewMatrix"},
  {"projectionMatrix",    UniformSource::ProjectionMatrix,    X3DFieldType::SFMatrix4f, false,
   "CameraDesc perspective/ortho projection"},
  {"normalMatrix",        UniformSource::NormalMatrix,        X3DFieldType::SFMatrix3f, false,
   "inverse-transpose of view*model"},
  {"modelViewProjectionMatrix", UniformSource::ModelViewProjectionMatrix, X3DFieldType::SFMatrix4f, false,
   "proj * view * model"},
  {"modelMatrix",         UniformSource::ModelMatrix,         X3DFieldType::SFMatrix4f, false,
   "RenderItem.worldTransform"},
  {"viewMatrix",          UniformSource::ViewMatrix,          X3DFieldType::SFMatrix4f, false,
   "CameraDesc.viewMatrix"},
  {"numLights",           UniformSource::NumLights,           X3DFieldType::SFInt32,   false,
   "active LightDesc count"},
  {"lightType",           UniformSource::LightType,           X3DFieldType::SFInt32,   true,
   "LightDesc::Type enum"},
  {"lightColor",          UniformSource::LightColor,          X3DFieldType::SFColor,   true,
   "LightDesc.color * intensity"},
  {"lightDirection",      UniformSource::LightDirection,      X3DFieldType::SFVec3f,   true,
   "LightDesc.worldDirection (eye-space)"},
  {"lightLocation",       UniformSource::LightLocation,       X3DFieldType::SFVec3f,   true,
   "LightDesc.worldLocation (eye-space)"},
  {"lightAttenuation",    UniformSource::LightAttenuation,    X3DFieldType::SFVec3f,   true,
   "LightDesc.attenuation"},
  {"lightRadius",         UniformSource::LightRadius,         X3DFieldType::SFFloat,   true,
   "LightDesc.radius"},
  {"lightAmbientIntensity", UniformSource::LightAmbientIntensity, X3DFieldType::SFFloat, true,
   "LightDesc.ambientIntensity"},
  {"lightBeamWidth",      UniformSource::LightBeamWidth,      X3DFieldType::SFFloat,   true,
   "LightDesc.beamWidth"},
  {"lightCutOffAngle",    UniformSource::LightCutOffAngle,    X3DFieldType::SFFloat,   true,
   "LightDesc.cutOffAngle"},
  {"diffuseColor",        UniformSource::DiffuseColor,        X3DFieldType::SFColor,   false,
   "MaterialDesc.phong.diffuse"},
  {"specularColor",       UniformSource::SpecularColor,       X3DFieldType::SFColor,   false,
   "MaterialDesc.phong.specular"},
  {"emissiveColor",       UniformSource::EmissiveColor,       X3DFieldType::SFColor,   false,
   "MaterialDesc.emissive"},
  {"baseColor",           UniformSource::BaseColor,           X3DFieldType::SFColor,   false,
   "MaterialDesc.physical.baseColor"},
  {"metallic",            UniformSource::Metallic,            X3DFieldType::SFFloat,   false,
   "MaterialDesc.physical.metallic"},
  {"roughness",           UniformSource::Roughness,           X3DFieldType::SFFloat,   false,
   "MaterialDesc.physical.roughness"},
  {"shininess",           UniformSource::Shininess,           X3DFieldType::SFFloat,   false,
   "MaterialDesc.phong.shininess"},
  {"ambientIntensity",    UniformSource::AmbientIntensity,    X3DFieldType::SFFloat,   false,
   "MaterialDesc.phong.ambientIntensity"},
  {"occlusionStrength",   UniformSource::OcclusionStrength,   X3DFieldType::SFFloat,   false,
   "MaterialDesc.phong/physical.occlusionStrength"},
  {"normalScale",         UniformSource::NormalScale,         X3DFieldType::SFFloat,   false,
   "MaterialDesc.normalScale"},
  {"transparency",        UniformSource::Transparency,        X3DFieldType::SFFloat,   false,
   "MaterialDesc.transparency"},
  {"alphaMode",           UniformSource::AlphaMode,           X3DFieldType::SFInt32,   false,
   "MaterialDesc.alphaMode enum"},
  {"alphaCutoff",         UniformSource::AlphaCutoff,         X3DFieldType::SFFloat,   false,
   "MaterialDesc.alphaCutoff"},
  {"baseColorTex",        UniformSource::BaseColorTex,        X3DFieldType::SFNode,    false,
   "TextureRef slot BaseColor sampler"},
  {"normalTex",           UniformSource::NormalTex,           X3DFieldType::SFNode,    false,
   "TextureRef slot Normal sampler"},
  {"emissiveTex",         UniformSource::EmissiveTex,         X3DFieldType::SFNode,    false,
   "TextureRef slot Emissive sampler"},
  {"metallicRoughnessTex",UniformSource::MetallicRoughnessTex,X3DFieldType::SFNode,    false,
   "TextureRef slot MetallicRoughness sampler"},
  {"occlusionTex",        UniformSource::OcclusionTex,        X3DFieldType::SFNode,    false,
   "TextureRef slot Occlusion sampler"},
  {"hasBaseColorTex",     UniformSource::HasBaseColorTex,     X3DFieldType::SFInt32,   false,
   "1 if BaseColor texture bound, else 0"},
  {"hasNormalTex",        UniformSource::HasNormalTex,        X3DFieldType::SFInt32,   false,
   "1 if Normal texture bound, else 0"},
  {"hasEmissiveTex",      UniformSource::HasEmissiveTex,      X3DFieldType::SFInt32,   false,
   "1 if Emissive texture bound, else 0"},
  {"hasMetallicRoughnessTex", UniformSource::HasMetallicRoughnessTex, X3DFieldType::SFInt32, false,
   "1 if MetallicRoughness texture bound, else 0"},
  {"hasOcclusionTex",     UniformSource::HasOcclusionTex,     X3DFieldType::SFInt32,   false,
   "1 if Occlusion texture bound, else 0"},
  {"envDiffuse",          UniformSource::EnvDiffuse,          X3DFieldType::SFNode,    false,
   "LightDesc.environment.diffuseTexture cubemap"},
  {"envSpecular",         UniformSource::EnvSpecular,         X3DFieldType::SFNode,    false,
   "LightDesc.environment.specularTexture cubemap"},
  {"envSH",               UniformSource::EnvSH,               X3DFieldType::MFColor,   false,
   "LightDesc.environment.diffuseCoefficients[9]"},
  {"brdfLUT",             UniformSource::BrdfLUT,             X3DFieldType::SFNode,    false,
   "consumer-managed BRDF integration LUT"},
  {"envIntensity",        UniformSource::EnvIntensity,        X3DFieldType::SFFloat,   false,
   "LightDesc.environment.intensity"},
  {"time",                UniformSource::Time,                X3DFieldType::SFFloat,   false,
   "wall-clock seconds since viewer start"},
  {"viewportSize",        UniformSource::ViewportSize,        X3DFieldType::SFVec2f,   false,
   "viewport width/height in pixels"},
  // ... expand with fog/clip/light remaining entries ...
};

} // namespace vocab
} // namespace x3d::runtime::extract
#endif
```

**Step 2: Build**

Run: `mise run build`

**Step 3: Commit**

```bash
git add runtime/extract/ShaderUniformVocabulary.hpp
git commit -m "feat(shader): add ShaderUniformVocabulary — typed portability surface"
```

## Task 3.4: ShaderBindingPlan.hpp + buildBindingPlan()

**Step 1: Write the failing test** — `runtime/extract/tests/shader_binding_plan_test.cpp`:

```cpp
#include "ShaderBindingPlan.hpp"
#include <cassert>
#include <string>
#include <vector>

using namespace x3d::runtime::extract;
using namespace x3d::runtime::extract::vocab;

int main() {
  // Mock declared uniforms: modelViewMatrix (vocab), uTypo (unrecognized),
  // hue (author field).
  ShaderProgramDesc prog;
  ShaderFieldBinding hueField;
  hueField.name = "hue";
  hueField.type = X3DFieldType::SFFloat;
  prog.fields.push_back(hueField);

  std::vector<std::pair<std::string, int>> declared = {
    {"modelViewMatrix", 0},
    {"uModelViewMat",   1},
    {"hue",             2},
  };

  ShaderBindingPlan plan = buildBindingPlan(declared, prog);

  assert(plan.entries.size() == 3);
  // modelViewMatrix → vocab
  assert(!plan.entries[0].unrecognized);
  assert(plan.entries[0].source == UniformSource::ModelViewMatrix);
  assert(!plan.entries[0].isAuthorField);
  // uModelViewMat → unrecognized, with suggestion
  assert(plan.entries[1].unrecognized);
  assert(!plan.entries[1].nearestVocabMatch.empty());
  // hue → author field
  assert(!plan.entries[2].unrecognized);
  assert(plan.entries[2].isAuthorField);

  // Diagnostics for unrecognized
  bool hasDiagnostic = false;
  for (const auto &d : plan.diagnostics)
    if (d.find("uModelViewMat") != std::string::npos &&
        d.find("modelViewMatrix") != std::string::npos)
      hasDiagnostic = true;
  assert(hasDiagnostic);
  return 0;
}
```

**Step 2: Register in `CMakeLists.txt`**:

```cmake
    # Author-shader binding plan (vocabulary + introspection).
    add_executable(x3d_shader_binding_plan
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/shader_binding_plan_test.cpp")
    target_link_libraries(x3d_shader_binding_plan PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_shader_binding_plan COMMAND x3d_shader_binding_plan)
```

**Step 3: Run test — expect FAIL** (`ShaderBindingPlan.hpp` doesn't exist yet)

Run: `mise run build`
Expected: FAIL — missing header.

**Step 4: Create `runtime/extract/ShaderBindingPlan.hpp`**:

```cpp
#ifndef X3D_RUNTIME_EXTRACT_SHADER_BINDING_PLAN_HPP
#define X3D_RUNTIME_EXTRACT_SHADER_BINDING_PLAN_HPP

#include "RenderItem.hpp"                  // ShaderProgramDesc
#include "ShaderUniformVocabulary.hpp"     // vocab::kVocabulary, UniformSource
#include <algorithm>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

struct BindingEntry {
  std::string declaredName;
  vocab::UniformSource source = vocab::UniformSource::Unrecognized;
  int location = -1;
  bool isAuthorField = false;
  bool unrecognized = false;
  std::string nearestVocabMatch;
};

struct ShaderBindingPlan {
  std::vector<BindingEntry> entries;
  std::vector<std::string> diagnostics;
};

// Edit-distance for nearest-match suggestion. Simple Levenshtein.
inline int editDistance(const std::string &a, const std::string &b) {
  std::vector<std::vector<int>> d(a.size() + 1, std::vector<int>(b.size() + 1));
  for (size_t i = 0; i <= a.size(); ++i) d[i][0] = static_cast<int>(i);
  for (size_t j = 0; j <= b.size(); ++j) d[0][j] = static_cast<int>(j);
  for (size_t i = 1; i <= a.size(); ++i)
    for (size_t j = 1; j <= b.size(); ++j)
      d[i][j] = std::min({
        d[i-1][j] + 1,
        d[i][j-1] + 1,
        d[i-1][j-1] + (a[i-1] == b[j-1] ? 0 : 1)
      });
  return d[a.size()][b.size()];
}

inline std::string nearestVocabMatch(const std::string &name) {
  const auto *best = &vocab::kVocabulary[0];
  int bestDist = editDistance(name, std::string(best->name));
  for (const auto &e : vocab::kVocabulary) {
    int d = editDistance(name, std::string(e.name));
    if (d < bestDist) { bestDist = d; best = &e; }
  }
  // Only suggest if reasonably close (within 1/3 of the name length).
  if (bestDist <= static_cast<int>(name.size()) / 3 + 1)
    return std::string(best->name);
  return {};
}

inline ShaderBindingPlan buildBindingPlan(
    const std::vector<std::pair<std::string, int>> &declaredUniforms,
    const ShaderProgramDesc &program) {
  ShaderBindingPlan plan;
  for (const auto &[name, loc] : declaredUniforms) {
    BindingEntry e;
    e.declaredName = name;
    e.location = loc;

    // 1. Check vocabulary.
    for (const auto &v : vocab::kVocabulary) {
      if (name == v.name) {
        e.source = v.source;
        break;
      }
    }
    if (e.source != vocab::UniformSource::Unrecognized) {
      plan.entries.push_back(e);
      continue;
    }

    // 2. Check author <field> declarations.
    for (const auto &f : program.fields) {
      if (name == f.name) {
        e.isAuthorField = true;
        break;
      }
    }
    if (e.isAuthorField) {
      plan.entries.push_back(e);
      continue;
    }

    // 3. Unrecognized — diagnose.
    e.unrecognized = true;
    e.nearestVocabMatch = nearestVocabMatch(name);
    plan.entries.push_back(e);

    std::string diag = "shader declared `" + name + "` — not in vocabulary, ";
    if (!e.nearestVocabMatch.empty())
      diag += "not a declared <field>. Did you mean `" + e.nearestVocabMatch + "`?";
    else
      diag += "not a declared <field>. Will be unbound (location -1).";
    plan.diagnostics.push_back(diag);
  }
  return plan;
}

} // namespace x3d::runtime::extract
#endif
```

**Step 5: Run test — expect PASS**

Run: `mise run build`

**Step 6: Commit**

```bash
git add runtime/extract/ShaderBindingPlan.hpp runtime/extract/tests/shader_binding_plan_test.cpp CMakeLists.txt
git commit -m "feat(shader): add ShaderBindingPlan + buildBindingPlan with edit-distance diagnostics"
```

---

# Phase 4: EnvironmentLight / IBL

**Spec reference:** §5 of the design doc.

## Task 4.1: Add Type::Environment + Environment struct to LightDesc

**Step 1: Write the failing test** — append to `light_system_test.cpp`:

```cpp
  // --- EnvironmentLight → LightDesc::Type::Environment -------------------
  {
    auto env = createX3DNode("EnvironmentLight");
    setF(env, "intensity", std::any(0.8f));
    // diffuseCoefficients: 9x3 SH
    std::vector<float> sh(27, 0.0f);
    sh[0] = 0.5f;  // L0,R
    setF(env, "diffuseCoefficients", std::any(sh));

    Scene scene;
    scene.addRootNode(env);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::LightSystem ls;
    auto lights = ls.collect(scene);
    assert(lights.size() == 1);
    const extract::LightDesc &L = lights[0];
    assert(L.type == extract::LightDesc::Type::Environment);
    assert(L.global == true);  // IBL is always scene-wide
    assert(feq(L.environment.intensity, 0.8f));
    assert(L.environment.hasSH);
    assert(feq(L.environment.diffuseCoefficients[0], 0.5f));
  }
```

**Step 2: Run test — expect FAIL**

Run: `mise run build`
Expected: FAIL — `Type::Environment` doesn't exist; `EnvironmentLight` not dispatched.

**Step 3: Add `Environment` to `LightDesc`** in `RenderItem.hpp` (spec §5):

```cpp
struct LightDesc {
  enum class Type { Directional, Point, Spot, Environment };
  // ... existing fields ...
  struct Environment {
    TextureRef diffuseTexture;
    TextureRef specularTexture;
    std::array<float, 27> diffuseCoefficients{};
    bool hasSH = false;
    float intensity = 1.0f;
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    uint32_t specularImageSize = 0;
    uint32_t specularMipCount = 0;
  };
  Environment environment;
};
```

Add `#include <array>` to `RenderItem.hpp` if not present.

**Step 4: Update `LightSystem::lightType()`** to dispatch `EnvironmentLight`:

```cpp
if (t == "EnvironmentLight") return LightDesc::Type::Environment;
```

**Step 5: Update `LightSystem::makeLight()`** for Environment (spec §5):

```cpp
if (type == LightDesc::Type::Environment) {
  L.global = true;  // IBL is always scene-wide
  L.environment.intensity = geombounds::getField<float>(n, "intensity", 1.0f);
  auto sh = geombounds::getField<std::vector<float>>(n, "diffuseCoefficients", {});
  if (sh.size() >= 27) {
    L.environment.hasSH = true;
    for (int i = 0; i < 27; ++i) L.environment.diffuseCoefficients[i] = sh[i];
  }
  // diffuseTexture / specularTexture: read as SFNode, convert to TextureRef
  // via the existing matsys::refOf helper (or a simplified path).
  // rotation: read as SFRotation.
  return L;
}
```

**Step 6: Run test — expect PASS**

Run: `mise run build`

**Step 7: Commit**

```bash
git add runtime/extract/RenderItem.hpp runtime/extract/LightSystem.hpp runtime/extract/tests/light_system_test.cpp
git commit -m "feat(light): add EnvironmentLight → LightDesc::Type::Environment with SH + cubemaps"
```

---

# Phase 5: PoC consumer programs

**Spec reference:** §6 of the design doc.

## Task 5.1: Factor bind block into per-program dispatch

**Step 1:** Refactor `drawItem` lambda in `main.cpp` (lines 1139-1241) into a switch on material model + topology:

- `topology != Triangles || !hasNormals || model == Unlit` → `unlitProg`
- `model == Phong` → `phongProg` (extend existing `lit.frag`)
- `model == Physical` → `pbrProg` (new shader)
- `shaderProgram.has_value()` → `authorProg`

Each branch reads the right per-variant fields (via `mat.phong.*` / `mat.physical.*`).

**Step 2: Build PoC**

Run: `mise run poc`

**Step 3: Commit**

```bash
git add examples/poc_renderer/main.cpp
git commit -m "refactor(poc): per-program dispatch (unlit/phong/pbr/author)"
```

## Task 5.2: Extend lit.frag for Phong textures + normal mapping

**Step 1:** Add normal-map sampling, emissive texture, and per-slot `Has*Tex` uniforms to `examples/poc_renderer/shaders/lit.frag`.

**Step 2: Update the Phong bind branch** in `main.cpp` to bind units 0-5 per spec §6.

**Step 3: Build + smoke-test PoC**

Run: `mise run poc`

**Step 4: Commit**

```bash
git add examples/poc_renderer/shaders/lit.frag examples/poc_renderer/main.cpp
git commit -m "feat(poc): extend Phong shader with normal/emissive/specular textures"
```

## Task 5.3: New pbr.frag for PhysicalMaterial

**Step 1:** Create `examples/poc_renderer/shaders/pbr.frag` — glTF metallic-roughness BRDF + IBL (envDiffuse via SH, envSpecular via prefiltered cubemap, brdfLUT).

**Step 2:** Add PBR bind branch in `main.cpp` — reads `mat.physical.*`, binds `metallicRoughnessTex` (unit 3), env cubemaps, brdfLUT.

**Step 3: Build PoC**

Run: `mise run poc`

**Step 4: Commit**

```bash
git add examples/poc_renderer/shaders/pbr.frag examples/poc_renderer/main.cpp
git commit -m "feat(poc): add PBR shader (metallic-roughness + IBL)"
```

## Task 5.4: Author-shader dispatch with binding plan

**Step 1:** Add author-shader path to `drawItem`: if `it.shaderProgram` present, compile (cached), call `buildBindingPlan`, log diagnostics, upload per plan.

**Step 2:** Set `isValid` / `lastError` back-channel on `ShaderProgramDesc` after compile/link.

**Step 3: Build PoC**

Run: `mise run poc`

**Step 4: Commit**

```bash
git add examples/poc_renderer/main.cpp
git commit -m "feat(poc): author-shader dispatch with binding plan + diagnostics"
```

## Task 5.5: Gamma/sRGB output encoding

**Step 1:** Add `LINEARtoSRGB` to the end of `lit.frag` and `pbr.frag` (Phong opt-in via uniform; PBR always).

**Step 2:** Upload color textures with `GL_SRGB8_ALPHA8` internal format; data textures with linear format.

**Step 3: Build PoC**

Run: `mise run poc`

**Step 4: Commit**

```bash
git add examples/poc_renderer/shaders/*.frag examples/poc_renderer/main.cpp
git commit -m "feat(poc): gamma/sRGB output encoding + sRGB texture uploads (MAT-009)"
```

---

# Phase 6: Docs + drift + gates

**Spec reference:** §7 of the design doc.

## Task 6.1: New wiki pages

**Step 1:** Create `docs/wiki/subsystems/materials.md` — discriminated union, extension seam, backMaterial, MAT-006 closure.

**Step 2:** Create `docs/wiki/subsystems/shaders.md` — SAI lifecycle, vocab header, binding plan, two-channel split.

**Step 3:** Create `docs/wiki/subsystems/ibl.md` — EnvironmentLight, prefilter pipeline, SH vs cubemap.

**Step 4:** Add nav entries to `mkdocs.yml`.

**Step 5: Run docs-build gate**

Run: `mise run docs-build`
Expected: PASS (no dead links / nav orphans).

**Step 6: Commit**

```bash
git add docs/wiki/subsystems/materials.md docs/wiki/subsystems/shaders.md docs/wiki/subsystems/ibl.md mkdocs.yml
git commit -m "docs: add materials/shaders/ibl subsystem pages"
```

## Task 6.2: Conformance findings update

**Step 1:** Edit `docs/conformance/findings.yaml`:
- Close `MAT-006` (backMaterial shipped).
- Add `MAT-007` (texCoordMapping), `MAT-008` (ORM packing), `MAT-009` (gamma stance).

**Step 2:** Edit `docs/conformance/components/Shaders.md` — update from "L1 data-only" to "L1 + binding plan".

**Step 3: Regen conformance view**

Run: `mise run conformance`
Expected: regenerates `model.json` + `INDEX.md` + component `.md` files.

**Step 4: Run conformance gate**

Run: `mise run conformance-gate`
Expected: PASS.

**Step 5: Commit**

```bash
git add docs/conformance/
git commit -m "docs(conformance): close MAT-006; add MAT-007/008/009; update Shaders component"
```

## Task 6.3: ADR

**Step 1:** Find next free ADR number in `docs/wiki/decisions/` (currently 0001-0020; use 0021).

**Step 2:** Create `docs/wiki/decisions/0021-material-shader-design.md` — follows the ADR template (see `0014-dynamic-field-foundation.md`).

**Step 3:** Update `docs/wiki/coverage.md` with the new ADR row.

**Step 4: Commit**

```bash
git add docs/wiki/decisions/0021-material-shader-design.md docs/wiki/coverage.md
git commit -m "docs(adr): 0021 — material-shader design (discriminated union + vocab+introspection)"
```

## Task 6.4: Capabilities + coverage

**Step 1:** Update `docs/sdk/v1-capabilities.md` with material/shader/IBL capability rows.

**Step 2:** Update `docs/wiki/coverage.md` with new subsystem rows + counts.

**Step 3: Commit**

```bash
git add docs/sdk/v1-capabilities.md docs/wiki/coverage.md
git commit -m "docs: update capabilities + coverage for material/shader/IBL"
```

## Task 6.5: Run all gates

**Step 1: Run drift suggester**

Run: `mise run docs-drift working`
Expected: flags the touched code; review `CITES` hits on `materials.md`/`shaders.md`/`Shaders.md` first.

**Step 2: Refresh RAG stores**

Run: `mise run code-ingest && mise run docs-ingest`

**Step 3: Run full CI gate**

Run: `mise run ci`
Expected: PASS — tests + golden + conformance-gate + build + cli-gate-regression all green.

**Step 4: If any gate fails, fix and re-run until green.**

**Step 5: Final commit (if any fixes)**

```bash
git add -A
git commit -m "chore: gate fixes for material-shader system"
```

---

## Open questions (resolve during implementation)

- Exact `kVocabulary[]` table contents — drafted in Task 3.3, finalized against the PoC consumer as first client.
- Whether `X3DFieldValue` lives in its own header or folds into `X3Dtypes.hpp` — Task 3.1 decides based on include-graph cycle check.
- `AccessType` visibility for `ShaderFieldBinding` — confirm it's in `X3Dtypes.hpp` or include `X3DReflection.hpp`.
- `EnvironmentLight` cubemap `TextureRef` population — may need `matsys::refOf()` extended for cubemap texture nodes.
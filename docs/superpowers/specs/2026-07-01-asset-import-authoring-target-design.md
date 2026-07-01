# Asset-Import consumer + `x3d_cpp::authoring` slim target — design

- **Date:** 2026-07-01
- **Status:** Approved (brainstorm) — pending implementation plan
- **Topic:** A write-side canonical consumer (arbitrary model → standards-compliant X3D 4.0)
  that links a new, footprint-gated authoring-only library target.

## 1. Motivation

Every canonical consumer today is **read-side**: parse X3D → extract → render
(`cpu_raster`, `poc_renderer`). None exercises the **write/authoring** path, and nothing
proves how small an embed can be if it only needs to *emit* X3D.

This project adds both halves at once:

1. **The tool** — an asset importer/exporter (`x3d_asset_import`) that takes a wide variety
   of 3D models (via assimp) and emits standards-compliant **X3D 4.0** with minimal fuss:
   geometry + hierarchy, Phong **and** PBR materials, converted textures, cameras, and lights.
2. **The slim target** — a dedicated `x3d_cpp::authoring` CMake target containing *only* the
   node-construction + serialization + validation surface the exporter needs, with a
   **footprint gate** (symbol scan + size baseline) that keeps it minimal. The exporter is the
   proof that the slim target is self-sufficient.

The exporter's `emit` unit links **only** `x3d_cpp::authoring`. That constraint is the point:
building it *discovers and enforces* the true minimal authoring footprint.

## 2. Architecture & data flow

New consumer at `examples/asset_import/` (binary `x3d_asset_import`), four decoupled units
around one linear pipeline:

```
source file ─▶ [ImportSource backend] ─▶ ImportScene (POD IR)
                                              │
                              [emit] ─────────┘  (links ONLY x3d_cpp::authoring)
                                 │
                                 ▼
                            X3DDocument  ──▶ [self-validate: range-check + profile-fit]
                                 │                          │
                                 ▼                          ▼
                          [texture pipeline]         narrowest profile
                          decode→re-encode→assets/         │
                                 │                          ▼
                                 └──────────▶ writer ──▶  out.x3d + assets/*.png
```

Units, each independently testable:

- **`import_source`** — the seam: a narrow, frozen POD interface (`ImportScene`) + an abstract
  `ImportSource`. No assimp types leak past it.
- **`emit`** — the heart: `ImportScene` → `x3d::nodes` graph → `X3DDocument`. **Links only
  `x3d_cpp::authoring`.** The proof-of-minimal-footprint unit.
- **`texture_pipeline`** — consumer-side decode (SDK stb seam) + re-encode (`stb_image_write`)
  + copy into `assets/` + URL rewrite. Never touches the slim target.
- **`main`** — CLI glue: arg parse, backend selection, orchestration, self-validate, output,
  and the optional `--verify` round-trip (the only part that links beyond the slim target).

## 3. The `ImportSource` seam

Backend-agnostic POD IR so no importer's types reach the emit side:

```cpp
struct ImportMesh    { std::vector<Vec3> positions, normals; std::vector<Vec2> uv;
                       std::vector<Vec4> colors; std::vector<uint32_t> indices;
                       int materialIndex; };
struct ImportMaterial{ std::string name; Vec3 diffuse, emissive, specular; float shininess;
                       std::optional<PbrParams> pbr;               // baseColor/metallic/roughness
                       std::optional<TextureSlots> textures;       // per-slot source ref (path|embedded)
                       AlphaMode alpha; float opacity; };
struct ImportNode    { std::string name; Mat4 localTransform;
                       std::vector<int> meshIndices, childIndices; };
struct ImportCamera  { Mat4 world; float yfov, znear, zfar; };     // → Viewpoint
struct ImportLight   { enum Kind { Dir, Point, Spot } kind; Vec3 color, pos, dir;
                       float intensity, cutOffAngle, beamWidth; Vec3 attenuation; float radius; };
struct EmbeddedTexture { std::string key; std::vector<uint8_t> bytes; std::string hintExt; };
struct ImportScene   { std::vector<ImportNode> nodes; int rootNode;
                       std::vector<ImportMesh> meshes; std::vector<ImportMaterial> materials;
                       std::vector<ImportCamera> cameras; std::vector<ImportLight> lights;
                       std::vector<EmbeddedTexture> embedded; };

struct ImportSource  { virtual ImportScene load(std::string path) = 0; virtual ~ImportSource() = default; };
```

Two backends:

- **`AssimpSource`** — gated behind `X3D_CPP_BUILD_ASSIMP` (OFF by default, mirrors
  QuickJS/wuffs). Triangulates (`aiProcess_Triangulate`), generates smooth normals when
  missing (`aiProcess_GenSmoothNormals`), flattens `aiScene` into the IR, extracts embedded
  textures (`aiTexture`) as blobs.
- **`FixtureSource`** — a tiny dependency-free backend producing a handful of synthetic scenes
  (a cube; a two-material hierarchy; a lit + camera scene). CI compiles and runs this **without
  assimp**, and it is the *second independent backend* that keeps the seam honest. A real glTF
  or USD backend can later upgrade this into a full U1–U4 genericity proof — the seam is already
  shaped for it. (Note: assimp's USD support is thin/experimental via bundled `tinyusdz`; a
  first-class USD backend would be a dedicated OpenUSD/`tinyusdz` implementation behind this same
  interface, not via assimp.)

## 4. X3D 4.0 mapping table

| Import IR | → X3D 4.0 | Notes |
|---|---|---|
| `ImportNode` tree | nested `Transform` | `localTransform` Mat4 **decomposed to TRS** (`translation`, `rotation` axis-angle from quat, `scale`, `scaleOrientation`). X3D has no matrix-transform node; non-decomposable shear is a documented lossy case (rare post-triangulation). |
| shared mesh across nodes | `DEF`/`USE` on the `Shape` | Instances emit `USE`; exercises the writer's DEF path, avoids duplicating geometry. |
| `ImportMesh` | `Shape { IndexedTriangleSet, Appearance }` | |
| positions / indices | `IndexedTriangleSet.index` + `Coordinate.point` | Single shared index (matches assimp's one index buffer/vertex). `ccw=true`, `solid=false` (don't backface-cull unknown winding). |
| normals | `Normal` (`normalPerVertex=true`) | assimp gen-smooth if missing; fixture supplies them. |
| uv | `TextureCoordinate.point` | |
| vertex colors | `ColorRGBA` / `Color` | RGBA when alpha present. |
| `ImportMaterial` (Phong) | `Material` | `diffuseColor`, `specularColor`, `emissiveColor`, `shininess` (normalized 0–1), `transparency`=1−opacity, `ambientIntensity`. |
| `ImportMaterial` (PBR present) | **`PhysicalMaterial`** (X3D 4.0) | `baseColor`, `metallic`, `roughness`, `emissiveColor`, `transparency`. Chosen per-material when the source carried PBR params (glTF/USD). |
| textures | material **texture slots** (X3D 4.0) | Phong: `diffuseTexture`/`emissiveTexture`/`normalTexture`/`specularTexture`/`occlusionTexture`. PBR: `baseTexture`/`metallicRoughnessTexture`/`emissiveTexture`/`normalTexture`/`occlusionTexture`. Each slot holds an `ImageTexture` (see §5). |
| `ImportCamera` | `Viewpoint` | `position` + `orientation` (from world matrix), `fieldOfView`=yfov. First camera bound; extras unbound. |
| `ImportLight` Dir/Point/Spot | `DirectionalLight` / `PointLight` / `SpotLight` | `color`, `intensity`, `direction`/`location` baked to world, `global=true`. Spot: `cutOffAngle`/`beamWidth`. Point/Spot pass attenuation/radius. |
| whole scene | `X3DDocument` | `version="4.0"`; `head` gets `<meta>` for source filename + generator tag; `<unit>` emitted if the source declares length units (else meters, documented). |
| profile | **narrowest that fits** | Query the SDK profile-fit tables (`tools/x3d-cli/*.gen.inc`) with the set of emitted node types → `Interchange`→`Immersive`→`Full`. `PhysicalMaterial`/lights bump as required. |

Scene root order: `Viewpoint(s)` → global lights → root `Transform`.

## 5. Texture pipeline (consumer-side)

For every referenced texture, whether an external path or an assimp-embedded blob:

1. **Get bytes** — read the external file (resolved against the model's dir) or take the
   embedded blob.
2. **Dedupe** — content-hash the bytes; identical textures collapse to one output file +
   shared URL.
3. **Convert (sane default)** — if already web-safe (`png`/`jpeg`), **copy through** (lossless,
   no needless recompress); otherwise **decode via the SDK's stb texture seam**
   (`X3D_CPP_BUILD_STB`) → **re-encode to PNG** via `stb_image_write` (vendored consumer-side
   header alongside the existing stb decode). `--recompress` forces re-encode.
4. **Write** to `<outdir>/assets/<name>.<ext>` (name from content hash + source basename).
5. **Wire** — `ImageTexture.url = ["assets/<name>"]`, relative. Sampler defaults:
   `repeatS/repeatT=true`; assimp wrap/filter modes map to a `TextureProperties` node only when
   they differ from defaults.

This unit links **stb + std**, never `x3d_cpp::authoring` — the emit side only ever sees a
finished relative URL string. Textures add zero weight to the slim target.

## 6. The `x3d_cpp::authoring` slim target

**Umbrella header** `include/x3d/authoring.hpp` re-exports *only* the emit subset (into
`x3d::authoring`, same underlying symbols as `x3d::sdk`):

- **Node construction** — the `x3d::nodes` generated bindings (`X3DNode`, all node types,
  `fields()`/get/set/DEF).
- **Containers** — `X3DDocument`, `Scene`, `Head`, `Component`, `Unit`, `Meta`, `Route`, `Profile`.
- **Writers** — `XmlWriter`, `JsonWriter`, `VrmlWriter`, `CanonicalXmlWriter`, `writeDocument`.
- **Validation/fit** — `X3DRangeValidate` + the profile-fit query.

**Excluded** (and the gate proves it): parse, `X3DExecutionContext`, extract/`SceneExtractor`,
`MeshBuilder`, physics, script/SAI, sound, io/resolvers, Inline/PROTO expansion, all render
seams. The umbrella must not transitively `#include X3DParse.hpp` etc. If a writer/scene header
drags one in, that is the coupling finding Approach A anticipated — sever it surgically.

**CMake target** `x3d_cpp::authoring` — a STATIC lib whose source list is an **explicit
allowlist** (writers, scene containers, range-validate, math — no more). Building it is how we
*discover* the true minimal source set: start from the header, add sources until `emit` + a
smoke `main` link, and that resulting set *is* the documented footprint.

**Footprint gate** (`mise run authoring-footprint`, wired into CI + `mise run ci`):

1. Builds `x3d_cpp::authoring` standalone + a tiny smoke binary that links *only* it.
2. **Symbol scan** (`nm`): the smoke binary must contain **zero** symbols from the excluded
   subsystems — a hard fail. This is the "slim" enforcement.
3. **Size baseline**: records stripped `.a` size + smoke `.text` size into a committed baseline
   TSV (same pattern as `cli-gate` baselines); regression beyond a threshold fails. Accepting a
   legitimate growth = refresh the baseline in the diff.

## 7. CLI

```
x3d_asset_import <input> [-o out.x3d] [--format xml|json|vrml|canonical]
  [--assets-dir DIR] [--no-textures] [--recompress]
  [--profile auto|interchange|immersive|full] [--verify] [--stats]
```

- `<input>` = a real model path (→ `AssimpSource`, when built with assimp) **or**
  `fixture:<name>` (→ `FixtureSource`, always available — CI drives this without assimp).
- Default output: `<stem>.x3d` + adjacent `assets/`. Self-validates (range-check + profile-fit),
  prints `--stats`, nonzero exit on hard-invalid.
- `--verify` re-parses the output with the **full SDK parser** and diffs a structural summary
  (node/route counts) to confirm round-trip. This is the only part of the binary that links
  beyond the slim target, and it lives in `main`, never in `emit`.

## 8. Testing

Doctest, repo style:

- **emit** unit — fixture IR → assert graph shape + fields: TRS-decomposition correctness,
  PBR-vs-Phong selection, DEF/USE instancing, texture-slot wiring. Links authoring only.
- **texture_pipeline** — dedupe, passthrough-vs-reencode, URL rewrite, `assets/` layout.
- **import_source** — fixture backend contract; assimp backend (behind gate) loads a committed
  tiny `.glb`/`.obj` → IR assertions.
- **golden** — `fixture:cube` + a couple synthetic scenes → committed **canonical-form** goldens
  with a golden-drift check (mirrors existing golden gates).
- **round-trip** — each golden re-parsed by the SDK parser: parses clean, zero range warnings.
- **footprint** — the symbol-scan + size baseline gate (§6).

## 9. CI & tasks

- Extend `scripts/validate-examples.sh` / examples-gate to build + headless-smoke this consumer
  **twice** (with and without `X3D_CPP_BUILD_ASSIMP`). No-assimp job: `fixture:*` conversions +
  goldens + footprint gate (all wired into `mise run ci`, tightening the BLD-3 residual).
  With-assimp: convert the committed tiny model (optional job if assimp is absent in CI).
- New mise tasks: `asset-import` (configure + build + ctest, mirroring `cpuraster`/`poc`) and
  `authoring-footprint` (the slim-target gate).

## 10. Docs (docs-are-part-of-diff)

- New example/subsystem page (`docs/wiki/subsystems/asset-import.md`) + `coverage.md` row +
  mkdocs nav.
- **ADR** for the `x3d_cpp::authoring` boundary (a binding design decision).
- `docs/sdk/v1-capabilities.md` — add the authoring/export capability + slim target.
- Root **NOTICE** updated for assimp + `stb_image_write` (Definition of Done).
- Run `mise run docs-drift`; write the "what the emit path actually drags in" finding into the
  subsystem page.

## 11. Scope & deferrals

**In v1:** geometry + hierarchy, Phong + PBR materials, converted+wired textures with sane
sampler defaults into an `assets/` folder, cameras, lights; XML default output with profile-fit
+ self-validation; the slim target + footprint gate; assimp behind a seam + a fixture backend.

**Deferred (documented):** skeletal/vertex animation and morph targets; a first-class USD backend
(OpenUSD/`tinyusdz`); non-decomposable (shear) transforms beyond TRS approximation; unit
inference beyond source-declared length units; `MultiTexture` composition beyond single-slot
textures; a full U1–U4 seam-genericity proof for `ImportSource` (fixture is the second backend
for now). Behavioral/spec gaps land in `docs/conformance/findings.yaml`; engineering deferrals on
GitHub Project #2.

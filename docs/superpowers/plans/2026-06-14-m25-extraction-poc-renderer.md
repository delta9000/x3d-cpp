# M2.5 Extraction API + PoC OpenGL Renderer — Implementation Plan

**Date:** 2026-06-14. Companion to the design spec. 14 tasks, dependency-ordered as a vertical slice.

**First light = T0 → T1 → T7a → T9 → T10** (unlit triangle on Wayland). Everything after widens behind an already-rendering pipeline.

Invariants on every task: header-only, golden BYTE-UNCHANGED, node-as-truth (side tables keyed by `const X3DNode*`), renderer-as-consumer (PoC is OUT of the SDK, behind `X3D_CPP_BUILD_POC=OFF`).

| ID | Title | Depends on |
|----|-------|-----------|
| **T0** | extract/RenderItem.hpp descriptor layer (pure POD, corrected) | — |
| **T1** | Promote MeshBuilder.hpp from PickSystem + re-seat PickSystem (BIT-IDENTICAL goldens) | T0 |
| **T7a** | Minimal SceneExtractor (vertical-slice spine) — unblocks first light | T1 |
| **T9** | PoC M0 — Wayland GLFW window + glad core-3.3 + camera (unblocked today) | — |
| **T10** | PoC M1 — draw one extracted mesh, unlit-flat (FIRST LIGHT) | T7a, T9 |
| **T2** | MeshBuilder: indexed/strip/fan/quad + ElevationGrid with winding + bounds guards | T1 |
| **T3** | MeshBuilder: normal/color/texcoord resolution + flat-normal gen + expansion policy | T2 |
| **T4** | MeshBuilder: primitive tessellators (Box/Sphere/Cone/Cylinder) + OPT-IN pick upgrade | T1 |
| **T5** | extract/MaterialSystem.hpp reflection flattener | T0 |
| **T6** | extract/LightSystem.hpp world-resolved collection (+scope+global) | T0 |
| **T7** | SceneExtractor full form — visibility-aware DFS + 3 reverse indices + entry-matrix cache | T3, T4, T5, T6, T7a |
| **T8** | SceneExtractor delta() incremental engine (per-path-correct, pollution-safe) | T7 |
| **T11** | PoC M2 — N meshes, per-path world transforms, incremental upload | T8, T10 |
| **T12** | PoC M3 — per-pixel lighting + Material color + culling | T11 |
| **T13** | PoC M4 (deferred/optional) — textures via asset resolver | T12 |

## Task detail

### T0 — extract/RenderItem.hpp descriptor layer (pure POD, corrected)
*(deps: —)*

Define PathKey (vector<const X3DNode*>, full-vector equality; hash is bucket-only), RenderItemId (dense handle, NOT raw hash), GeomId(node*+contentVersion), MeshData(positions/indices/normals/texcoords/colors/ccw/solid/hasNormals/hasColors; header note: local-frame, no V-flip, indices always populated), MaterialDesc(tagged superset + AlphaMode/alphaCutoff + toRGBA() with header-documented alpha=1-transparency), TextureRef/Slot/Source/SamplerParams, LightDesc(+global+scopeRoot), CameraDesc(+ortho+cameraChanged; fieldOfView=min-dimension note), BackgroundDesc(+backgroundChanged), RenderDelta(added/removed/updatedTransform/updatedGeometry/updatedMaterial + camera/background/lightsChanged bits). NO changeBits on RenderItem. Header-only; depends only on X3Dtypes+Mat4+Aabb+std. Compile-only golden test. Mark MultiTexture/alphaMode/OrthoViewpoint fields 'descriptor-only, not exercised by PoC' in comments.

### T1 — Promote MeshBuilder.hpp from PickSystem + re-seat PickSystem (BIT-IDENTICAL goldens)
*(deps: T0)*

Move extractTriangles into extract/MeshBuilder.hpp as buildLocalMesh(geom,MeshBuildOptions)->MeshData covering EXACTLY IFS(fan)/IndexedTriangleSet(triples)/TriangleSet(implicit) with ok()-bounds guards. Re-seat PickSystem::narrowPhase to build the local mesh once for THESE THREE TYPES ONLY, keep the Sphere analytic fast-path AND the rayAabb fallback for all other types. ACCEPTANCE: every existing pick golden passes UNCHANGED (bit-identical). Do NOT add primitive mesh-picks here.

### T7a — Minimal SceneExtractor (vertical-slice spine) — unblocks first light
*(deps: T1)*

extract/SceneExtractor.hpp MINIMAL form: holds const X3DExecutionContext&; full walk over grouping+Shape nodes accumulating worldM (worldOfRec pattern), emitting one RenderItem per (geometry-bearing Shape, path) for ONLY T1's three types; MaterialDesc = Unlit-white stub; NO LightSystem, NO Switch/LOD visibility logic yet. PathIndex interning + per-path worldTransform. fullSnapshot()->RenderDelta(all in 'added'). camera()->CameraDesc reading viewMatrix()+Viewpoint.fieldOfView. sceneWorldBounds() over emitted items. ACCEPTANCE: a DEF'd Shape USE'd under two Transforms yields TWO RenderItems with distinct worldTransforms while ctx.worldTransform(node) returns ONE (the M2C-1 acceptance test).

### T9 — PoC M0 — Wayland GLFW window + glad core-3.3 + camera (unblocked today)
*(deps: —)*

examples/poc_renderer/ skeleton; CMakeLists FetchContent GLFW>=3.4 (Wayland on, X11 ON safety net) + PRE-GENERATED glad sources committed at third_party/glad/ (glad.c, glad/gl.h, KHR/khrplatform.h — real files). Root CMake option(X3D_CPP_BUILD_POC OFF)+conditional add_subdirectory; link x3d_cpp::x3d_cpp. main.cpp: parseFile->getScene->buildSceneGraph; CHECK BridgeResult from buildFrom and bail/log on failure; tick(0); GLFW hints (CONTEXT 3.3, CORE_PROFILE, FORWARD_COMPAT, DEPTH_BITS 24); glad init; GL_KHR_debug callback; clear to boundBackground color; build view=ctx.viewMatrix()+PoC perspective; forced fullscreen-clear sanity step. README lists exact Arch prereqs (wayland, wayland-protocols, libxkbcommon, mesa/EGL, libnvidia-egl-wayland). Optional `mise run poc` task. Verify window opens on Wayland, FPS loop runs.

### T10 — PoC M1 — draw one extracted mesh, unlit-flat (FIRST LIGHT)
*(deps: T7a, T9)*

Sub-deliverable: ship a tiny hand-written triangle .x3d (TriangleSet or IFS) at examples/poc_renderer/assets/. Construct SceneExtractor(ctx) [T7a minimal]. fullSnapshot(); GpuMesh cache keyed by GeomId; upload positions+indices; constant color from MaterialDesc.toRGBA; GL_DEPTH_TEST on, GL_CULL_FACE OFF (double-sided); flat.vert/frag; mvp=projection*view*model; near/far from sceneWorldBounds(). ACCEPTANCE: the triangle renders on Wayland/NVIDIA.

### T2 — MeshBuilder: indexed/strip/fan/quad + ElevationGrid with winding + bounds guards
*(deps: T1)*

Add IndexedTriangleFanSet/StripSet/QuadSet, TriangleFanSet(fanCount)/StripSet(stripCount)/QuadSet, ElevationGrid(height/xDimension/zDimension/xSpacing/zSpacing). Per-triangle strip winding flip. ok()-style guards on every index/count array. Tests: fan-triangulation counts, strip winding (explicit — wrong flip inverts normals), malformed-index safety.

### T3 — MeshBuilder: normal/color/texcoord resolution + flat-normal gen + expansion policy
*(deps: T2)*

Resolve authored Normal/Color(MFColor)/ColorRGBA(MFColorRGBA)/TextureCoordinate honoring normalPerVertex/colorPerVertex + normalIndex/colorIndex/texCoordIndex. Promote SFColor->SFColorRGBA(alpha=1). Generate FLAT normals when absent. Drive indexed-vs-expanded off actual *PerVertex flags; trivial 0..N-1 index for expanded. Carry ccw/solid via getField<SFBool>(geom,'solid'/'ccw',true). Set MeshData.hasColors (per-vertex Color overrides Material diffuse — documented). Tests: flat-normal correctness, colorIndex/normalIndex honoring, per-face expansion, ColorRGBA-vs-Color promotion.

### T4 — MeshBuilder: primitive tessellators (Box/Sphere/Cone/Cylinder) + OPT-IN pick upgrade
*(deps: T1)*

Parametric tessellation via MeshBuildOptions density (sphere 16x16, cone/cyl 24 radial), CCW outward normals. Validate triangle/vertex counts + outward normals. SEPARATELY (opt-in flag / distinct follow-up): upgrade PickSystem primitives from rayAabb proxy to mesh picks WITH its own regenerated pick goldens — do NOT fold under T1's behavior-identical banner.

### T5 — extract/MaterialSystem.hpp reflection flattener
*(deps: T0)*

materialOf(appearance)->MaterialDesc dispatching on nodeTypeName() via getField<SFColor>/<SFFloat> by spec field name; null material+Appearance=>Phong 0.8 grey; null Appearance=>Unlit white; alphaMode/alphaCutoff off Appearance; toRGBA alpha=1-transparency. texturesOf(appearance)->vector<TextureRef> (material-slot precedence over Appearance.texture, sampler off textureProperties, PixelTexture->Inline, MovieTexture->Movie, MultiTexture->channels). Tests: Material diffuse/transparency (assert default toRGBA().a==1.0), PhysicalMaterial baseColor/metallic/roughness, null->grey/white per case.

### T6 — extract/LightSystem.hpp world-resolved collection (+scope+global)
*(deps: T0)*

Copy PickSystem roots_/forEachChild/worldOf accumulation. collect(): walk accumulating worldM; for Directional/Point/SpotLight with on==true emit LightDesc (worldDirection=transformDirection.normalized, worldLocation=transformPoint), reading color/intensity/ambientIntensity/attenuation/radius/beamWidth/cutOffAngle generically; CARRY authored global (verified DirectionalLight default false) + scopeRoot (enclosing grouping node). Tests: PointLight location world-resolves under a translating Transform; DirectionalLight global reads false by default (no silent promotion).

### T7 — SceneExtractor full form — visibility-aware DFS + 3 reverse indices + entry-matrix cache
*(deps: T3, T4, T5, T6, T7a)*

Extend T7a: visibility DFS special-casing Switch (whichChoice default -1 => nothing; recurse only children[whichChoice]) and LOD (children[0] static, documented) BY nodeTypeName BEFORE the generic child loop (NOT forEachChild). Build THREE reverse indices (transformDeps/geomDeps/materialDeps) + interior-node entry-matrix cache. Wire MeshBuilder[T3/T4] + MaterialSystem[T5] + LightSystem[T6]. camera()/lights()/background() read-outs (OrthoViewpoint surfaced ortho=true). Anchor RenderItem emission on hasField('geometry'). ACCEPTANCE: Switch whichChoice=-1 draws nothing; whichChoice=1 draws only child 1; LOD draws one child. Update BACKLOG M2C-1 -> CLOSED with this commit.

### T8 — SceneExtractor delta() incremental engine (per-path-correct, pollution-safe)
*(deps: T7)*

delta()->RenderDelta with ASSERTED one-delta-per-tick contract (tick() clears dirty_ at line 103). DirtyLocalTransform/DirtyWorldTransform on Transform => updatedTransform via transformDeps, RE-ACCUMULATE worldM along stored PathKey (O(path), never ctx.worldTransform()). content-DirtyField on geometry (field-name in content subset, NOT bare DirtyBounds) => bump contentVersion + updatedGeometry via geomDeps. DirtyField on appearance-subtree => updatedMaterial via materialDeps. DirtyChildren on grouping node => resume subtree walk from cached entry matrix => added/removed. Transform-only tick MUST NOT re-walk. camera/lights/background full with change bits. Tests: transform-only tick yields ONLY updatedTransform (no re-walk, USE-shared placement updates correctly); scale-animation on parent does NOT mark updatedGeometry; material-color route yields updatedMaterial; DirtyChildren yields added/removed.

### T11 — PoC M2 — N meshes, per-path world transforms, incremental upload
*(deps: T8, T10)*

Iterate RenderItems; model=item.worldTransform (per-path). Each frame: exactly one delta(); re-extract+re-upload only updatedGeometry GeomIds (glBufferData orphan), refresh model for updatedTransform, add/remove for added/removed; GC GpuMeshes whose GeomId left. Still GL_CULL_FACE OFF. ACCEPTANCE: a route/animation-driven corpus scene updates incrementally; a USE'd-shape-under-two-Transforms scene renders BOTH placements at distinct transforms.

### T12 — PoC M3 — per-pixel lighting + Material color + culling
*(deps: T11)*

Add normals attribute + lit.vert/frag (Lambert+ambient, two-sided N=gl_FrontFacing?N:-N). No-explicit-light fallback = NavigationInfo headlight (default true) camera-space directional; otherwise honor LightDesc Directional (direction/color/intensity), do NOT promote global=false to scene-wide. Honor MeshData.diffuse/emissive/specular/transparency + per-vertex Color override. Enable GL_CULL_FACE honoring MeshData.ccw (ccw=true=>GL_CCW front) + solid=true=>cull back. ACCEPTANCE: a real corpus IFS scene renders lit with correct front faces.

### T13 — PoC M4 (deferred/optional) — textures via asset resolver
*(deps: T12)*

Vendor stb_image; resolve ImageTexture url MFString relative to the parsed-file dir (asset resolver lives in the PoC, outside the SDK); glTexImage2D; sample using extracted texcoords (NO V-flip — X3D origin bottom-left=GL). Only after M3 solid; out of first-light scope.


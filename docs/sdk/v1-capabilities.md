# x3d-cpp-gen SDK — v1 capability matrix

What the v1 façade (`x3d::sdk`) supports, and what is deferred to post-v1. The
deferred rows are breadth, not spine: the architecture accommodates each as an
isolated addition. Source of record: `docs/superpowers/specs/2026-06-16-v1-closure-roadmap-design.md`.
Live deferral tracking lives in the two trackers `CLAUDE.md` designates (the
`docs/superpowers/BACKLOG.md` tracker is **deprecated**, 2026-06-22): behavioral /
spec-conformance gaps in [`docs/conformance/findings.yaml`](../conformance/findings.yaml);
engineering / planning deferrals in the
[GitHub Project](https://github.com/users/delta9000/projects/2).

## In v1 (supported)

| Capability | Notes |
|---|---|
| Load 4 encodings (XML, ClassicVRML, VRML97, JSON) | + gzip input, versions 3.0–4.1, lenient read, BOM strip |
| PROTO / EXTERNPROTO | local PROTO always expands; file-local EXTERNPROTO via `localFileProtoResolver` (http/urn skipped by default) |
| Conformance diagnostics | `rangeWarnings` (out-of-range values) + `protoWarnings` (expansion issues); per-version validation moat |
| Serialization | `XmlWriter` / `JsonWriter` / `VrmlWriter`, reflection-driven |
| Scene graph | DEF/USE, Transform hierarchy (+ HAnimHumanoid/HAnimJoint/CADPart), bounds, binding stacks |
| View-dependent nodes | LOD, Billboard, ProximitySensor, VisibilitySensor |
| Animation | TimeSensor, full interpolator family, Followers (§39 Damper + Chaser, all 14 node types), ROUTE cascade |
| Custom behavior | subclass `System`, `ctx.addSystem`, dirty-aware `ctx.writeField` |
| Input seam | `setPointer` / `setPointerButton` / `setPointerPresent` / `setKey` |
| Pointing-device sensors | TouchSensor, PlaneSensor, CylinderSensor, SphereSensor (drag) |
| Navigation (collision-free) | EXAMINE, FLY, LOOKAT, NONE |
| Picking engine | ray cast + closest hit; exact for Sphere/Box/Cone/Cylinder + indexed/triangle meshes; AABB proxy for the long tail |
| Extraction → render feed | full snapshot + incremental delta; meshes, materials, lights (scoped), camera, background, scene bounds |
| Mesh primitives + sets | Box/Sphere/Cone/Cylinder, IFS/ITS/TriangleSet/strip/fan, ElevationGrid, Extrusion, IndexedLineSet/LineSet/PointSet. **Caveat:** ElevationGrid/GeoElevationGrid use auto flat normals only — authored `color`/`normal` (and `colorPerVertex`/`normalPerVertex`) are dropped (EXT-001); the 8 §14 2D primitives are not extracted (G2D-1, see deferred) |
| NURBS curve + patch | `NurbsCurve` → line mesh, `NurbsPatchSurface` → triangle mesh with analytic normals + implicit `(u,v)` texcoords, via the first-party I/O-free `runtime/extract/NurbsEval.hpp` (Cox–de Boor + rational eval; full periodic `closed`/`uClosed`/`vClosed`; clamped-uniform default knots; control-point convex-hull bounds; ADR-0040, closes NRB-1 for curve+patch). **Caveat:** trimmed/swept/swung surfaces, NURBS interpolators, authored `NurbsTextureCoordinate`, and double-precision CAD fidelity are deferred (NRB-2/NRB-3, see deferred) — they route through the `externalGeometryResolver` fallback |
| Normals | flat + creaseAngle smoothing (lattice-adjacency) |
| Texture coordinates | default per-primitive (§13), authored TextureCoordinate(Generator), TextureTransform |
| Texture descriptors | `TextureRef` (url/repeat/sampler/texCoordMapping multi-UV), resolved RGBA via the `TextureResolver` seam; ORM channel-packing documented (MAT-007, MAT-008) |
| Material descriptors | `MaterialDesc` discriminated union: Phong (`Material`) / Physical (`PhysicalMaterial`) / Unlit (`UnlitMaterial`); `MaterialModel` tag; `backMaterial` (unique_ptr, two-sided, MAT-006); `doubleSided`; `toRGBA()`; gamma/sRGB stance documented (MAT-009) |
| Author-shader INFRASTRUCTURE | `ShaderProgramDesc` / `ShaderStageDesc` / `ShaderFieldBinding` descriptors + `ShaderUniformVocabulary` typed portability surface + `buildBindingPlan()` vocab/author-field/unrecognized dispatch with Levenshtein suggestions. **ComposedShader extraction wiring** (populating `RenderItem::shaderProgram` from the scene graph) **is a deferred follow-on** — the infrastructure is defined and tested but `shaderProgram` is never set by any extractor codepath today. |
| Text | FontMetrics seam + layout engine (justify/spacing/length/maxExtent) + glyph render-items + Text output fields. **Caveat:** `justify` END mis-aligns the minor axis for `topToBottom=FALSE` (horizontal) and `leftToRight=FALSE` (vertical) — TXT-2/TXT-4 |
| Scripting / SAI | ECMAScript via Duktape, in-process SAI core; `ScriptEngine` / `ScriptSystem` / `SaiContext`. **File-authored Scripts supported across all 4 encodings** (XML `<field>`+CDATA, ClassicVRML/VRML97 interface decls + `url` body, JSON field array + source): author fields land in the `DynamicFieldStore` (visible via `effectiveFields` to ROUTE + SAI resolution), inline source captured into `Script.sourceCode`, handlers dispatch, author `outputOnly`/`inputOutput` writes drive the cascade (SCR-SAI-DYN S1) |
| Asset resolver / IO seam | `AssetResolver` (`function<AssetResult(url, AssetKind)>`) — header-only std-function callback with two invocation contracts (render-time Pending-allowed + parse-time SYNC); proven generic via libcurl HTTP + AWS S3 SDK backends (ADR-0023, [seam-status GREEN](../wiki/seam-status.md)). Embedder opts in via `-DX3D_CPP_BUILD_CURL=ON` and/or `-DX3D_CPP_BUILD_S3=ON`; default build (both OFF) keeps the bytes-not-loaded-here invariant |
| Geo (flat-fallback) | GeoElevationGrid renders unanchored without a `GeoProjection`; anchored with one |
| Sound / Audio (§16) — **seam in v1; §16 node coverage partial** | The engine-agnostic `AudioBackend` seam + dependency-free `BuiltinDspBackend` ship and are proven generic (second backend miniaudio, `-DX3D_CPP_BUILD_MINIAUDIO=ON`, + the headless `x3d_sound_swaptest`; ADR-0020/0026): synthesis numerically (RMS ±2%, Goertzel ±5%) + spatialization structurally. **But §16 node coverage is narrow:** only `OscillatorSource` (source) + `Gain`/`BiquadFilter` (processing) are built — other sources (`AudioClip`/`Buffer`/`Stream`/`MicrophoneSource`) and processing nodes (`Analyser`/`Convolver`/`Delay`/`DynamicsCompressor`/`WaveShaper`) are skipped (SND-4/5); `enabled` is ignored (SND-1), the time-dependent lifecycle (startTime/stopTime/loop/isActive) is **inert** (SND-2), `tailTime`/`periodicWave` ignored (SND-8/9), and HRTF/Doppler/ellipsoid + a LabSound backend are deferred (SND-3). See `docs/conformance/components/Sound.md`. |
| Rigid-body physics (§37) | engine-agnostic core seam + flag-gated Jolt backend (`-DX3D_CPP_BUILD_PHYSICS=ON`, Jolt v5.5.0; ADR-0019): RigidBodyCollection over Box/Sphere/Cylinder/Cone + gravity + forces/torques/damping + collision response + CollisionSensor reporting (contacts/intersections/isActive) + Ball/Hinge/Slider joints. Remaining §37 fields/nodes ignored (see [physics.md](../wiki/subsystems/physics.md) §37 matrix + `CONF-RBP*`) |
| Authoring / Export pipeline | `x3d_cpp::authoring` slim header target (nodes, document model, XML/JSON/VRML codecs, range validation, profile fit). Footprint gate (`authoring-footprint.sh`) enforces symbol quarantine. CLI `x3d_asset_import` converts 3D assets to conformant X3D 4.0 with deduplicated, recompressed textures in `assets/`. Backends resolve through a priority `BackendRegistry` (`--backend` override); **glTF is first-class via the default-ON header-only cgltf backend** (no assimp required), with assimp as the broader-format fallback and USD/USDZ opt-in — the `ImportSource` seam's genericity proven by a cgltf-vs-assimp swap-test (ADR-0044). |


## Post-v1 (deferred, with reasons)

Not exposed in the façade — not even as experimental. These are scene-dependent
breadth. Each is tracked as a card in the
[GitHub Project](https://github.com/users/delta9000/projects/2).

| Deferred | Reason |
|---|---|
| WALK navigation + collision / terrain-following / gravity | Requires an avatar-volume collision subsystem (volume sweep, `Collision` node, step height, gravity); WALK is non-conformant without it. FLY ships collision-free. |
| EXPLORE mode; ANIMATE transition curve; MPEG-object LOOKAT | Not in the Core/Interchange minimum; spline curve + media-object seam have no corpus coverage. |
| Pick-sensor nodes (Line/Point/Primitive/VolumePickSensor) | Read `pickingGeometry`/`pickTarget`, distinct from the pointer-device seam; no current consumer (the pick *engine* ships). |
| Full / dynamic SAI (`createX3DFromString`, runtime node add/remove) | Dynamic structural mutation needs incremental re-indexing; no current consumer. |
| NURBS trimmed/swept/swung surfaces + interpolators | Curve + patch ship in v1 (see *In v1*, ADR-0040). The remaining NURBS surfaces (`NurbsTrimmedSurface`/`NurbsSweptSurface`/`NurbsSwungSurface`) need 2D contour-loop clipping or profile-sweep eval (NRB-3); the three NURBS interpolators (NRB-2) and authored `NurbsTextureCoordinate` are also deferred. Lowest-impact corpus slice. |
| 2D geometry (Arc2D, ArcClose2D, Circle2D, Disk2D, Polyline2D, Polypoint2D, Rectangle2D, TriangleSet2D) | All 8 §14 Geometry2D nodes are absent from `recognizedGeometryType()` — extraction silently drops them (G2D-1). |
| Geospatial (full projection) | Geo-accurate bounds/anchoring need the GEO coordinate projection; flat-fallback ships. |
| Layering / Layout (per-layer binding + view volumes) | Needs binding stacks + view-dependent eval keyed by layer. |
| H-Anim (full), Particle systems | Advanced components; breadth beyond the common-scene v1 target. (Rigid-body physics and audio ship as seams — see *In v1*.) |
| EnvironmentLight / IBL (image-based lighting) | `EnvironmentLight` is an X3D 4.1 node; `generated_cpp_bindings/` is code-generated from the 4.0 UOM with a byte-identical golden invariant. A hand-authored 4.1 binding conflicts with the golden gate. Needs a defined strategy for 4.1 extension nodes. The `ShaderUniformVocabulary` already reserves `envDiffuse`/`envSpecular`/`envSH`/`brdfLUT` entries for future use. |
| MultiTexture compositing, MovieTexture frames | Beyond the single-channel `TextureRef` descriptor. |
| Bidi / complex text shaping (language field) | Beyond left-to-right / top-to-bottom layout. |

## Known v1 limitations (work, with caveats)

| Limitation | Impact |
|---|---|
| `worldTransform(node)` / per-item path keying uses the first path | A node reached through a USE'd interior group reports its first-path transform. |
| VisibilitySensor uses a forward cone, not a 6-plane frustum | Conservative (over-reports visible, never wrongly culls); consumers can supply planes. |
| `MaterialDesc::textures[]` is descriptor-only in the first PoC | Populated but the SDK does not itself consume it; the consumer binds. |
| `RenderDelta` unsupported-geometry push channel not wired | Use the `skippedGeometryCounts()` pull accessor instead. |
| Fog is inert — bound and round-tripped, but no fog effect is rendered | `boundFog()` has no production callers and the shader path has no fog uniform; tracked as conformance finding ENV-10 (`docs/conformance/findings.yaml`). |
| Per-light shadows are not generated — the §17 modulation is hardcoded to "unobstructed" | `X3DLightNode.shadows`/`shadowIntensity` round-trip but `shadowTest` is fixed at 1; the `ShadowQuery` seam + reference modulation are designed (ADR-0028 / ADR-0027) but not yet wired. |

## v1 gate record

**Historical snapshot — the T-GATE sweep (2026-06-17), not kept live.** These
were the verified gate values at v1 closure (the baseline for conformance
tracking); they are a point-in-time record. For current state run `mise run ci`
— the suite has grown since (e.g. `subsystems/physics.md` records ctest at
150/150 with physics OFF; the current corpus scan is in
`docs/conformance/REPORT.md`).

| Metric | Value |
|---|---|
| Golden | BYTE-IDENTICAL (`*.hpp` regenerated == committed; no `.cpp` drift) |
| Golden hash (*.hpp + *.cpp) | `7f67529bc380ca6425a24f679fda8804d7b9668448a26bcc57cf3256ec415b30` (UNCHANGED — the SCR-SAI-DYN S1 / RTC-5/6 work is codegen-free; the generated tree stays byte-identical to T-GATE) |
| ctest count | **110/110 passed** (0 failures) — was 94/94 at T-GATE; +7 file-authored-Script un-tabling; +3 corpus-smoke widening; +1 audit harness; +5 corpus-correctness fixes (VRML MF bracket, ProtoInstance round-trip, ProtoBody DEF scope, enum-quote) |
| ctest time | ~15 s total (corpus subset dominates) |
| Corpus smoke (widened) | **Full conformance archive: 17,719 files swept, 0 crashes, 0 pipeline errors, 99.95% OK** (9 parse-rejects = genuinely malformed/unsupported input, caught cleanly; 292,122 render items; skipped geometry = NURBS + 2D primitives, both documented post-v1). On-demand via `mise run corpus`; bounded 250-file subset wired as the `x3d_corpus_smoke` ctest. Surfaced + fixed a containment-cycle stack overflow (`ec9bc83`). |
| Branch | `modernize-x3d-spec` (the v1-closure dev branch; since merged to `main`) |
| Commit | see `git log --oneline` (T-GATE sweep commit) |

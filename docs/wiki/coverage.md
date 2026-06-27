---
title: Coverage Manifest
summary: Full target set for the wiki — every subsystem, decision, and guide; tracks covered vs. planned and is the source of truth for "100% coverage".
tags: [coverage, manifest, meta]
updated: 2026-06-24
related:
  - index.md
  - architecture.md
  - subsystems/cli-suite.md
  - decisions/0001-ext-firewall.md
  - guides/gate-system.md
---

# Coverage Manifest

This is the canonical enumeration of the wiki's **target set** — every page the wiki intends to have. "100% coverage" is measured against this manifest: every row must map to a page that meets its template (see the project wiki design spec at `docs/superpowers/specs/2026-06-20-project-wiki-design.md`, section *"100% coverage" — definition & measurement*).

Each row carries a stable slug so Waves 2–3 fill it deterministically:

- **Subsystems** → `subsystems/<slug>.md` (use the subsystem template).
- **Decisions (ADRs)** → `decisions/NNNN-<slug>.md` (use the ADR template, numbered sequentially).
- **Guides** → `guides/<slug>.md`.

**Status legend:** `covered` = page exists and meets its template (Wave 1 vertical slice, or later filled). `planned` = slug is reserved; page is authored in Wave 2 (subsystems + decisions) or Wave 3 (guides + completeness pass).

The Wave-1 vertical slice marks three rows `covered`: the [CLI Suite subsystem](subsystems/cli-suite.md), [ADR-0001 Ext Firewall](decisions/0001-ext-firewall.md), and the [Gate System guide](guides/gate-system.md). Everything else is `planned`.

This manifest is derived from the directory structure under `runtime/`, `src/x3d_cpp_gen/`, and `tools/`; from the dated specs in `docs/superpowers/specs/`; and from the project auto-memory. The Wave-3 completeness critic re-derives it to catch anything new.

---

## 1. Subsystems

Every top-level functional area gets one page. The canonical list follows the architecture layers: generator → generated bindings → runtime core → codecs/parse → event/behavior systems → extract → ext firewall → SDK → CLI → gate/conformance infra.

| Status | Page slug | Functional area | Primary code |
|---|---|---|---|
| covered | `subsystems/generator.md` | Generator pipeline — reads the X3D UOM, emits C++ bindings | `src/x3d_cpp_gen/generator.py`, `parser.py`, `emit/`, `backends/`, `model/` |
| covered | `subsystems/templates.md` | Jinja2 emit templates (class header/impl, test) | `src/x3d_cpp_gen/templates/class_template.{hpp,cpp}.jinja`, `test_template.cpp.jinja` |
| covered | `subsystems/generated-bindings.md` | The committed, golden-locked generated C++ node bindings | `generated_cpp_bindings/` |
| covered | `subsystems/scene-graph.md` | Runtime core: scene graph, DEF/USE sharing, document/scene model | `runtime/X3DScene.hpp`, `X3DDocument.hpp`, `X3DRuntime.hpp`, `runtime/scene/` |
| covered | `subsystems/reflection.md` | Reflection layer: field descriptors, reflection thunks, factory/registry | `src/x3d_cpp_gen/emit/reflection.py`, `descriptors.py`, `registry.py`, `factory.py` |
| covered | `subsystems/execution-context.md` | Execution context: per-tick driver, field-write seam, scene bridge | `runtime/events/X3DExecutionContext.hpp`, `X3DSceneBridge.hpp`, `X3DActiveNode.hpp` |
| covered | `subsystems/dirty-bounds-transform.md` | Dirty-tracking + world-transform propagation + bounding volumes | `runtime/scene/DirtyTracker.hpp`, `TransformSystem.hpp`, `BoundsSystem.hpp`, `GeometryBounds.hpp`, `CycleBreaker.hpp` |
| covered | `subsystems/codecs-writers.md` | Codec writers: XML/VRML/JSON serialization + field-value IO | `runtime/codecs/` (`XmlWriter.hpp`, `VrmlWriter.hpp`, `JsonWriter.hpp`, `FieldValueIO.hpp`, `X3DCodecs.hpp`) |
| covered | `subsystems/canonical-xml.md` | Canonical XML writer (X3DC14N) | `runtime/codecs/CanonicalXmlWriter.hpp` |
| covered | `subsystems/parse-readers.md` | Parse frontend: XML/VRML/JSON readers + node builder + version inference | `runtime/parse/` (`XmlReader.hpp`, `Vrml97Reader.hpp`, `ClassicVrmlReader.hpp`, `JsonReader.hpp`, `X3DReader.hpp`, `NodeBuilder.hpp`) |
| covered | `subsystems/proto-expand.md` | PROTO/EXTERNPROTO expansion + IS-connection plumbing | `runtime/X3DProtoExpand.hpp`, `X3DProtoClone.hpp`, `X3DProto.hpp`, `parse/X3DProtoResolver.hpp` |
| covered | `subsystems/inline-expand.md` | Parse-time Inline expansion (OBJ/glTF→X3D ingestion seam) | `runtime/InlineExpand.hpp`, `X3DImportExport.hpp` |
| covered | `subsystems/event-cascade.md` | Event cascade: per-tick event propagation, route-loop dedup, timestamp quantum | `runtime/events/X3DEventCascade.hpp`, `X3DEventGraph.hpp`, `X3DFieldAddress.hpp` |
| covered | `subsystems/routes.md` | ROUTEs: route model, field addressing, route resolution over effective fields | `runtime/X3DRoute.hpp`, `runtime/events/DynamicField.hpp` |
| covered | `subsystems/system-time.md` | TimeSensor system + time-dependent base | `runtime/events/TimeSensorSystem.hpp`, `TimeSensorBehavior.hpp`, `X3DTimeDependentSystem.hpp` |
| covered | `subsystems/system-interpolators.md` | Interpolator system (linear + spline) | `runtime/events/InterpolatorSystem.hpp`, `Interpolation.hpp`, `SplineInterpolatorSystem.hpp`, `SplineInterpolation.hpp`, `InterpolatorRegistration.hpp` |
| covered | `subsystems/system-followers.md` | Followers component (§39): Damper (IIR cascade) + Chaser (re-basing ramp), all 14 node types | `runtime/events/FollowerSystem.hpp`, `FollowerArith.hpp`, `FollowerRegistration.hpp` |
| covered | `subsystems/system-eventutility.md` | EventUtility nodes (BooleanFilter/Sequencer/Toggle/Trigger, IntegerSequencer, …) | `runtime/events/EventUtilitySystem.hpp` |
| covered | `subsystems/system-viewdependent.md` | View-dependent system (LOD/visibility/transform-sensor/Billboard view-coupling) | `runtime/scene/ViewDependentSystem.hpp`, `runtime/scene/Billboard.hpp` |
| covered | `subsystems/system-viewpointbind.md` | Binding stacks: Viewpoint/NavigationInfo/Background/Fog bind | `runtime/events/ViewpointBindSystem.hpp`, `runtime/scene/BindingSystem.hpp`, `BindingStack.hpp`, `runtime/events/ViewpointOffset.hpp` |
| covered | `subsystems/system-navigation.md` | Navigation system: EXAMINE/FLY/LOOKAT/NONE + head pose | `runtime/events/NavigationSystem.hpp`, `HeadPose.hpp` |
| covered | `subsystems/system-pointing.md` | Pointing-device sensors (Touch + drag: Plane/Sphere/Cylinder) | `runtime/events/PointingSensorSystem.hpp`, `PointerState.hpp`, `runtime/events/drag/`, `runtime/scene/PickSystem.hpp` |
| covered | `subsystems/system-keydevice.md` | Key-device sensor system + key state | `runtime/events/KeyDeviceSensorSystem.hpp`, `KeyState.hpp` |
| covered | `subsystems/system-script-sai.md` | Script/SAI runtime: ScriptEngine seam, SAI context, ECMAScript backend | `runtime/script/` (`ScriptSystem.hpp`, `ScriptEngine.hpp`, `SaiContext.hpp`, `EcmaScriptBackend.{hpp,cpp}`) |
| covered | `subsystems/system-asset-io.md` | AssetResolver/IO seam proven generic: libcurl HTTP + AWS S3 backends + CI-gated byte-equal swap-test | `runtime/extract/AssetResolver.hpp`, `runtime/io/curl/HttpResolver.{hpp,cpp}`, `runtime/io/s3/S3Resolver.{hpp,cpp}` |
| covered | `subsystems/system-texture-decode.md` | TextureResolver decode seam proven generic: stb_image + wuffs backends + CI-gated byte-equal swap-test + multi-format sniff-dispatch composer | `runtime/extract/TextureResolver.hpp`, `MultiFormatTextureResolver.hpp`, `runtime/io/stb/StbTextureResolver.{hpp,cpp}`, `runtime/io/wuffs/WuffsTextureResolver.{hpp,cpp}` |
| covered | `subsystems/system-font-metrics.md` | FontMetrics seam proven generic: stb_truetype + FreeType backends + CI-gated exact-equal advanceEm swap-test over Liberation font fixtures (PLAIN style) | `runtime/extract/FontMetrics.hpp`, `runtime/io/stbtt/StbttFontMetrics.{hpp,cpp}`, `runtime/io/freetype/FreetypeFontMetrics.{hpp,cpp}` |
| covered | `subsystems/sensors.md` | Sensor layer overview: time/pointing/key/visibility sensor wiring + active-node model | `runtime/events/X3DActiveNode.hpp`, `X3DSystem.hpp` (sensor cross-cutting view) |
| covered | `subsystems/extract.md` | Extraction pipeline: SceneExtractor → MeshBuilder → PackedMesh + RenderItem | `runtime/extract/SceneExtractor.hpp`, `MeshBuilder.hpp`, `PackedMesh.hpp`, `RenderItem.hpp` |
| covered | `subsystems/extract-textures.md` | Texture/material/light extraction + resolver seam | `runtime/extract/TextureExtract.hpp`, `TextureResolver.hpp`, `TextureDesc.hpp`, `TextureTransform2D.hpp`, `MaterialSystem.hpp`, `LightSystem.hpp`, `AssetResolver.hpp` |
| covered | `subsystems/extract-text.md` | Text extraction + layout via the font-metrics seam | `runtime/extract/TextExtract.hpp`, `TextLayout.hpp`, `FontMetrics.hpp` |
| covered | `subsystems/extract-topology.md` | Topology classification (tris/lines/points) for extracted meshes | `runtime/extract/Topology.hpp` |
| covered | `subsystems/ext-firewall.md` | The ext firewall: binary mesh / STL / ExternalGeometry behind the resolver seam | `runtime/ext/`, `include/x3d/ext.hpp`, `runtime/ext/codecs/StlReader.hpp` |
| covered | `subsystems/math.md` | SF*-native math core (Mat4, Aabb, Ray, Intersect) — no external math lib | `runtime/math/` (`Mat4.hpp`, `Aabb.hpp`, `Ray.hpp`, `Intersect.hpp`) |
| covered | `subsystems/sdk-facade.md` | The SDK façade — the single public header consumers include | `include/x3d/sdk.hpp` |
| covered | `subsystems/cli-suite.md` | CLI suite: `x3d convert/validate/extract/canonicalize/sim` | `tools/x3d-cli/`, `tools/x3d_cli.cpp` |
| covered | `subsystems/conformance-infra.md` | Conformance infrastructure: validator, manifests, prose anchors, fidelity, view generator | `src/x3d_cpp_gen/conformance/`, `docs/conformance/`, `tools/corpus_audit.{hpp,cpp}`, `tools/corpus_sweep.cpp` |
| covered (partial) | `subsystems/physics.md` | §37 RigidBody dynamics via a core seam + flag-gated Jolt backend — Box/Sphere/Cylinder/Cone + gravity + forces/torques/damping + collision response + collision reporting (CollisionSensor contacts/intersections/isActive) + Ball/Hinge/Slider joints; remaining §37 fields/nodes ignored (see the page's §37 coverage matrix + `CONF-RBP*`) | `runtime/physics/` (`PhysicsBackend.hpp`, `PhysicsSystem.hpp`, `ContactReporter.hpp`, `jolt/JoltBackend.{hpp,cpp}`) |
| covered | `subsystems/sound.md` | §16 Sound audio graph via a proven-generic [STABLE] AudioBackend seam — BuiltinDspBackend (always-built) + MiniaudioBackend (flag-gated) + CI-gated headless swap-test (`x3d_sound_swaptest`) | `runtime/sound/` (`AudioBackend.hpp`, `SoundSystem.hpp`, `dsp/BuiltinDspBackend.{hpp,cpp}`, `miniaudio/MiniaudioBackend.{hpp,cpp}`, `RecordingBackend.hpp`, `tests/sound_swap_test.cpp`) |
| covered | `subsystems/materials.md` | §12 MaterialDesc discriminated union (Phong/Physical/Unlit), backMaterial (MAT-006), texCoordMapping multi-UV (MAT-007), ORM channel-packing (MAT-008), gamma/sRGB stance (MAT-009) | `runtime/extract/MaterialSystem.hpp`, `runtime/extract/RenderItem.hpp`, `runtime/extract/X3DFieldValue.hpp` |
| covered | `subsystems/shaders.md` | ComposedShader → ShaderProgramDesc extraction; ShaderUniformVocabulary typed portability surface; buildBindingPlan() three-bucket dispatch; PoC four-program dispatch (unlit/Phong/PBR/author); IBL deferred | `runtime/extract/ShaderUniformVocabulary.hpp`, `runtime/extract/ShaderBindingPlan.hpp`, `runtime/extract/RenderItem.hpp` |

## 2. Decisions (ADRs)

One ADR per binding decision. Numbered sequentially; the slug is a short topic name. Harvested from the project auto-memory (decisions / modernization / build / cli-suite files), the dated specs, and notable commits.

| Status | Page slug | Decision | Grounding source |
|---|---|---|---|
| covered | `decisions/0001-ext-firewall.md` | The ext firewall — binary/STL/ExternalGeometry isolated behind `include/x3d/ext.hpp` + a resolver seam, kept out of the spec-correct core | `2026-06-18-binary-mesh-texture-abstractions.md`, `2026-06-19-binary-geometry-extension-design.md`, memory: ingestion-roadmap |
| covered | `decisions/0002-event-api-for-inputonly.md` | inputOnly fields exposed as an event API (`onX`/`processX`), not a write-only `setX` | memory: decisions |
| covered | `decisions/0003-throw-on-range.md` | Typed setters THROW `std::out_of_range`; the reflection `set` thunk is non-validating (lenient read) | memory: decisions, `2026-06-07-lenient-read-range-warnings-design.md` |
| covered | `decisions/0004-virtual-mixins.md` | AbstractObjectType mixins inherit `public virtual` and keep their real fields | memory: decisions |
| covered | `decisions/0005-golden-files-in-git.md` | Generated headers committed as golden files → codegen must be deterministic | memory: decisions, build |
| covered | `decisions/0006-compiled-static-lib.md` | Generated layer compiled as a static lib `x3d_cpp_nodes` (C1) → ~33× cold-build speedup (two steps: C1@-j4 → ~17×, then -j raise → ~33×); no PCH | memory: build, `2026-06-16-c1-decl-def-split-design.md` |
| covered | `decisions/0007-version-inference-ladder.md` | Version-inference ladder + VRML97→3.0 floor for spec-version resolution (M3 VP-2) | `2026-06-13-m3-versioning-design.md`, memory: modernization |
| covered | `decisions/0008-canonicalize-source-provenance.md` | The `x3d canonicalize` source-provenance / base-URL handling call | `2026-06-20-x3d-canonicalize-design.md` |
| covered | `decisions/0009-sim-snapshot-diff.md` | `x3d sim` field tracer uses snapshot-diff, not a cascade observer (the single observer slot is reserved for dirty-tracking) | `2026-06-20-x3d-sim-design.md` |
| covered | `decisions/0010-differential-gate-regression.md` | The differential CLI gate is wired as a baseline regression gate (cli-gate / canon-gate `--gate`) | `2026-06-20-x3d-cli-convert-validate-design.md`, memory: cli-suite |
| covered | `decisions/0011-ecs-flavored-not-ecs.md` | OOP node model + reflection stay the truth; adopt ECS-*flavored* System abstraction only, not full ECS | memory: decisions, `2026-06-07-architecture-validation-and-resequencing.md` |
| covered | `decisions/0012-no-external-math-lib.md` | No GLM/Eigen in the core; one SF*-native math module; math is the consumer's choice at the seam | memory: decisions (math policy) |
| covered | `decisions/0013-js-engine-choice.md` | ECMAScript engine choice for Script/SAI (quickjs-ng targeted; Duktape backend shipped) | memory: modernization, `2026-06-16-script-sai-runtime-design.md` |
| covered | `decisions/0014-dynamic-field-foundation.md` | Author Script `<field>` decls live as per-instance dynamic fields; cascade/codecs route over `effectiveFields()` | `2026-06-05-dynamic-field-foundation-spec.md`, `2026-06-17-script-cdata-untabling-design.md` |
| covered | `decisions/0015-extraction-pull-per-path.md` | Extraction seam is PULL of an incrementally-maintained dirty set; identity is per-PATH, geometry/material node-keyed | `2026-06-14-m25-extraction-poc-renderer-design.md`, memory: modernization |
| covered | `decisions/0016-cycle-breaker.md` | Containment cycles severed to a DAG once in `buildSceneGraph` (`breakContainmentCycles`) to stop recursive-walker stack overflow | memory: modernization, `runtime/scene/CycleBreaker.hpp` |
| covered | `decisions/0017-inline-expansion-parse-time.md` | OBJ/glTF ingestion via parse-time Inline expansion; binary direction stays behind EXTERNPROTO | `2026-06-19-inline-expansion-design.md`, memory: ingestion-roadmap |
| covered | `decisions/0018-cave-cross-process-delta-contract.md` | CAVE consumer: 1-master / N-wall topology + serializable `{frame#, worldClock, RenderDelta}` wire format + master-broadcasts-clock convention | memory: cave-consumer, `2026-06-18-future-proof-architecture-synthesis.md` §5–6 |
| covered | `decisions/0019-physics-seam.md` | §37 physics as a core engine-agnostic seam + flag-gated Jolt backend (mirrors the script seam); not engine code in the core | `2026-06-20-physics-jolt-seam-design.md`, memory: physics-seam |
| covered | `decisions/0020-sound-seam.md` | §16 audio as a core engine-agnostic AudioBackend seam + dependency-free BuiltinDspBackend (no flag-gate); LabSound is the deferred flag-gated production backend | `2026-06-20-sound-seam-design.md` |
| covered | `decisions/0021-material-shader-design.md` | MaterialDesc discriminated union (Phong/Physical/Unlit); backMaterial as unique_ptr; ShaderUniformVocabulary + buildBindingPlan() introspection; PoC four-program dispatch; IBL/EnvironmentLight deferred (X3D 4.1 node vs. golden invariant) | `2026-06-21-material-shader-design.md` |
| covered | `decisions/0022-scriptengine-second-backend-swap-test.md` | An interface is *proven generic* only when a second independent backend runs identical fixtures to identical observable behavior, gated in CI — established by adding a QuickJS ScriptEngine backend alongside Duktape + the CI-gated `x3d_quickjs_swap` test; the binding pattern for every seam card | `2026-06-22-scriptengine-quickjs-design.md` |
| covered | `decisions/0023-assetresolver-second-backend-swap-test.md` | The genericity-proof pattern applied to the AssetResolver/IO seam: a second backend (AWS S3 SDK) alongside libcurl HTTP + the CI-gated `x3d_assetresolver_swap` byte-equal test; freezes the seam `[STABLE]` and unblocks the LoadSensor / http-EXTERNPROTO / script-fetch findings | `2026-06-23-assetresolver-http-design.md` |
| covered | `decisions/0024-textureresolver-second-backend-swap-test.md` | The genericity-proof pattern applied to the TextureResolver decode seam: a second decoder (wuffs) alongside stb_image + the CI-gated `x3d_texture_tests` byte-equal swap-test over PNG/BMP/GIF/TGA; completes the fetch+decode texture pipeline (thesis-completion + memory-safe decode) | `2026-06-23-textureresolver-decode-design.md` |
| covered | `decisions/0025-fontmetrics-second-backend-swap-test.md` | The genericity-proof pattern applied to the FontMetrics seam: a second backend (FreeType) alongside stb_truetype + the CI-gated `x3d_text_tests` exact-equal advanceEm swap-test over Liberation fixtures (PLAIN style) | `2026-06-24-fontmetrics-seam-genericity-design.md` |
| covered | `decisions/0026-audiobackend-second-backend-swap-test.md` | The genericity-proof pattern applied to the AudioBackend DSP engine seam: a second backend (miniaudio, vendored single-header, flag-gated) alongside BuiltinDspBackend + the CI-gated headless `x3d_sound_swaptest`; synthesis proven numerically (RMS ±2%, Goertzel ±5%) and spatial proven structurally (ear-sign, symmetry, monotonic distance falloff); HRTF/Doppler/ellipsoid deferred (SND-3 partial) | `2026-06-24-audio-seam-genericity-design.md` |
| covered | `decisions/0027-reference-lighting-evaluator.md` | A single reference lighting evaluator owns the spec-normative §17 shading path (per-light ambient/diffuse/specular + the `shadowTest` modulation) and the until-now-unpinned color-space / ambient conventions; the named home of the modulation ADR-0028 references. Proposed | ADR-0028 dependency; reference-consumer `MaterialShader` (PoC + cpu_raster); rendering findings (GitHub Project) |
| covered | `decisions/0028-shadow-visibility-seam.md` | Shadows split at the SDK boundary: the §17 modulation is normative (reference evaluator, ADR-0027) while shadow *generation* is a `ShadowQuery` seam with a first-party CPU ray-cast default proven on structural invariants; `castShadow` surfaced on RenderItem. Proposed | §17 (ISO/IEC 19775-1); ADR-0026 invariant-proof pattern; `castshadow_extract_test` |
| covered | `decisions/0029-directoutput-non-routable.md` | A `directOutput` Script write to another node mutates the target field but does not seed the event cascade; only a Script's own outputOnly writes are routable (SAI 4.5.2) | `docs/conformance/findings.yaml`; SAI §4.5.2 |
| covered | `decisions/0030-fieldofview-validate-not-retype.md` | `OrthoViewpoint.fieldOfView` keeps MFFloat (retyping to SFVec4f breaks ClassicVRML); validated as a fixed SFVec4f-semantics 4-tuple, shared with TextureProjectorParallel | `docs/conformance/findings.yaml` |
| covered | `decisions/0031-extrusion-degenerate-scp.md` | For an underdetermined Extrusion spine (≤2 distinct points), in-plane SCP axes align to the local coordinate system; spines with <2 distinct points render nothing — deterministic degenerate handling | `docs/conformance/findings.yaml` |
| covered | `decisions/0032-hanimdisplacer-routable-coordpoint.md` | A Segment's `coord.point` reflects the weighted Displacer sum and emits `point_changed`; the neutral pose is retained internally and never destructively overwritten (headless runtime) | `docs/conformance/findings.yaml` |
| covered | `decisions/0033-proto-builtin-shadow-error.md` | A ProtoDeclare/ExternProtoDeclare whose name collides with a built-in node type is rejected (quarantined + diagnostic); the built-in keeps precedence. Lenient by default, fatal in strict mode (security) | `docs/conformance/findings.yaml`; ext-firewall |
| covered | `decisions/0034-sensor-switch-lod-activation.md` | An environmental sensor reachable only via a non-selected Switch child or inactive LOD level is treated as removed from the hierarchy (deactivated); Script + time-dependent nodes are not gated | `docs/conformance/findings.yaml` |
| covered | `decisions/0035-viewport-clipboundary-glviewport.md` | `clipBoundary` is the sub-region the layer view is mapped (rescaled) into, aspect derived from the sub-region; exposes a derived `ViewportRegion{x,y,w,h,aspect}` renderer contract (glViewport remap semantics) | `docs/conformance/findings.yaml` |
| covered | `decisions/0036-codec-roundtrip-fidelity-oracles.md` | Names the structural properties a codec round-trip must preserve (DEF/USE/ROUTE survival, authored containerField placement, idempotence) and requires any asserting oracle be proven to fail under injected faults before it is trusted | `2026-06-26 codec round-trip fidelity work` |
| covered | `decisions/0037-graph-walk-traversal-budget.md` | The per-path scene-graph walks (extractor, light collection, pick) carry a `WalkBudget` node-visit cap that bounds an acyclic "doubling DAG" fan-out, surfaced as a graceful `budgetExceeded` signal; `worldOf` instead memoizes (first-found search). Extends ADR-0016 from crash-proof to DoS-bounded | `runtime/scene/tests/walker_budget_test.cpp`; issue #21 |
| covered | `decisions/0038-local-resolver-path-confinement.md` | The default Inline/EXTERNPROTO file resolvers canonicalize a file-like url and confine it to a configurable root (default: the parsed file's own dir, secure; widenable to a trusted content tree for legit `../` refs) — absolute urls and escapes above the root are rejected (SEC-3), and the canonical path doubles as the cycle-guard key so spelling aliases cannot defeat it (SEC-4) | `runtime/parse/tests/path_confine_test.cpp`; SEC-3/SEC-4 |
| covered | `decisions/0039-generated-binding-namespaces.md` | The 685 generated binding types move out of the global namespace into `x3d::core` (value/reflection vocabulary) + `x3d::nodes` (node classes), mirrored by `x3d/core/` + `x3d/nodes/` header subdirs and `#pragma once` (API-1 + API-2); clean break, no shim; consumers qualify asymmetrically (a `using`-directive for the small core vocabulary, explicit `x3d::nodes::` / a per-file alias for the 685 node types) | `runtime/tests/namespace_taxonomy_test.cpp`; API-1/API-2 |
| covered | `decisions/0040-nurbs-tessellation-first-party.md` | NURBS curve + patch-surface tessellation ships first-party (I/O-free `runtime/extract/NurbsEval.hpp` Cox–de Boor math unit wired into two `MeshBuilder` arms) rather than behind a swap-tested seam — NURBS eval needs no outside world and X3D §27 prescribes the result, so a second backend would be tautological; `externalGeometryResolver` stays the unrecognized-geometry fallback (now serving the deferred trimmed/swept/swung nodes); control-point convex-hull AABB bounds; clamped-uniform default knots; full periodic `closed`/`uClosed`/`vClosed` handling. Closes NRB-1 for curve + patch (NRB-3 tracks deferred surfaces) | `runtime/extract/tests/nurbs_eval_test.cpp`, `runtime/extract/tests/mesh_builder_nurbs_test.cpp`; NRB-1 |

## 3. Guides

Operational how-tos.

| Status | Page slug | Guide |
|---|---|---|
| covered | `guides/build-and-mise.md` | Build & mise tasks — the `dev` preset, `mise run build`, the Ninja job pool, ccache |
| covered | `guides/gate-system.md` | The gate system — golden / conformance / cli-gate-regression / canon-gate regression gates |
| covered | `guides/rags.md` | The RAGs — running `code_rag`/`spec_rag`, when to use which (companion to the knowledge map) |
| covered | `guides/conformance-campaign.md` | The conformance campaign — cadence, the audit workflow, the generated conformance view |
| covered | `guides/add-a-cli-command.md` | How to add a CLI command to the `x3d` suite |
| covered | `guides/workflow-subagent-discipline.md` | The workflow / subagent discipline — multi-agent fan-out, review gates |
| covered | `guides/card-to-done-workflow.md` | Card → Done — Definition of Ready/Done, the card→issue→branch→PR→docs chain, `pick-card.sh` |
| covered | `guides/golden-regeneration.md` | Golden regeneration — when and how to regenerate golden files and accept drift |

## 4. Trackers

Top-level live trackers (a tracker is a page that tracks an ongoing, cross-cutting state — like this coverage manifest itself).

| Status | Page slug | Tracker |
|---|---|---|
| covered | `seam-status.md` | Seam-Status Matrix — the live tracker of the product genericity thesis: one row per seam (ScriptEngine, AssetResolver/IO, Physics, Audio, FontMetrics, TextureResolver, GeoProjection, Consumer); GREEN = interface frozen + ≥2 backends + a CI-gated swap-test. Formalizes the pattern from [ADR-0022](decisions/0022-scriptengine-second-backend-swap-test.md). |

---

## Coverage summary

| Category | Total | Covered | Planned |
|---|---|---|---|
| Subsystems | 41 | 41 | 0 |
| Decisions (ADRs) | 39 | 39 | 0 |
| Guides | 8 | 8 | 0 |
| Trackers | 1 | 1 | 0 |
| **Total** | **87** | **87** | **0** |

> ADR coverage (`decisions/*.md` ⇄ this table) is enforced by `mise run coverage-gate`
> (`scripts/coverage_gate.py`), which fails CI on any ADR file missing a row or any
> row pointing at a missing file. Keep the two in lock-step.

Wave 1 delivered 3 `covered` rows (the vertical slice). Wave 2 filled all subsystem + decision rows (ADR-0001..0017). Wave 3 filled all guide rows and ran the completeness critic, which independently re-derived the target set and found one genuine gap: ADR-0018 (CAVE cross-process delta contract), now added and filled. The wiki-physics wave added ADR-0019 + the Physics subsystem page. The wiki-sound wave added ADR-0020 + the Sound subsystem page. The followers-runtime wave (§39) added the Followers subsystem page. The material-shader wave (2026-06-21) added ADR-0021 + the Materials and Shaders subsystem pages (+3 rows). The scriptengine-quickjs wave (2026-06-22..23) added ADR-0022 (second-backend swap-test = genericity proof) + the new top-level **Seam-Status Matrix** tracker (`seam-status.md`). The seam-genericity waves (2026-06-23) added ADR-0023 + the **Asset Resolver / IO Seam** subsystem page (libcurl + S3) and ADR-0024 + the **Texture Decode Seam** subsystem page (stb_image + wuffs), bringing both seams GREEN. The 2026-06-24 seam waves added ADR-0025 + the **Font Metrics Seam** subsystem page (stb_truetype + FreeType), and ADR-0026 (AudioBackend second backend + headless swap-test), bringing both FontMetrics and Audio seams GREEN. The 2026-06-24..26 rendering + conformance-campaign waves added ADR-0027 (reference lighting evaluator, Proposed) and ADR-0028 (shadow visibility seam, Proposed) plus the per-finding conformance decisions ADR-0029..0036 (directOutput non-routable, fieldOfView validation, degenerate Extrusion SCP, HAnimDisplacer routable coord.point, PROTO built-in shadow rejection, sensor activation under Switch/LOD, Viewport.clipBoundary glViewport semantics, and codec round-trip fidelity oracles). The 2026-06-26 graph-walk hardening follow-up added ADR-0037 (traversal budget bounding the acyclic doubling-DAG fan-out, extending ADR-0016 from crash-proof to DoS-bounded). The 2026-06-27 AssetResolver/IO input-hardening work added ADR-0038 (the default Inline/EXTERNPROTO file resolvers confine includes to the source file's directory subtree and key the cycle guard on the canonical path — SEC-3/SEC-4). The 2026-06-27 generated-binding namespace refactor added ADR-0039 (the 685 generated types move out of the global namespace into `x3d::core` + `x3d::nodes`, mirrored by `x3d/core/` + `x3d/nodes/` header subdirs — API-1 + API-2 — with asymmetric consumer qualification and a clean break, no shim). The 2026-06-27 NURBS curve+patch wave added ADR-0040 (NURBS curve + patch-surface tessellation is first-party I/O-free math — `runtime/extract/NurbsEval.hpp` wired into two `MeshBuilder` arms — not a swap-tested seam, since X3D §27 prescribes the result; closes NRB-1 for curve + patch). No rows remain `planned`.

---

## Residuals / out-of-scope items

The Wave-3 completeness critic identified one gap that was confirmed as genuine and covered in this wave:

**ADR-0018: CAVE cross-process RenderDelta wire contract** (`decisions/0018-cave-cross-process-delta-contract.md`)
— Filled in Wave 3. The 1-master / N-wall topology + `{frame#, worldClock, RenderDelta}` wire-format binding + master-broadcasts-clock convention is now documented. This was the only true coverage gap identified by the critic; all other candidate gaps were already covered by existing pages.

No items were deferred or silently dropped. Any future gaps (e.g., a `RenderDeltaCodec` subsystem once the CAVE integration layer is built, or a `subsystems/cave-integration.md` page once the wall-consumer process is implemented) should be added as new rows in this manifest at the start of the wave that fills them.

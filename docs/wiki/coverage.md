---
title: Subsystem source map
summary: Wiki subsystem pages and their primary source paths.
tags: [subsystems, reference]
updated: 2026-11-05
---

# Subsystem source map

Use this table to find the primary source paths behind subsystem pages. The wiki
navigation lists all pages, including decisions and guides; this page carries no
coverage status or synchronization gate.

| Page slug | Functional area | Primary code |
|---|---|---|
| `subsystems/generator.md` | Generator pipeline — reads the X3D UOM, emits C++ bindings | `src/x3d_cpp_gen/generator.py`, `parser.py`, `emit/`, `backends/`, `model/` |
| `subsystems/templates.md` | Jinja2 emit templates (class header/impl, test) | `src/x3d_cpp_gen/templates/class_template.{hpp,cpp}.jinja`, `test_template.cpp.jinja` |
| `subsystems/generated-bindings.md` | The committed, golden-locked generated C++ node bindings | `generated_cpp_bindings/` |
| `subsystems/scene-graph.md` | Runtime core: scene graph, DEF/USE sharing, document/scene model | `runtime/X3DScene.hpp`, `X3DDocument.hpp`, `X3DRuntime.hpp`, `runtime/scene/` |
| `subsystems/hanim.md` | H-Anim skin binding, pose, reference CPU skinner, Joint and Segment displacers | `runtime/hanim/` |
| `subsystems/reflection.md` | Reflection layer: field descriptors, reflection thunks, factory/registry | `src/x3d_cpp_gen/emit/reflection.py`, `descriptors.py`, `registry.py`, `factory.py` |
| `subsystems/execution-context.md` | Execution context: per-tick driver, field-write seam, scene bridge | `runtime/events/X3DExecutionContext.hpp`, `X3DSceneBridge.hpp` |
| `subsystems/hanim.md` | H-Anim discrete motion playback and joint event writes | `runtime/hanim/HAnimMotionSystem.hpp`, `runtime/events/tests/hanim_motion_test.cpp` |
| `subsystems/dirty-bounds-transform.md` | Dirty-tracking + world-transform propagation + bounding volumes | `runtime/scene/DirtyTracker.hpp`, `TransformSystem.hpp`, `BoundsSystem.hpp`, `GeometryBounds.hpp`, `CycleBreaker.hpp` |
| `subsystems/codecs-writers.md` | Codec writers: XML/VRML/JSON serialization + field-value IO | `runtime/codecs/` (`XmlWriter.hpp`, `VrmlWriter.hpp`, `JsonWriter.hpp`, `FieldValueIO.hpp`, `X3DCodecs.hpp`) |
| `subsystems/canonical-xml.md` | Canonical XML writer (X3DC14N) | `runtime/codecs/CanonicalXmlWriter.hpp` |
| `subsystems/parse-readers.md` | Parse frontend: XML/VRML/JSON readers + node builder + version inference + authored scalar presence | `runtime/parse/` (`XmlReader.hpp`, `Vrml97Reader.hpp`, `ClassicVrmlReader.hpp`, `JsonReader.hpp`, `X3DReader.hpp`, `NodeBuilder.hpp`), `runtime/X3DAuthoredScalarFields.hpp` |
| `subsystems/proto-expand.md` | PROTO/EXTERNPROTO expansion + IS-connection plumbing | `runtime/X3DProtoExpand.hpp`, `X3DProtoClone.hpp`, `X3DProto.hpp`, `parse/X3DProtoResolver.hpp` |
| `subsystems/inline-expand.md` | Parse-time Inline expansion (OBJ/glTF→X3D ingestion seam) | `runtime/InlineExpand.hpp`, `X3DImportExport.hpp` |
| `subsystems/event-cascade.md` | Event cascade: per-tick event propagation, route-loop dedup, timestamp quantum | `runtime/events/X3DEventCascade.hpp`, `X3DEventGraph.hpp`, `X3DFieldAddress.hpp` |
| `subsystems/routes.md` | ROUTEs: route model, field addressing, route resolution over effective fields | `runtime/X3DRoute.hpp`, `runtime/events/DynamicField.hpp` |
| `subsystems/system-time.md` | TimeSensor system + time-dependent base | `runtime/events/TimeSensorSystem.hpp`, `X3DTimeDependentSystem.hpp` |
| `subsystems/system-interpolators.md` | Interpolator system (linear + spline) | `runtime/events/InterpolatorSystem.hpp`, `Interpolation.hpp`, `SplineInterpolatorSystem.hpp`, `SplineInterpolation.hpp`, `InterpolatorRegistration.hpp` |
| `subsystems/system-followers.md` | Followers component (§39): Damper (IIR cascade) + Chaser (re-basing ramp), all 14 node types | `runtime/events/FollowerSystem.hpp`, `FollowerArith.hpp`, `FollowerRegistration.hpp` |
| `subsystems/geospatial.md` | §25 Geospatial coordinate conversion: the GeoProjection seam, frames and node glue | `runtime/math/Geo*.hpp`, `runtime/scene/GeoNodes.hpp`, `runtime/math/tests/geo_projection_test.cpp` |
| `subsystems/system-eventutility.md` | EventUtility nodes (BooleanFilter/Sequencer/Toggle/Trigger, IntegerSequencer, …) | `runtime/events/EventUtilitySystem.hpp` |
| `subsystems/system-viewdependent.md` | View-dependent system (LOD/visibility/transform-sensor/Billboard view-coupling) | `runtime/scene/ViewDependentSystem.hpp`, `runtime/scene/Billboard.hpp` |
| `subsystems/system-viewpointbind.md` | Binding stacks: Viewpoint/NavigationInfo/Background/Fog bind | `runtime/events/ViewpointBindSystem.hpp`, `runtime/scene/BindingSystem.hpp`, `BindingStack.hpp`, `runtime/events/ViewpointOffset.hpp` |
| `subsystems/system-navigation.md` | Navigation system: EXAMINE/FLY/LOOKAT/NONE + head pose | `runtime/events/NavigationSystem.hpp`, `HeadPose.hpp` |
| `subsystems/system-pointing.md` | Pointing-device sensors (Touch + drag: Plane/Sphere/Cylinder) | `runtime/events/PointingSensorSystem.hpp`, `PointerState.hpp`, `runtime/events/drag/`, `runtime/scene/PickSystem.hpp` |
| `subsystems/system-keydevice.md` | Key-device sensor system + key state | `runtime/events/KeyDeviceSensorSystem.hpp`, `KeyState.hpp` |
| `subsystems/system-loadsensor.md` | LoadSensor as a live X3DNetworkSensorNode: a time-driven System over the AssetResolver seam observing child URL-object load state per tick (closes NSN-1..7, NSN-9) | `runtime/events/LoadSensorSystem.hpp`, `runtime/io/file/FileResolver.hpp` |
| `subsystems/system-script-sai.md` | Script/SAI runtime: ScriptEngine seam, SAI context, ECMAScript backend | `runtime/script/` (`ScriptSystem.hpp`, `ScriptEngine.hpp`, `SaiContext.hpp`, `EcmaScriptBackend.{hpp,cpp}`) |
| `subsystems/sai-provider.md` | Portable/native provider composition, ownership, host lifecycle and source/installed consumer checks | `adapters/sai_provider/`, `tests/cmake/sai_provider/`, `tests/cmake/sai_coexistence/`, `scripts/verify_sai_provider.sh`, `scripts/verify_sai_coexistence.sh` |
| `subsystems/system-asset-io.md` | AssetResolver callback with file/HTTP/FTP backends and HTTP contract tests | `runtime/extract/AssetResolver.hpp`, `runtime/io/` |
| `subsystems/system-texture-decode.md` | TextureResolver decode seam proven generic: stb_image + wuffs backends + CI-gated byte-equal swap-test + multi-format sniff-dispatch composer | `runtime/extract/TextureResolver.hpp`, `MultiFormatTextureResolver.hpp`, `runtime/io/stb/StbTextureResolver.{hpp,cpp}`, `runtime/io/wuffs/WuffsTextureResolver.{hpp,cpp}` |
| `subsystems/system-font-metrics.md` | FontMetrics callback with stb metrics, glyph atlas, and contract tests | `runtime/extract/FontMetrics.hpp`, `runtime/io/stbtt/`, `runtime/io/tests/font_metrics_tests.cpp` |
| `subsystems/sensors.md` | Sensor layer overview: time/pointing/key/visibility sensor wiring + System model | `runtime/events/X3DSystem.hpp` (sensor cross-cutting view) |
| `subsystems/extract.md` | Extraction pipeline: SceneExtractor → MeshBuilder → PackedMesh + RenderItem | `runtime/extract/SceneExtractor.hpp`, `MeshBuilder.hpp`, `PackedMesh.hpp`, `RenderItem.hpp` |
| `subsystems/hanim.md` | §26 skin extraction, pose-only deltas, Segment displacement at mesh build, and the CPU `deformedMesh` API | `runtime/extract/SceneExtractor.hpp`, `MeshBuilder.cpp`, `runtime/hanim/HAnimSkin.hpp` |
| `subsystems/extract-textures.md` | Texture/material/light extraction + resolver seam | `runtime/extract/TextureExtract.hpp`, `TextureResolver.hpp`, `TextureTransform2D.hpp`, `MaterialSystem.hpp`, `LightSystem.hpp`, `AssetResolver.hpp` |
| `subsystems/extract-text.md` | Text extraction + layout via the font-metrics seam | `runtime/extract/TextExtract.hpp`, `TextLayout.hpp`, `FontMetrics.hpp` |
| `subsystems/extract-topology.md` | Topology classification (tris/lines/points) for extracted meshes | `runtime/extract/Topology.hpp` |
| `subsystems/ext-firewall.md` | The ext firewall: binary mesh / STL / ExternalGeometry behind the resolver seam | `runtime/ext/`, `include/x3d/ext.hpp`, `runtime/ext/codecs/StlReader.hpp` |
| `subsystems/math.md` | SF*-native math core (Mat4, Aabb, Ray, Intersect) — no external math lib | `runtime/math/` (`Mat4.hpp`, `Aabb.hpp`, `Ray.hpp`, `Intersect.hpp`) |
| `subsystems/sdk-facade.md` | The SDK façade — the single public header consumers include | `include/x3d/sdk.hpp` |
| `subsystems/cli-suite.md` | CLI suite: `x3d convert/validate/extract/canonicalize/sim` | `tools/x3d-cli/`, `tools/x3d_cli.cpp` |
| `subsystems/conformance-infra.md` | Conformance infrastructure: validator, manifests, prose anchors, fidelity, view generator | `src/x3d_cpp_gen/conformance/`, `docs/conformance/`, `tools/corpus_audit.{hpp,cpp}`, `tools/corpus_sweep.cpp` |
| `subsystems/physics.md` | §37 RigidBody dynamics via a core seam + flag-gated Jolt backend — Box/Sphere/Cylinder/Cone + gravity + forces/torques/damping + collision response + collision reporting (CollisionSensor contacts/intersections/isActive) + Ball/Hinge/Slider joints; remaining §37 fields/nodes ignored (see the page's §37 coverage matrix + `CONF-RBP*`) | `runtime/physics/` (`PhysicsBackend.hpp`, `PhysicsSystem.hpp`, `ContactReporter.hpp`, `jolt/JoltBackend.{hpp,cpp}`) |
| `subsystems/sound.md` | Sound graph through AudioBackend, SoundSystem, and BuiltinDspBackend | `runtime/sound/` |
| `subsystems/materials.md` | §12 MaterialDesc discriminated union (Phong/Physical/Unlit), backMaterial (MAT-006), texCoordMapping multi-UV (MAT-007), ORM channel-packing (MAT-008), gamma/sRGB stance (MAT-009) | `runtime/extract/MaterialSystem.hpp`, `runtime/extract/RenderItem.hpp`, `runtime/extract/X3DFieldValue.hpp` |
| `subsystems/shaders.md` | §31 shader selection (selectShader) → RenderItem::shaderProgram; ShaderSystem isSelected/isValid/activate; ShaderUniformVocabulary typed portability surface; buildBindingPlan() three-bucket dispatch; PoC four-program dispatch (unlit/Phong/PBR/author); IBL deferred | `runtime/extract/ShaderExtract.hpp`, `runtime/events/ShaderSystem.hpp`, `runtime/extract/ShaderUniformVocabulary.hpp`, `runtime/extract/ShaderBindingPlan.hpp`, `runtime/extract/RenderItem.hpp` |
| `subsystems/asset-import.md` | Asset-import backend seam, texture pipeline, and the footprint-gated authoring target | `examples/asset_import/`, `include/x3d/authoring.hpp`, `scripts/authoring-footprint.sh` |

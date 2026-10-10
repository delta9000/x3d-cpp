---
title: X3D 4.0 Requirements Audit
summary: Published Full-profile requirements, implementation evidence, missing behavior, and acceptance work at a8aefc5.
tags: [conformance, audit, requirements]
updated: 2026-10-02
---

# X3D 4.0 requirements audit

**Full-profile conformance is not established.** This audit covers the requirement
families in all 36 components, common execution rules, profile minima, and the
boundaries with encodings and SAI. It records 13 additional implementation gaps.
The previous 23 open/deferred findings were an incomplete backlog.

Baseline: `a8aefc5`, 2026-09-27. Audit: 2026-09-28. The implementation evidence
below is source inspection, with the UNIT defect additionally reproduced through
the rebuilt CLI as recorded below. Existing tests are cited as evidence to inspect;
their presence is not a claim that they cover an entire clause or passed in this
audit. Verification of the audit artifacts is recorded at the end.

This is a requirements and coverage audit, not a certification or an exhaustive
execution of every field permutation. The inventory contains **260 concrete
nodes and 4,191 field entries**, including inherited fields repeated on concrete
nodes. Each entry still needs semantic evidence before sign-off. The generated
component report at the baseline listed 262 entries because it also included
`X3DSingleTextureTransformNode` and `X3DStatement`; neither is a concrete node in
the vendored UOM. The follow-up correction restricts the report to the UOM's
concrete-node list. Older documentation citing 330 concrete nodes must not be
used as the audit denominator.

- [Node inventory](x3d4-requirements-nodes.csv): every concrete node in the
  vendored 4.0 UOM, its normative chapter, field-entry count, and associated open
  findings. A row without a finding is explicitly **unverified**, not compliant.
- [Source manifest](x3d4-requirements-sources.json): official URLs, retrieval date,
  and SHA-256 hashes of the retrieved HTML. Downloaded source bodies are temporary
  research material and are not vendored into this repository.
- Behavioral findings remain in `docs/conformance/findings.yaml`; the `REQ-*`
  rows contain code evidence and acceptance cases. This document groups that work
  and records obligations that are not yet proven bugs.

## Target and claim boundary

Use the published **ISO/IEC 19775-1:2023, X3D 4.0** text and its
[Full profile](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fullProfile.html).
X3D 4.1 additions are outside this target. Maintain separate declarations for
the versions and encodings actually supported.

[Clause 6](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/conformance.html)
distinguishes valid files, generators, and browsers. Browser conformance includes
presentation, interaction, events, and minimum capacities. A headless SDK can
supply much of this behavior, but a Full browser claim must name a complete host
configuration: renderer, audio, script engines, resource loaders, fonts, and
enabled optional components. Passing a descriptor to an unimplemented consumer
does not establish the presentation requirement.

Audit three deliverables separately:

| Deliverable | Acceptance boundary |
|---|---|
| Authoring and codecs | Generated files conform to their declared version/profile and encoding; valid input in the claimed scope can be read. Cross-encoding loss is assessed against what the destination grammar represents. |
| Domain runtime and extraction | Fields, event semantics, lifecycles, graph changes, geometry and resource descriptors implement the requirements assigned to the SDK. |
| Full host configuration | Required resources and languages are available and graphics, audio and interaction work end to end. Optional backends must actually be enabled and wired in the tested configuration. |

SAI (19775-2), language bindings (19777), and encoding grammars (19776) need their
own versioned requirements lists. Part 1 coverage does not certify those parts.

## Reliability of the existing coverage records

At the baseline, `scripts/conformance_view.py::classify_behaves` inferred effective
wiring from any closed finding and labeled a wired node with no open finding
`conformant`. The follow-up removes both inferences: those nodes remain
`unverified`. An explicit open finding can establish inert behavior; static
wiring detection alone cannot prove either conformance or inertness.
Non-behavioral nodes can receive `n/a` even when rendering behavior is absent.
Schema validation, generated-source parity and corpus parsing address different
properties from runtime conformance.

The following reference-data errors were corrected after the audit. The CLI
profile-table generator now uses the same reviewed component levels for these
three profiles; it previously inferred levels from UOM node-list prose.

| Reference data | Baseline value | Corrected value from published profile table |
|---|---|---|
| Interchange / Networking | 2 | 1, Annex B table B.2 |
| Interactive / EnvironmentalSensor | 2 | 1, Annex C table C.2 |
| Interactive / Navigation | 2 | 1, table C.2 |
| Interactive / EnvironmentalEffects | 2 | 1, table C.2 |
| Immersive / Lighting | 3 | 2, Annex D table D.2 |
| Immersive / EnvironmentalEffects | 3 | 2, table D.2 |
| Immersive / CubeMapTexturing | 1 | Not listed in table D.2; check prerequisites before assigning membership |

Sources: [Interchange](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/interchange.html),
[Interactive](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/interactive.html),
[Immersive](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/immersive.html).
The `Full: {"*": 9}` entry is a membership shortcut, not evidence of support at
every required level. The report's `Levels` column lists levels found on node
bindings; it is not a list of implemented component capabilities.

There are also source reconciliation issues. The Full appendix includes levels
such as Time 2 and Sound 3 that must be checked against each component's own
support table. It omits some newer node names that appear in component chapters
and the UOM. Resolve conflicts with the published prose and documented errata;
do not silently use UOM membership as a replacement for the component chapter.
The appendix links TextureProjection with the wrong URL capitalization; the UOM
uses another missing filename. The working published chapter is
[textureProjection.html](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/textureProjection.html).
The Sound chapter's OscillatorSource heading contains a grammar declaration
named `Oscillator`; the UOM uses `OscillatorSource`. A grammar-declaration scan
also does not find `ProtoInstance`, whose requirements belong to the prototype
machinery. These are source-mapping exceptions, not missing runtime nodes.

## Component requirements and evidence

`Partial` means a specific missing requirement is identified. `Unverified` means
there is implementation and some evidence, but the component has not received a
complete semantic sign-off. No row below is certified complete. Levels are the
Full appendix's requested levels, with the source reconciliation caveat above.
Paths in the evidence column are repository-relative; test files are candidate
evidence, not whole-component proofs.

| Component / Full level | Required work or remaining verification | Implementation and test evidence | Assessment |
|---|---|---|---|
| [Core](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html) / 2 | Apply root UNIT conversions; resolve external-unit wording; test required profile/component rejection in the host. | `runtime/X3DHeader.hpp`, `runtime/extract/RuntimeSession.hpp`, `runtime/parse/tests/core_diagnostics_test.cpp`; REQ-UNIT | Partial |
| [Time](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/time.html) / 2 | Sign off activation, pause/resume, loop boundaries, timestamps, and large time jumps for every time-dependent consumer. | `runtime/events/X3DTimeDependentSystem.hpp`, `runtime/events/tests/timesensor_rtc_test.cpp`, `media_time_test.cpp` | Unverified |
| [Networking](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html) / 4 | Complete host protocol coverage; verify URL fallback, base URLs, Inline replacement, IMPORT/EXPORT and Anchor behavior together. | `runtime/events/InlineRuntimeSystem.hpp`, `AnchorSystem.hpp`, `runtime/io/curl/HttpResolver.cpp`; REQ-FTP | Partial |
| [Grouping](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/grouping.html) / 3 | Sign off visible/bboxDisplay, StaticGroup, child mutation, DEF/USE paths and event delivery when not rendered. | `runtime/scene/TransformSystem.hpp`, `runtime/extract/SceneExtractor.hpp`, `scene_extractor_t8_test.cpp` | Unverified |
| [Rendering](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html) / 5 | Add scoped ClipPlane extraction and presentation; retain mesh/color/normal/attribute rules and prove required capacities. | `runtime/extract/MeshBuilder.cpp`, `RenderItem.hpp`, `ShaderUniformVocabulary.hpp`; REQ-CLIP | Partial |
| [Shape](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html) / 4 | FillProperties now reaches both reference consumers with pixel tests. Verify remaining material channels, alpha rules and back-material combinations; the OpenGL PoC still ignores the separate back material. | `runtime/extract/MaterialSystem.hpp`, `runtime/extract/tests/material_system_test.cpp`, `scripts/check_poc_fill_properties.py`; REQ-FILL fixed | Partial |
| [Geometry3D](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html) / 4 | Sign off indexing, concavity, normals, UVs, bounds and runtime field changes, including degenerate inputs where behavior is defined. | `runtime/extract/MeshBuilder.cpp`, `mesh_builder_ext001_test.cpp`, `mesh_builder_extrusion_scp_test.cpp` | Unverified |
| [Geometry2D](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html) / 2 | Verify every closure/solid/angle option and live mutation for all eight geometries. | `runtime/extract/MeshBuilder.cpp`, `runtime/extract/tests/mesh_builder_geom2d_test.cpp` | Unverified |
| [Text](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/text.html) / 1 | Prove required character repertoire and actual glyph presentation; review language/direction/style rules and minima. | `TextExtract.hpp`, `TextLayout.hpp`, `runtime/io/stbtt/StbttGlyphAtlas.cpp`, `examples/cpu_raster/tests/text_render_test.cpp` | Unverified |
| [Sound](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html) / 3 | Finish sources/processors, channel routing, HRTF/Doppler, tails, custom waveforms and backend bypass; check destination fields omitted by broad findings. | `runtime/sound/SoundSystem.hpp`, `AudioBackend.hpp`, `runtime/sound/tests/sound_immersive_test.cpp`; SND-1/3/4/5/6/8/9 | Partial |
| [Lighting](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/lighting.html) / 3 | Consume shadow controls; verify light scope, attenuation, material interaction and the required simultaneous-light capacity. | `runtime/extract/LightSystem.hpp`, `examples/cpu_raster/cpuraster/MaterialShader.hpp`; REQ-SHADOW | Partial |
| [Texturing](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html) / 4 | Execute MultiTexture combiners; verify MovieTexture presentation, sampler requirements and all coordinate-generation modes in the selected host. | `MaterialSystem.hpp`, `TextureExtract.hpp`, PoC `MovieState`, `runtime/io/plmpeg/tests/movie_decoder_tests.cpp`; REQ-MULTITEXTURE | Partial |
| [Interpolation](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html) / 5 | Sign off all interpolation families, endpoint/duplicate-key rules, output sizes and numeric tolerances. | `runtime/events/InterpolatorRegistration.hpp`, `runtime/events/tests/interpolator_conformance_test.cpp` | Unverified |
| [PointingDeviceSensor](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html) / 1 | Verify shared/instanced sensors, scope, dragging, deactivation and transformed hits in an interactive host. | `runtime/events/PointingSensorSystem.hpp`, `runtime/events/tests/pointing_sensor_test.cpp`, `drag_math_test.cpp`, `runtime/extract/tests/interactive_profile_test.cpp`; IACC-1 fixed | Unverified |
| [KeyDeviceSensor](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/keyDeviceSensor.html) / 2 | Verify host key mapping, focus, modifiers, Unicode input and StringSensor deletion/termination. | `runtime/events/KeyDeviceSensorSystem.hpp`, `runtime/events/tests/key_device_sensor_test.cpp` | Unverified |
| [EnvironmentalSensor](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalSensor.html) / 3 | Sign off instanced activation regions, exit outputs and transformed target/sensor combinations; apply permitted visibility latitude. | `runtime/scene/ViewDependentSystem.hpp`, `runtime/scene/tests/view_dependent_test.cpp` | Unverified |
| [Navigation](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html) / 3 | Verify avatar-volume clearance and all specified mode/transition combinations. Existing collision probes omit sideways clearance; this needs a normative behavioral oracle. | `runtime/events/NavigationSystem.hpp::resolveMove`, `runtime/events/tests/collision_test.cpp`, `navigation_test.cpp` | Unverified |
| [EnvironmentalEffects](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html) / 4 | Implement local fog and authored vertex fog depths; verify all supported background texture types end to end. | `SceneExtractor::fog`, `scene_extractor_fog_test.cpp`, `background_desc_test.cpp`; REQ-LOCALFOG | Partial |
| [Geospatial](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html) / 2 | Verify all coordinate conventions, URL tile transitions and geographic events; supply a geoid model when claiming geoid-relative heights. | `runtime/math/GeoFrame.hpp`, `GeoBuiltinProjection.hpp`, `runtime/math/tests/geo_projection_test.cpp`, optional PROJ tests | Unverified |
| [HAnim](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html) / 3 | Reconcile referenced H-Anim edition and motion draft; verify deformed bounds/picking and shared Segment geometry. Wire deformation in the selected renderer. IK enforcement is not required. | `runtime/hanim/HAnimSkinImpl.hpp`, `HAnimMotionSystem.hpp`, `runtime/extract/tests/scene_extractor_hanim_test.cpp` | Unverified |
| [NURBS](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html) / 4 | Implement the trimmed surface and authored NURBS texture coordinates (swept/swung surfaces shipped, NRB-3); verify order/control-point/contour minima and weight convention. | `runtime/extract/NurbsEval.hpp`, `MeshBuilder.cpp`, `runtime/events/NurbsInterpolatorSystem.hpp`; NRB-3/NRB-4 | Partial |
| [DIS](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/dis.html) / 2 | Implement transport, PDU encode/decode, entity mapping and network-driven events in the host. | `runtime/scene/ViewDependentSystem.hpp` has local PDU state only; NSN-10 | Partial |
| [Scripting](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/scripting.html) / 1 | Resolve Full-profile Java support; audit ECMAScript binding and every SAI service independently of Script lifecycle tests. | `runtime/script/ScriptSystem.hpp`, `EcmaScriptBackend.cpp`, `QuickJsBackend.cpp`; REQ-JAVA | Partial |
| [EventUtilities](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html) / 1 | Sign off filters, sequencers and triggers across boundary values, timestamps and ROUTE feedback. | `runtime/events/EventUtilitySystem.hpp`, `runtime/events/tests/event_utility_test.cpp`, `event_utility_output_admission_test.cpp`, `runtime/extract/tests/interactive_profile_test.cpp`; IACC-2/3 fixed | Unverified |
| [Shaders](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html) / 1 | Bind texture and array uniforms; run geometry/tessellation stages in a host that links them. | `runtime/extract/ShaderExtract.hpp`, `runtime/events/ShaderSystem.hpp`, `runtime/extract/tests/shader_selection_test.cpp`, cpu_raster `author_shader_test`, PoC `author_shader_gl_test.py`; REQ-SHADER fixed, REQ-SHADER-2 | Partial |
| [CADGeometry](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/CADGeometry.html) / 2 | Verify CAD hierarchy, face placement, visibility, quad geometry and live edits; generic traversal alone is insufficient evidence. | `SceneExtractor.hpp`, `TransformSystem.hpp`, `runtime/extract/tests/scene_extractor_audit_test.cpp` | Unverified |
| [Texturing3D](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texture3D.html) / 2 | Supply 3D image/pixel/composed textures, coordinates, transforms, sampling and updates. | `MaterialSystem::refOf` has no 3D path; T3D-1 | Partial |
| [CubeMapTexturing](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalTexturing.html) / 3 | Implement image/generated cubes and consume cube descriptors as environment maps. | `MaterialSystem::refOf`, `runtime/extract/tests/background_desc_test.cpp`; REQ-CUBE | Partial |
| [Layering](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layering.html) / 1 | Independent binding/navigation, active layer, viewport clipping, composition and repeated layer orders. | `runtime/scene/BindingSystem.hpp`, `SceneExtractor.hpp`; REQ-LAYER, LAY-3, VIEWPORT-CLIP | Partial |
| [Layout](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layout.html) / 2 | Implement layout rectangles, screen transforms, pixel units and screen text. Pixel-specific exceptions do not apply to our raster examples. | `TextExtract::readFontStyleParams`; REQ-LAYOUT | Partial |
| [RigidBodyPhysics](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html) / 2 | Mesh/compound collidables, offsets, missing joints/motors/readback, mass models and remaining solver/sleep parameters. Review contact/friction refinements too. | `runtime/physics/PhysicsSystem.hpp`, `runtime/physics/jolt/JoltBackend.cpp`, `runtime/physics/tests/`; seven CONF-RBP findings | Partial |
| [Picking](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html) / 3 | Implement sensor targets, geometry, match/intersection/sort modes and output events; pointer picking is separate. | `runtime/scene/PickSystem.hpp` is the pointer engine; CONF-PICKSENSOR | Partial |
| [Followers](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html) / 1 | Sign off all field types, initialization, order/tolerance/tau/duration, destinations and activation boundaries. | `runtime/events/FollowerSystem.hpp`, `runtime/events/tests/follower_conformance_test.cpp` | Unverified |
| [ParticleSystems](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html) / 3 | Implement lifecycle, emitters, forces, geometry/color/texture ramps and presentation. | Generated nodes only; no particle runtime; PRT-1 | Partial |
| [VolumeRendering](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html) / 4 | Implement voxel resources, volume nodes, required style combinations and presentation. | No volume extraction/presentation path; VOL-1 | Partial |
| [TextureProjection](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/textureProjection.html) / 2 | Implement perspective/parallel projectors, scope and associated output updates. | `runtime/extract/LightSystem.hpp::lightType`, `runtime/codecs/FieldAliases.hpp`; REQ-PROJECTION | Partial |

## Common semantics, encodings and SAI

| Requirement family | Evidence inspected | Work needed for sign-off |
|---|---|---|
| [Concepts §4](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html): graph identity, execution contexts, DEF/USE, PROTO, IS, ROUTE, event cascade | `X3DScene.hpp`, `X3DProtoExpand.hpp`, `X3DSceneBridge.hpp`, `X3DEventCascade.hpp`; codec PROTO and event tests | Link each normative rule to assertions, including nested contexts, dynamic Inline unload, ordering, fan-out and event cycles. Existing broad tests do not provide a complete clause map. |
| [Fields §5](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fieldTypes.html): types, defaults, access, ranges and event aliases | Generated reflection, `FieldValueIO.cpp`, `X3DRangeValidate.hpp`, write-field and codec tests | Audit all field families in each claimed encoding and script binding. UOM default parity establishes only the structural part. |
| Profile/component availability and diagnostics | `profiles.yaml`, CLI validation, `RuntimeSession::create` | Distinguish document validity from host capability. The host must reject requirements it cannot meet; successful lenient parsing is not evidence that it can present the file. |
| Encoding contracts | XML/ClassicVRML/JSON readers and writers, `codec_roundtrip_audit_test.cpp` and PROTO regressions | Pin the applicable ISO encoding editions separately. JSON and VRML97 need their own claims. Binary encoding is not automatically required for a host claiming other encodings. |
| SAI and scripting-language binding | `SaiContext.hpp`, `docs/conformance/sai-services.yaml`, `scripts/check_sai_services.py` | All 29 registered service rows are partial. The validator checks schemas/test-name existence, not full semantics. Inventory the unregistered services and reconcile the independent `x3d-sai` work only when integration is evidenced. |
| Optional feature configuration | CMake flags, `attachStandardRuntime`, `SessionOptions`, consumer setup | Produce one named configuration that actually enables every backend needed by the claim; test its resource wiring. Merely having an interface or backend library is insufficient. |

The native C++ SAI is developed separately under
[ADR-0047](../decisions/0047-sai-sister-repo-split.md). A read-only check of
`x3d-sai` at `856cf3e` on 2026-09-28 found a stale generated baseline reporting
65 services. Rechecking the source register found 36 rows: one implemented,
24 partial and 11 planned. The existing test binary passed 90 cases and 60,514
assertions, but `scripts/sai_conformance.py check` failed on seven invariants
without preserving services; `strict` reported 149 diagnostics. Passing that
test suite therefore does not establish readiness for integration.

The public `scene_edit` API supports prototype declarations but lacks prototype
instance creation and IS bindings. The next step is the declaration model's
independent review, followed by a composed instance/IS test proving independent
instance state, shared declaration identity and atomic rejection of invalid
bindings. Inline contexts, live cross-context operations and the runtime adapter
remain subsequent work. Keep the sister repository's conformance evidence
separate until an adapter actually exercises this runtime.

## Executed reproduction: UNIT angle conversion

After `build-ci/x3d` was rebuilt from the audit baseline, export this scene with
`build-ci/x3d extract degrees.x3d -o degrees.stl`:

```xml
<X3D version="4.0" profile="Full">
  <head><unit category="angle" name="degree"
              conversionFactor="0.017453292519943295"/></head>
  <Scene><Transform rotation="0 1 0 90">
    <Shape><Box size="2 4 6"/></Shape>
  </Transform></Scene>
</X3D>
```

Export a second scene with an empty head and rotation `0 1 0 1.5707963267948966`.
Both produce twelve triangles. Their world-space bounds should agree, allowing
float rounding. Reading the binary STL vertices gives these positive extents:

| Input | X | Y | Z |
|---|---:|---:|---:|
| Degrees with UNIT | 3.1300633 | 2.0 | 2.2382174 |
| Equivalent radians | 2.9999998 | 2.0 | 1.0000001 |

The discrepancy reproduces REQ-UNIT through parse, scene construction, extraction
and export. It establishes the angle case; length, mass, force and referenced
document cases still need their own acceptance tests. The probe used temporary
fixtures and is not a newly installed regression test.

## Capacity and resource acceptance suite

The [Full-profile tables](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fullProfile.html#t-OtherLimitations)
contain explicit lower bounds. These are additional acceptance fixtures, not
assertions that the current code fails at each bound:

| Family | Boundary fixtures to run in the claimed host |
|---|---|
| Geometry | 15,000 coordinates/colors/normals/indices where specified; 5,000 faces with ten vertices each; 16,000 grid heights; 2,500 Extrusion cross-section/spine products. |
| Scene/event scale | 500 children; 1,000 interpolation keys; five nested PROTO definitions and 30 fields per access category; 25 Script fields per category. |
| Presentation | Eight simultaneous lights; six clipping planes; six layers; 512×512 image/pixel textures; 512³ volume data. |
| NURBS | Order 30, 1,500 control points, and ten trimming contours. |
| Text and names | Required Latin-1 repertoire; 100 strings of 100 characters; 50-byte identifiers and the field-specific string/array bounds. |
| Resources | Ten-URL fallback; relative URLs and file/http/ftp; JPEG/PNG, the specified MPEG-1 movie variants and 30-second uncompressed PCM WAV. |
| Field storage | Exercise every SF/MF type at its stated numeric precision and array limit, including matrices, doubles, images and UTF-8 strings. |

Inspect `runtime/RecursionLimits.hpp` alongside these fixtures. Resource limits
may exceed normative minima; their existence alone is not a violation. Check
actual truncation and output, not just parsing success. The PoC's eight-light
constant matches the light count but does not prove all eight contribute
correctly. The current volume path already has a functional gap before capacity
can be measured.

## Exceptions, ambiguities and corrections to the earlier assessment

- H-Anim joint limits/stiffness are application hints. Published §26.3.3 explicitly
  leaves enforcement to applications; adding an IK solver is not a prerequisite
  for this claim. Verify stored fields and routing separately.
- The published Scripting component generally permits language choice, while
  Full table F.3 expressly names Java and ECMAScript. REQ-JAVA records the stricter
  profile obligation; it need not become a mandatory dependency of the base SDK.
- Shaders require at least one supported language, not every listed language.
  Unsupported-language nodes must retain their scene/event behavior (§31.2.4).
- `ENC-PROTO-APPINFO` describes loss across a grammar boundary. Its note states
  that ClassicVRML has no corresponding declaration metadata production. Treat
  this as a conversion limitation pending the separate encoding audit; do not
  invent a syntax extension or count it as proof of a browser defect.
- `VIEWPORT-CLIP` and `LAY-3` overlap. Track required clipping separately from the
  chosen viewport remapping contract; the latter has a recorded interpretation
  issue in ADR-0035.
- HAnimMotion currently cites a 2.1 working draft for some behavior. Reconcile
  that evidence with the H-Anim edition referenced by the chosen 4.0 standard.
- `NurbsWeightMode` deliberately offers two conventions (NRB-4); conformance
  fixtures must name the chosen interpretation. Peer-browser agreement alone
  does not settle contradictory normative text.
- The built-in geospatial backend needs an injected geoid model to perform
  geoid-relative height conversion. A configured PROJ/tinygeoid path must be part
  of the tested configuration when that capability is claimed.
- Movie decoding and a PoC movie-texture path already exist. The stale blanket
  deferral in `v1-capabilities.md` must not be interpreted as absence of all movie
  playback. Verify the actual timing, required formats and background use cases.
- Some historical `fixed` rows explicitly leave work behind: TXF-4 leaves CPU
  minification/mipmaps; SND-7 leaves destination/channel behavior. Re-read closure
  notes when assigning requirements. Do not treat all ignored fields as bugs:
  first check the component's latitude and profile exceptions.

## Completion order and exit criteria

1. Correct the profile reference data and separate structural inventory from
   semantic evidence in release reporting. Assign every inventory row a clause
   owner and explicit supported/partial/unverified status. Resolve published
   table/UOM discrepancies before claiming profile levels.
2. Close common-runtime obligations first: UNIT semantics, required capability
   handling, and remaining event/context evidence. Run the applicable capacity
   cases against Interchange and Interactive configurations.
3. Complete the presentation paths needed by the intended host: shader execution,
   clipping/fill, local fog, multitexturing, cube maps, shadows and projectors.
   Test observable output and runtime edits, not only descriptors.
4. Implement independent layers/viewports before screen layout. Implement 3D
   texture resources before volume rendering. Finish picking sensors, NURBS,
   particles, DIS, audio and physics using the existing finding groups.
5. Finish the separately scoped encoding/SAI/language-binding audits. Configure
   Java, resource protocols, fonts and media for the Full host, and prove their
   interaction with dynamic scene loading and scripts.
6. For each applicable normative rule, retain a test/assertion, observable oracle,
   configuration, result and accepted interpretation. Close a finding only when
   all its required behavior is covered, or split the remaining obligation into
   another finding. Run component interactions and all profile minima before
   publishing a Full claim.

This order is dependency-based, not an effort estimate. The node inventory is a
coverage denominator; the findings register is the implementation backlog. A
release sign-off requires both semantic evidence and the complete host's results.

## Verification of this audit

- `mise run ci` passed: 764 Python tests passed and 12 skipped; C++ suites,
  header/install checks, golden parity, coverage/doc gates and the CLI differential
  regression gate passed. This verifies the existing suite, not every normative
  requirement listed above.
- The focused conformance-view and SAI-register tests passed (33 tests), and both
  register validators passed. Regeneration has no unresolved finding references.
- The strict wiki build passed. Inventory checks matched all 260 concrete nodes,
  4,191 field entries, 36 component chapters and 45 retrieved sources; source
  hashes, binding paths and finding references matched.
- REQ-UNIT was reproduced with the rebuilt CLI. The other new findings are based
  on source inspection and retain explicit acceptance cases for implementation.

The initial audit changed no runtime code. The follow-up corrects profile data
and coverage reporting and validates UNIT declarations at parse time. It does
not yet apply UNIT factors to runtime calculations. These checks do not
substitute for the missing acceptance fixtures or a complete host conformance run.

Follow-up verification, 2026-09-28: `mise run ci` exited 0 with these output
excerpts after the profile, reporting and UNIT-validation changes:

```text
[test] 767 passed, 12 skipped in 62.71s (0:01:02)
[build-ci] 100% tests passed out of 50
[build-ci] 100% tests passed out of 2
[build] 100% tests passed out of 56
[cli-gate-regression] PASS — no regressions (all baseline-PASS items still pass).
Finished in 104.41s
```

The documentation drift advisory could not reach its local embeddings service
at `localhost:8080`. Its citation-only check was run separately and the reader,
capability and conformance documentation was reviewed and updated.

### FillProperties follow-up

REQ-FILL is fixed in extraction and the CPU/OpenGL reference consumers. The
regressions cover defaults, all four filled/hatched combinations, required
styles 1–6, unsupported-style fallback, color, depth holes, back-facing polygons,
material models, and live field edits and replacement. CPU integration tests
also check that suppressing polygon fills does not suppress lines or points.
The register now contains 35 open/deferred findings; it is still not an
exhaustive count of unmet requirements.

`mise run ci` exited 0 after these changes:

```text
[test] 767 passed, 12 skipped in 118.21s (0:01:58)
[build-ci] 100% tests passed out of 50
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 2
Finished in 240.35s
```

`env -u WAYLAND_DISPLAY -u XDG_SESSION_TYPE mise run validate-examples` exited 0,
including these output lines:

```text
OK: FillProperties GL pixels (defaults, four fill states, hatch styles 1-6/fallback, Phong/PBR/Unlit, depth holes, backfaces)
== examples validated: cpu_raster + x3d2svg + poc_renderer + asset_import compile and run headless ==
```

The isolated Xvfb run selected the NVIDIA OpenGL driver despite the software-GL
environment request. These are hardware GL pixel results, not llvmpipe results.

### Native SAI identity follow-up

Review of the declaration model reproduced identity reuse after abandoning an
edit: a retained declaration handle resolved to a different, subsequently
committed declaration. Node allocation used the same rollbackable counter
design. The sister repository now reserves identities in the execution context;
abandoned, poisoned and conflicting edits consume IDs without making them
available to later objects. Local and external declarations share their sequence.

The isolated fix passed the full build and CTest command:
`cmake --build /tmp/x3d-sai-identity-build -j 3 && ctest --test-dir /tmp/x3d-sai-identity-build --output-on-failure`.

```text
100% tests passed out of 100
Total Test time (real) = 0.21 sec
```

The reviewed patch was applied to `x3d-sai` on branch
`fix/aborted-edit-identities`. The original reproduction, linked against that
repository's rebuilt library, now reports:

```text
abandoned_id=1 replacement_id=2 abandoned_handle_resolves=0
```

In the actual sister repository, `cmake --build build -j 3` followed by
`ctest --test-dir build --output-on-failure` also exited 0:

```text
100% tests passed out of 100
Total Test time (real) =   0.11 sec
```

This fixes stale handles impersonating later objects. It does not complete the
declaration review, prototype instance/IS support, or the independent SAI
conformance gates described above.

### UNIT integration evidence

The initial runtime review found a gap beyond conversion arithmetic.
`RuntimeSession` retains the document header, but passes only `doc.scene` into
`X3DExecutionContext`. Inline resolvers return a `Scene`, while external
prototype resolvers return a `ProtoDeclaration`; those payloads originally
discarded the referenced document's unit declarations. The source snapshots
described below now preserve that provenance. Conversion during runtime
evaluation and event processing remains open.

The native SAI's generated metadata has a separate gap. Its UOM parser has no
per-field unit-category member, the metadata emitter supplies `std::nullopt`,
and `unitCategoriesComplete()` is explicitly false. The adapter preserves those
facts. Unit-aware operations on handwritten descriptors do not establish unit
support for generated X3D node types. A specification-backed field-dimension
mapping is still required; field names alone are insufficient evidence.

The native SAI kernel now accepts the five derived field categories from
Architecture §4.3.6: acceleration, angular velocity, area, speed and volume.
Explicit field and declaration-interface descriptors use the appropriate
length or angle factor, with squared and cubed factors for area and volume.
Direct UNIT declarations remain restricted to the four base categories.
Storage and descriptor defaults remain canonical; authored reads and writes
cross the conversion boundary. Unrepresentable conversion factors are rejected,
but scalar payload multiplication can still overflow. This does not close
REQ-UNIT in the runtime SDK.

### Native SAI template-event follow-up

The declaration review reproduced a second scope violation: `event_batch::send`
accepted a template node as an SFNode payload and committed it into an ordinary
scene field. The reproduction reported:

```text
send=1 commit=1 scene_field_points_to_template=1
```

The fix rejects template nodes as live event targets and as SFNode/MFNode event
payloads. Capability queries report template targets as unwritable by event
intent; rejected batches preserve both scene and template state. Ordinary
scene-node events remain supported. The regression is registered as
`sai_decl_event_scope`.

After combining the scope fix and derived-unit support in the actual `x3d-sai`
working tree, `cmake --build build -j 3` followed by
`ctest --test-dir build --output-on-failure` exited 0:

```text
100% tests passed out of 102
Total Test time (real) =   0.11 sec
```

The original scope-leak reproduction, rebuilt against that repository's library,
now reports `send=0 error=14` (`invalid_context`).

The declaration stop/go review remained incomplete after this fix. At that
point the service register lacked declaration-service rows, and its schema
check reported seven invariants without preserving services. The historical
review plan also names a `golden` task that is absent from the sister
repository's current task list; its generator gate is `gen-determinism`.

### EXTERNPROTO and declaration-register follow-up

The independent review found that external resolution incorrectly required
identical interface order and defaults. Architecture §4.4.5 permits an external
interface to expose a subset of the implementation's fields and forbids locally
authored external defaults. The kernel now matches fields by name, kind and
access. Omitted optional C++ unit/type annotations do not restrict resolution;
explicit conflicting annotations are rejected. Changes to unexposed local
fields and implementation defaults no longer invalidate an external reference.
`sai_decl_external_subset` tests this behavior and rejection without publication.

The review also reproduced dangling numeric node IDs accepted as type and
declaration-interface defaults. Shared descriptor validation now rejects nonzero
raw node IDs, including those in MFNode defaults. Null defaults and null list
positions remain representable. At that stage, non-null node defaults still
required a context-bearing authoring API with ownership and retention checks.
That API is covered in the follow-up below; the raw-ID restriction remains.

The service register now contains 63 rows: one implemented, 48 partial and 14
planned. It records declaration lookup, enumeration and mutation alongside the
existing graph and descriptor services. Three planned framework-policy rows
cover provenance, validation and adapter traces without claiming ISO service
coverage. The generated baseline maps 35 of 90 ISO services and 12 of 16 listed
C++ binding obligations; the binding audit remains incomplete.

The normal register checks validate consistency, evidence names and generated
drift, and now run in the sister repository's `mise run ci`. The strict gate
still rejects unfinished services, missing tests, unmapped services and binding
obligations. Neither a consistent register nor the declaration regressions
establish full SAI conformance or complete the declaration stop/go review.

Verification on 2026-09-29: `mise run ci` in the actual sister repository exited
0, including the UOM pin, generator determinism, both register checks, Python
tests, C++ build and CTest. Output excerpts:

```text
SAI conformance OK: 63 services, 35 invariants
============================== 30 passed in 0.10s ==============================
100% tests passed out of 104
Total Test time (real) =   0.13 sec
```

A separate Debug build used `-O0 -g1 -fsanitize=address,undefined
-fno-omit-frame-pointer`, with the same sanitizer flags at link time.
`ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/x3d-sai-declaration-san -R '^sai_decl_' --output-on-failure`
exited 0:

```text
100% tests passed out of 17
Total Test time (real) =   0.42 sec
```

The valid external-interface reproduction, linked against the actual rebuilt
library, now reports `resolved`. The strict conformance command still exits 1;
55 ISO services and four listed C++ binding obligations remain unmapped, in
addition to partial/planned services and missing executable invariant tests.

### Context-bearing PROTO defaults and live ownership claims

Local declarations now accept non-null SFNode and MFNode interface defaults
through `scene_edit::set_declaration_default`. Callers supply node handles;
MFNode defaults preserve order, shared references and null positions. Validation
checks context, handle lifetime, field access and accepted node types before
claiming the reachable node graph into the declaration's scope. Body roots and
defaults may share nodes within that scope. Ordinary scene references and nodes
owned by another declaration block the claim.

Replacing or clearing a default retains previously claimed nodes until the
declaration is removed. Removal releases those nodes without deleting them.
Descriptor updates can preserve a validated default, but cannot introduce new
nonzero node IDs. Snapshots retain the old defaults after later edits.

The review also reproduced a race: another scene could commit an import after
an ownership claim was staged, and the source edit would then claim the imported
node as private template state. Commit now rechecks newly claimed nodes under
the context locks against committed imports, active field observers and
undrained events, including nodes carried in event payloads. Rejection leaves
the published source revision, exports and declarations unchanged.

The service register contains 64 rows after adding the default-authoring API:
one implemented, 49 partial and 14 planned. ISO service coverage remains 35 of
90. These changes do not provide PROTO instance creation, IS connections or
runtime UNIT integration, and the declaration stop/go review remains open.

Verification on 2026-09-29 in the actual sister repository:

```text
$ cmake --build build -j3 && ctest --test-dir build --output-on-failure
[100%] Built target x3d_sai_experimental_tests
100% tests passed out of 106
Total Test time (real) =   0.11 sec

$ cmake --build /tmp/x3d-sai-declaration-san -j2 --target x3d_sai_experimental_tests && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/x3d-sai-declaration-san -R '^sai_decl_' --output-on-failure
[100%] Built target x3d_sai_experimental_tests
100% tests passed out of 19
Total Test time (real) =   0.50 sec

$ mise run sai-conformance-gate && mise run sai-invariants && uv run pytest tests/ -q
SAI conformance OK: 64 services, 35 invariants
SAI conformance OK: 64 services, 35 invariants
30 passed in 0.10s
```

The sanitizer build retains the ASan/UBSan flags documented above. These are
targeted build, regression and register checks; the full generator pipeline was
not rerun for this change. `mise run sai-conformance` still exits 1, reporting
unfinished services and an incomplete C++ binding audit.

### External resolution authority and reserved prototype fields

The next review reproduced a declaration-identity error: an external descriptor
could supply a numeric declaration ID obtained from another context and resolve
to an unrelated local declaration with the same number. The native API now
requires `scene_edit::resolve_external_declaration(external, local)` with
context-bearing handles. It checks both identities, declaration kinds and
interface compatibility before staging a resolution change. Descriptor updates
may preserve the existing validated target or release it, but cannot introduce
or redirect a target through a bare ID. Successful resolution clears an earlier
failure diagnostic. The new regression is `sai_decl_external_authority`.

PROTO and EXTERNPROTO interface creation and updates now reject redeclaration
of the inherited `metadata` field. This follows
[Architecture §§4.4.4.2 and 4.4.5.2](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html);
ordinary node metadata fields remain valid. The
`sai_decl_interface_metadata` regression checks rejection without publishing
either a new declaration or a changed interface.

These checks do not establish complete prototype semantics. The API still
permits incomplete local descriptors, including absent initialization defaults
and empty bodies. Instance creation, IS connections, nested declarations and
template ROUTEs remain implementation work. The new resolution method binds an
existing local declaration; it does not implement URL fetching or the SAI
`requestImmediateLoad` service.

Verification on 2026-09-29 used the actual sister repository. The service
register now records 65 services (one implemented, 50 partial, 14 planned), with
the same 35 of 90 ISO services mapped. Command output excerpts:

```text
$ cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build -j3 && ctest --test-dir build --output-on-failure
[100%] Built target x3d_sai_experimental_tests
100% tests passed out of 108
Total Test time (real) =   0.12 sec

$ cmake --build /tmp/x3d-sai-declaration-san -j2 --target x3d_sai_experimental_tests && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/x3d-sai-declaration-san -R '^sai_decl_' --output-on-failure
[100%] Built target x3d_sai_experimental_tests
100% tests passed out of 21
Total Test time (real) =   0.54 sec

$ mise run sai-conformance-gate && mise run sai-invariants && uv run pytest tests/ -q
SAI conformance OK: 65 services, 35 invariants
SAI conformance OK: 65 services, 35 invariants
30 passed in 0.11s
```

### Runtime source UNIT provenance

`parseDocument` now snapshots `head.units` into `Scene::sourceUnits` and each
reachable local `ProtoDeclaration::sourceUnits` before expansion. Existing
resolver return types carry these owning snapshots, so file and asset loads
retain child-document units after the temporary document is destroyed. Empty
child snapshots remain empty; the parser does not replace them with the parent
document's declarations. Writers still serialize `Head`.

EXTERN expansion now retains its selected declaration on the instance and in
`Scene::expandedSources`. Retention does not cache future expansion attempts:
each attempt calls the supplied resolver, and failed resolution clears the
current retained declaration. Tests cover distinct parent/child factors,
children without UNIT statements, local nested prototype references, file and
asset external resolution, and header round-trip.

This preserves information required by Core §7.2.5.5's external-content
alignment rule. It does not change numeric values or close REQ-UNIT. A separate
reader review at that point found no retained distinction between explicitly authored scalar
values and generated defaults: XML assigns attributes, JSON assigns `@` fields,
and Classic assigns field tokens, then discards that distinction. PROTO defaults
and instance overrides have separate value records. Conversion must account for
those boundaries and use a specification-backed field-dimension map; the UOM
has no structured per-field dimension attribute.

Verification on 2026-09-29: `CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci` exited 0
in the main working tree. Output excerpts:

```text
[test] 767 passed, 12 skipped in 53.15s
[build-ci] 100% tests passed out of 50
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 2
[cli-gate-regression] PASS — no regressions (all baseline-PASS items still pass).
[cli-gate-regression] TIER 1 idempotence: 200/200 (100%) (OK)
Finished in 184.31s
```

`mise run docs-build` exited 0 (`Documentation built in 2.90 seconds`). The
advisory `mise run docs-drift` failed because its embedding service at
`localhost:8080` was unavailable; direct file citations were checked instead.


### Authored scalar-field provenance

The XML, JSON and Classic readers now record accepted scalar assignments,
including explicit values equal to generated defaults. `Scene::authoredScalarFields`
uses weak node identity, so DEF/USE references share marks without keeping nodes
alive. These are parse-time records; ordinary runtime writes do not update them.

PROTO declarations retain body and node-valued interface-default marks after
reader-local scenes and external source documents are destroyed. Expansion
copies body marks onto cloned nodes and records successful scalar IS assignments.
Marks on node-valued defaults and nested overrides follow those graphs into the
destination scene. Unknown fields, read-only fields and rejected enum tokens
are not marked. Existing lenient numeric parsing remains unchanged.

This adds provenance only. Field dimensions, conversion rules for defaults and
interface values, numeric conversion, and conversion-aware serialization remain
open under REQ-UNIT. No conformance requirement is closed by these changes.

Final-source verification on 2026-09-29:
`CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci` exited 0. Output excerpts:

```text
[test] 767 passed, 12 skipped in 33.79s
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 50
[build-ci] 100% tests passed out of 2
[cli-gate-regression] TIER 1 idempotence: 200/200 (100%) (OK)
Finished in 65.03s
```

The final run rebuilt `x3d_proto_front_door` in both presets and exercised the
nested literal-override regression. The strict documentation build is also
required for this update. `mise run docs-drift` again failed to connect to its
embedding service at `localhost:8080`; directly affected reader, scene, PROTO,
capability and conformance pages were updated instead.


### UNIT conversion semantics review

Architecture [§4.3.6](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#Standardunitscoordinates)
uses initial base units in the specification and requires conversion as needed
for processing. This supports keeping generated built-in defaults in canonical
units while converting explicitly authored dimensional values. That default
interpretation is an implementation inference; it is not a separately stated
rule for omitted fields in the UNIT clause.

As corroborating implementation evidence, X_ITE revision
`73055733df2ab18ff5e9e7af944a0707a44a588a` marks
[Box.size as length](https://github.com/create3000/x_ite/blob/73055733df2ab18ff5e9e7af944a0707a44a588a/src/x_ite/Components/Geometry3D/Box.js),
passes actual XML attributes through
[XMLParser.fieldValue](https://github.com/create3000/x_ite/blob/73055733df2ab18ff5e9e7af944a0707a44a588a/src/x_ite/Parser/XMLParser.js),
and applies unit factors to scalar/vector values and rotation angles in
[VRMLParser](https://github.com/create3000/x_ite/blob/73055733df2ab18ff5e9e7af944a0707a44a588a/src/x_ite/Parser/VRMLParser.js).
The rotation axis is not scaled. These files were fetched and checked against
the pinned revision; this is source inspection, not an executed browser test.

Custom PROTO scalar/vector fields require a separate decision. X_ITE's
[X3DField.addReference](https://github.com/create3000/x_ite/blob/73055733df2ab18ff5e9e7af944a0707a44a588a/src/x_ite/Base/X3DField.js)
wires IS relationships without copying unit categories, while XML user-field
creation constructs fields without a unit annotation. This does not justify
inferring dimensions from all numeric types or from arbitrary IS targets.
One interface field can feed multiple compatible field types with different
physical meanings. The runtime's conversion policy must resolve this case
explicitly, with tests, before REQ-UNIT can close.

### Runtime UNIT conversion progress

Runtime entry now normalizes explicitly authored, recognized dimensional
fields to initial/SI units. Generated built-in defaults stay in canonical
units, and runtime writes and routed updates use those initial units. The
implementation carries each source scene's UNIT declarations into Inline and
EXTERNPROTO expansion and converts PROTO values at dimensional IS targets.
This includes angle and length geometry, plus mass, force and torque values
consumed by `PhysicsSystem`.

Focused evidence is in `runtime_session_test.cpp`:
`UNIT runtime: equivalent authored length and angle geometry` compares mesh
and transform output for degree/centimeter and radian/meter scenes;
`UNIT runtime: routed motion, defaults, and subsequent writes use initial units`
covers routed values, canonical defaults and later writes;
`UNIT runtime: Inline and EXTERNPROTO use their source units` covers source
factors and confirms a codec round-trip before runtime preserves authored
values and UNIT declarations;
`UNIT runtime: PhysicsSystem receives kilograms and newtons` checks mass,
force and torque inputs; and
`UNIT runtime: custom PROTO values convert independently at dimensional IS targets`
checks that a single PROTO value is converted according to each target field.

REQ-UNIT remains open. The known field-dimension mapping is incomplete and its
coverage range is unresolved; conversion-aware writing after runtime has not
been established. The conflicting published Core wording for external UNIT
declarations also remains a separate specification question.


### PROTO node initialization isolation

Reviewing the UNIT conversion boundary exposed a separate violation of
[§4.4.4.2](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#PROTOInterfaceDeclarationSemantics):
node-valued defaults were installed directly from the declaration instead of
being copied per instance. The focused reproduction used the original expansion
header with the new regression test:

```text
build/x3d_parse_tests --test-case='PROTO node defaults belong to each instance' --no-colors
values: CHECK( 0x562eccc7e928 != 0x562eccc7e928 )
values: CHECK( 7 == 2 )
[doctest] test cases:  1 | 0 passed | 1 failed | 54 skipped
[doctest] assertions: 14 | 9 passed | 5 failed |
[doctest] Status: FAILURE!
```

The test showed both instances and the declaration sharing a Box; changing one
instance's size changed the other instance and the template. The fix uses the
body clone map for interface defaults and nested literal node overrides, so
aliases remain shared within an instance while independent instances receive
independent graphs. Authored-field metadata follows the clones. Explicit caller
node overrides retain caller identity, as required for DEF/USE sharing.

The same effective-value path now treats a present node fieldValue with no
nodes as an explicit NULL/empty override. It no longer falls back to the
interface default, including across nested IS forwarding. These fixes are
tracked by PROTO-NODE-INIT; broader prototype and UNIT obligations remain open.

Main-tree verification on 2026-09-29:
`CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci` exited 0. Output excerpts:

```text
[test] 767 passed, 12 skipped in 52.07s
[build-ci] 100% tests passed out of 50
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 2
[cli-gate-regression] TIER 1 idempotence: 200/200 (100%) (OK)
Finished in 127.62s
```

The advisory docs-drift command again failed to connect to the embedding
service at `localhost:8080`. The directly affected PROTO subsystem page,
capability matrix, conformance finding and this audit were updated. This
verification covers the initialization changes; complete reader DEF scoping,
prototype semantics and UNIT conversion remain separate obligations.

### PROTO reader scope and nested declaration retention

The XML and JSON readers previously parsed interface node defaults against the
outer scene's DEF table; Classic parsed each default against a separate table.
All three now use one private DEF scope for a declaration's interface and body,
with enclosing prototype declarations available by name. Nested declarations
replace inherited prototype names in that private lookup scope, including
PROTO/EXTERNPROTO shadowing. XML also recognizes declarations directly inside
ProtoBody. Parent scope records and already-bound instances retain their identity.

`declarationDefScopeTest` and `nestedDeclarationShadowingTest` exercise all three
encodings. The original XML reader failed the new assertion that an interface
default cannot USE an outer DEF. These changes close PROTO-PARSE-SCOPE.

Serialization remains incomplete: ProtoBody retains nodes, routes, IS connections
and instances, but not authored nested declaration statements. A parsed local
`Choice` declaration containing a Sphere was omitted when its enclosing
`WrapProto` was written; only Group and ProtoInstance Choice remained. Reparse
can therefore select an outer Choice or fail. PROTO-NESTED-WRITE tracks retaining
both declarations and their lexical placement, including unused declarations
and instances on either side of a shadowing declaration. Emitting declarations
first would not preserve those bindings in general.

### Native SAI unit conversion range

The sibling kernel accepted finite authored values whose unit conversion
exceeded the destination field's finite range. The fix checks float bounds
before narrowing and rejects nonfinite results from finite double inputs.
Vector components, rotation angles and arrays propagate conversion failure.
Existing explicit NaN/Inf behavior and ordinary rounding remain unchanged.

Public API regressions cover scalar and aggregate writes, cancellation of an
event batch containing an earlier valid event, and direct/imported authored
reads whose inverse conversion overflows. The three tests failed before the
fix and passed afterward in the isolated implementation checkout. Service and
invariant registers now link these regressions; field services remain partial,
and this does not implement the codec bridge or generated field dimensions.

Integrated main-tree verification on 2026-09-29:
`CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci` exited 0. Output excerpts:

```text
[test] 767 passed, 12 skipped in 54.07s
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 50
[build-ci] 100% tests passed out of 2
[cli-gate-regression] TIER 1 idempotence: 200/200 (100%) (OK)
Finished in 196.53s
```

`mise run conformance-gate` reported:
`Conformance view OK: committed docs/conformance/ matches sources.`
The generated register contains 283 findings: 36 open and 247 closed. This is a
finding count, not a percentage of full standard coverage. The advisory
`mise run docs-drift` failed because `localhost:8080/v1/embeddings` was unavailable;
affected subsystem and capability documentation was updated directly.

Integrated sibling verification used
`cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build -j2 && ctest --test-dir build --output-on-failure`:

```text
100% tests passed out of 111
```

The remaining CI tasks were run explicitly: `mise run uom-pin`,
`mise run gen-determinism`, `mise run sai-conformance-gate`,
`mise run sai-invariants`, and `mise run pytest`. Register checks reported
`SAI conformance OK: 65 services, 35 invariants`; Python tests reported
`30 passed in 0.22s`. The bounded build replaces CI's unbounded `-j` invocation.
These gates validate implementation and register consistency, not complete SAI
conformance. The service register remains 1 implemented, 50 partial and 14 planned.

### Ordered PROTO statements and shared writer scope

The follow-up review reproduced an additional DEF/USE serialization failure.
An interface default `Group DEF='Shared'` and a body child `Group USE='Shared'`
share identity after reading. The command
`build/x3d convert /tmp/x3d-proto-default-alias.x3d -f xml -o /tmp/x3d-proto-default-alias-out.x3d`
exited 0 but emitted another `Group DEF="Shared"` in the body. That divides one
node into two on reparse and duplicates its name in the same scope.
PROTO-WRITE-DEF-SCOPE tracks a writer context shared by interface and body,
with fresh contexts for nested declarations.

The expansion model also loses the distinction between the first node and its
peers when the first node is a prototype instance. A Leaf prototype containing
Group, a Wrap body containing only ProtoInstance Leaf, and an instance of Wrap
produced this result from `build/x3d validate /tmp/x3d-proto-primary.x3d`:

```text
valid:         no
[proto]
  warning: Wrap: empty proto body
```

The command exited 3. PROTO-PRIMARY-ORDER tracks selecting the first authored
body node, including prototype instances, while retaining subsequent nodes as
active peers outside the rendered hierarchy. If that first instance cannot
expand, promoting a later peer would change the prototype's type and is not a
valid recovery. These requirements follow
[§4.4.4.3–4.4.4.4](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#PROTODefinitionSemantics).

A reader inventory confirmed that declarations inside ordinary node fields and
Classic node-valued defaults also lose their authored placement. Retaining
ordered direct ProtoBody statements addresses only part of PROTO-NESTED-WRITE;
that finding must remain open until these additional placements are preserved.

The integrated implementation now records direct body declarations and
node/instance order in `ProtoBody::statements`. Existing node and instance
collections remain authoritative; `orderedStatements()` skips removed nodes
and invalid indices, preserves repeated node occurrences, and appends new
unrecorded nodes/instances. Reordering or erasing the public instance vector
requires updating statement indices. The same order drives all four writers
and runtime primary/peer selection. Unused nested declarations also retain
source UNIT snapshots.

Each declaration writer now shares a fresh DEF/USE context between interface
and body. Nested declarations create independent contexts. The regression
reparses each writer's output and checks pointer identity across both interface
fields and the body, plus independence of a nested declaration's same-named DEF.

The old expander failed all three new model cases. The fixed isolated suite
reported 59 passing cases and 17,125 assertions. Integrated focused verification
used `cmake --build build --target x3d_proto_front_door x3d_parse_tests -j8 && ctest --test-dir build --output-on-failure -R '^(x3d_proto_front_door|x3d_parse_tests)$'`
and reported `100% tests passed out of 2` before the writer-context increment.
The final full CI run below covers the combined changes. Review removed an
unneeded copy of the consumed-node counts; replay uses the existing counts
once their earlier role is finished.

Combined main-tree verification on 2026-09-29:
`CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci` exited 0 after regeneration of the
conformance views. Exact output excerpts:

```text
[test] 767 passed, 12 skipped in 54.24s
[build-ci] 100% tests passed out of 50
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 2
[cli-gate-regression] TIER 1 idempotence: 200/200 (100%) (OK)
Finished in 190.06s
```

The original alias reproduction now emits `Group USE="Shared"` in the body.
The nested-only-body reproduction no longer produces `empty proto body` and
its parsed expansion is covered by the regression tests. CLI validation still
exits 3 because its separate unused-prototype diagnostic overlooks Leaf's use
inside Wrap; DIAG-NESTED-PROTO-USE records that remaining false positive. This
is not a claim that the fixture passes all CLI validation.

The advisory docs-drift command again failed to connect to the local embedding
service. The affected documentation was updated directly. No commits or pushes
were performed.

### Nested declaration-use diagnostics

DIAG-NESTED-PROTO-USE is fixed in the CLI and differential gate through the
shared private `tools/x3d-cli/proto_use.hpp` analysis. Uses are recorded by bound
declaration identity, including those inside local prototype bodies. The walk
follows both retained declaration statements and local instance bindings, so
node-contained declarations whose placement is not yet serialized are still
examined. A visited worklist prevents repeated traversal and cycles.

External instances count their external declaration rather than the resolved
implementation. Generated `expandedSources` records are excluded: authored
instance lists already carry the uses in this source, and generated records
can originate in external implementations. Merely declaring a local prototype
does not count as using it; a used shadowing declaration cannot hide an unused
outer declaration with the same name.

The original nested-only-body fixture and a node-contained declaration fixture
failed with `unused-proto` before the fix and validate without diagnostics in
the isolated fixed build. Regression checks retain warnings for an unused outer
declaration and the existing unused fixture. An unresolved external instance
still reports its resolution warning without an additional unused warning.
This is a correction to an authoring diagnostic, not completion of the remaining
prototype semantics or declaration-placement work.

Integrated verification: `CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci` exited 0:

```text
[test] 767 passed, 12 skipped in 19.67s
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 50
[build-ci] 100% tests passed out of 2
[cli-gate-regression] TIER 1 idempotence: 200/200 (100%) (OK)
Finished in 54.64s
```

`build/x3d validate /tmp/x3d-proto-primary.x3d --json` now exits 0 with
`"valid": true`, an empty diagnostics array and `"counts": {"total": 0}`.
The conformance generator reports 286 findings (36 open, 250 closed); these
counts do not measure complete standard coverage. The advisory docs-drift
service remains unavailable at localhost:8080; the CLI subsystem and audit
were updated directly.

### Declaration placement inside prototype nodes

The next reproduction uses a valid Classic node-body sequence: Collision's
`proxy` field instantiates an outer Choice prototype, a local Choice declaration
then shadows it, and `children` instantiates the local Choice. Before the fix,
`build/x3d convert /tmp/x3d-proto-field-placement.x3dv -f xml -o /tmp/x3d-proto-field-placement-before.x3d`
exited 0 but omitted the local declaration. Reparse consequently selected the
outer Choice for both fields.

Encoding grammar matters here. The published
[Classic VRML Annex A](https://www.web3d.org/documents/specifications/19776-2/V3.3/Part02/grammar.html)
allows declarations as node-body statements between field assignments; MFNode
list values contain node statements, not prototype declarations. The available
[4.0 Classic encoding draft](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19776-2v4.0-CD/Part02/grammar.html)
retains that distinction. The reader's acceptance of declarations directly
inside Classic lists is therefore leniency, not a sufficient output-validity
test. Declaration placement inside the body of a default node is distinct from
a declaration inserted directly into a node-valued default list.

The official [X3D 4.0 XML schema](https://www.web3d.org/specifications/x3d-4.0.xsd)
allows declarations interspersed with grouping-node child content. Exact XML
ordering can therefore require more than grouping all values by field. JSON
support is assessed against this repository's reader/writer contract; available
[4.0 JSON encoding material](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19776-5v4.0-WD1/Part05/X3D_JSON.html)
is a draft, and the checked public schema index does not establish a finalized
4.0 JSON placement rule. No JSON ISO conformance claim follows from these tests.

The integrated implementation adds weak-parent child statement records and reuses
one reconciliation helper for writers and nested expansion. Body-contained
instances now expand in authored MFNode order, preserving USE aliases and
per-instance independence (`PROTO-CHILD-ORDER`, fixed). Interface-default graph
placement and repeated Classic/JSON field groups remain open under
`PROTO-NESTED-WRITE`. Review also required reflected child reorders/insertions to
survive serialization; declaration and instance anchors remain in the ledger.

The actual CLI now preserves the local `Choice` declaration between `proxy` and
`children` in the Collision reproduction. For a declaration interleaved within
one MFNode field, Classic conversion reports a serialization error (exit 2) and
leaves an existing output file unchanged. JSON round trips remain compatibility
evidence only. The additions review found zero unjustified additions: the new
ledger carries declaration/order information absent from reflected values, and
its shared helper is used by expansion and all four writers.

Verification after integration and mutation correction:

```text
CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci
[test] 767 passed, 12 skipped in 52.13s
[build-ci] 100% tests passed out of 50
[build-ci] 100% tests passed out of 2
[build] 100% tests passed out of 56
[cli-gate-regression] PASS — no regressions (Tier-1 100%; all baseline-PASS items still pass; tier-2/3 executed).
Finished in 131.31s
```

`mise run docs-build` completed successfully. The advisory `mise run docs-drift`
could not connect to `http://localhost:8080/v1/embeddings`; it supplied no drift
assessment. The conformance generator reports 287 findings: 36 open and 251
closed. These counts cover the register, not every requirement in the standard.

### Prototype instances inside interface-default graphs

A Classic reproduction gives `Wrap` an MFNode default containing a Group whose
body declares `Leaf` and initializes `children [ Leaf {} ]`; the outer body
forwards that default using `children IS content`. Before the fix,
`build/x3d convert /tmp/x3d-default-contained.x3dv -f xml -o /tmp/x3d-default-contained-before.x3d`
exited 0 but wrote an empty default Group. The declaration and instance were both
lost. This is a node-body declaration permitted by the published Classic grammar,
not a declaration placed directly in an MFNode list.

[X3D 4.0 prototype semantics](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/concepts.html#PROTOinterfaceDeclarationSemantics)
define instance-local field values and copies of the prototype definition.
The existing clone map already includes ordinary interface-default graphs;
reader context and writer order context were missing for their contained
prototype statements. Reusing the declaration's node statement ledger avoids
another independently maintained graph representation.

A distinct remaining gap is a default whose root is itself a ProtoInstance.
`ProtoField::nodeDefault` currently holds only concrete X3DNode pointers. XML/JSON generic node parsing creates generated statement nodes without runtime
prototype binding; Classic records instances in a temporary Scene
without a field owner. The eventual fix must retain instance identity and its
owning default field, expand it before IS value forwarding, and preserve DEF/USE
aliases across interface fields. Treating it as a direct ProtoBody instance would
incorrectly create a primary or peer node. This remains part of
`PROTO-NESTED-WRITE`; the contained-graph fix does not close that finding.

After integration, the original CLI reproduction retains `ProtoDeclare Leaf`
and `ProtoInstance Leaf` inside the default Group. The repository validator
reports `valid: true` with zero diagnostics (exit 0). The front-door regressions
cover SFNode and MFNode capture, all four writer outputs, JSON re-reading, local
binding and unused declarations; the MFNode case also checks independently
expanded children for two outer instances. The Classic direct-default guard
checks that an unsupported direct instance does not leak into the body.

Final verification:

```text
CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci
[test] 767 passed, 12 skipped in 51.26s
[build-ci] 100% tests passed out of 50
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 2
[cli-gate-regression] PASS — no regressions (Tier-1 100%; all baseline-PASS items still pass; tier-2/3 executed).
Finished in 106.97s
```

The strict docs build succeeded. Advisory docs-drift could not connect to its
local embedding service and provided no assessment. The additions review found
zero unjustified additions: reader context and existing writer context supply
the missing behavior; the local Classic helper prevents a direct default from
being misclassified as body content. No new graph model was introduced.

### Direct prototype-instance defaults

The next reproduction uses `PROTO Wrap [ initializeOnly MFNode content
[ Leaf {} ] ] { Group { children IS content } }`. The actual command
`build/x3d convert /tmp/x3d-direct-default.x3dv -f xml -o /tmp/x3d-direct-default-before.x3d`
exited 0 while serializing an empty `content` field. This is a direct instance
default, distinct from an ordinary default Group containing instances.

[ADR-0056](../decisions/0056-prototype-instance-default-templates.md) records the
representation decision: a declaration-only node wrapper keeps the existing
node-default vector and DEF/USE pointer identity, including aliases across
fields and into body graphs. Materialization belongs to prototype expansion,
with its existing resolver and recursion guard. A separate field-owned instance
list would require another order ledger and an additional alias mechanism.

The direct-default implementation now passes its isolated regressions and full
repository CI. The original CLI reproduction retains `ProtoInstance Leaf`
inside `content` and validates with zero diagnostics. Scene writers now emit
external declarations before local declarations that can depend on them.

A separate concrete XML probe establishes PROTO-NODE-VALUE-INSTANCE: Holder's
SFNode `payload` fieldValue contains `ProtoInstance Leaf` inside Outer's default.
Conversion preserves that XML tag, but a runtime probe following both Collision
proxy fields prints:

```text
Collision
Collision
ProtoInstance
```

The final node should be Leaf's Shape primary. The generated node factory does
contain a concrete ProtoInstance statement node; generic `readNode` therefore
does not necessarily discard the tag. It bypasses runtime prototype binding
and expansion. The CLI also reports Leaf as unused. This corrects the earlier
code-only inventory's assumption that XML/JSON factory lookup would drop the
node. The next fix must cover node-valued instance overrides and caller identity,
not just preserve serialization text.

Final implementation verification:

```text
CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci
[test] 767 passed, 12 skipped in 53.33s
[build-ci] 100% tests passed out of 50
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 2
[cli-gate-regression] PASS — no regressions (Tier-1 100%; all baseline-PASS items still pass; tier-2/3 executed).
Finished in 192.21s
```

The strict docs build succeeded. Advisory docs-drift again could not connect to
its local embedding service and provided no assessment. The additions review
found zero unjustified additions: the template supplies missing node identity,
the materialization traversal supplies expansion context, and shared IS forwarding
replaces duplicated semantics. The register has 289 findings: 37 open and 252
closed. The increase in open findings records the independently reproduced
node-valued instance-override gap; these counts do not measure total ISO coverage.

### Prototype instances in node-valued overrides

Before this change, rerunning `/tmp/x3d-node-value-probe` against the actual
runtime printed `Collision`, `Collision`, `ProtoInstance`. The last node should
be Leaf's Shape primary. Reader interception must cover both direct fieldValue
roots and instances nested inside ordinary fieldValue graphs, while resetting
that capture context at nested prototype declarations.

The existing template representation now also serves node-valued overrides.
Declaration-owned literal values require per-outer cloning. Caller-owned
ordinary nodes instead retain their identity, with template children replaced
by their expanded primaries. An ephemeral context shares materialization across
caller fields and scene-root USE references. Failed external caller templates
remain inert and serializable, with diagnostics, and can be retried in a fresh
transaction. Expanded-instance writers must emit USE for shared named primaries,
rather than repeating DEF and silently creating separate nodes on reparse.

The shared-caller test exposed an independent ordering defect, now tracked as
PROTO-SCENE-ORDER. Actual CLI reproduction:
`build/x3d convert /tmp/x3d-scene-proto-order.x3dv -f xml -o /tmp/x3d-scene-proto-order-before.x3d`
exits 0 but writes `Ordinary`, `First`, `Last` for authored roots `First`,
`Ordinary`, `Last`. `expandScene` appends prototype primaries after existing
ordinary roots. The value-identity regressions therefore identify roots by DEF
and check their membership rather than using an incorrect positional assumption;
restoring source order remains required work.

The integrated runtime now materializes the nested value. After rebuilding the
probe against the current headers and runtime, `/tmp/x3d-node-value-probe`
exited 0 and printed:

```text
Collision
Collision
Shape
```

A further round-trip regression covers a scene-root USE of a prototype instance
defined inside fieldValue. Serialization can emit that shared instance before
its owning outer instance, so readers now register scene-root DEF templates too.
A weak identity link joins that template to its authoritative structural record;
expansion seeds the transaction memo from the structural result. The same
primary serves both the field value and the scene-root alias.

The additions review found zero unjustified additions. Scoped capture state
retains the owning field value without leaking across nested declarations; the
transaction context preserves caller identity and avoids repeated failed
resolution; the weak root link preserves structural-record authority. The tests
exercise runtime node types, pointer identity, failure recovery and four writer
reparses. No persistent expansion cache was added.

Final integrated verification (all commands exited 0 unless noted):

```text
CMAKE_BUILD_PARALLEL_LEVEL=8 mise run ci
[test] 767 passed, 12 skipped in 54.10s
[build-ci] 100% tests passed out of 50
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 2
[cli-gate-regression] PASS — no regressions (Tier-1 100%; all baseline-PASS items still pass; tier-2/3 executed).
Finished in 195.48s
```

`build/x3d validate /tmp/x3d-instance-node-value.x3d --json` reports
`"valid": true`, an empty diagnostics list and `"total": 0`. Converting
`/tmp/x3d-fieldvalue-root-use.x3d` to XML and converting that result again yields
identical files (`diff -u` exits 0); the second output retains the Shared DEF
and Outer's fieldValue USE. The strict `mise run docs-build` completed with
`Documentation built in 5.42 seconds` before this evidence append.
Advisory `mise run docs-drift` exited 1 because its embedding service at
localhost:8080 was unavailable; it provided no drift assessment.

`mise run conformance` reports `290 findings (37 open, 253 closed)`.
PROTO-NODE-VALUE-INSTANCE is fixed; PROTO-SCENE-ORDER remains open alongside
PROTO-NESTED-WRITE and the other requirements gaps. Full X3D 4.0 and native SAI
conformance remains unfinished.

### Prototype placement order in scene graphs

The next batch revalidated PROTO-SCENE-ORDER against the integrated runtime.
`build/x3d convert /tmp/x3d-scene-proto-order.x3dv -f xml -o /tmp/x3d-scene-proto-order-current.x3d`
still emits roots `Ordinary`, `First`, `Last` for authored `First`, `Ordinary`,
`Last`. The same sequence in an ordinary Group.children field, reproduced by
`build/x3d convert /tmp/x3d-scene-proto-child-order.x3dv -f xml -o /tmp/x3d-scene-proto-child-order-before.x3d`,
is also reordered. Both commands exit 0.

This affects behavior, not just source layout. [Core 7.2.2](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/components/core.html)
requires initial binding of the first encountered bindable node and explicitly
allows a locally defined prototype's first node to be that candidate.

The chosen representation extends existing instance templates to the authored
positions of all scene structural instances: roots and ordinary node fields.
The ordered node vectors remain the position authority. Structural instance
records retain source values, and expansion replaces template slots in place.
An expired weak placement identity still identifies a removed authored slot;
it must not be mistaken for a programmatic instance that needs appending.
No additional scene ordering ledger is required.

An executable probe using `parseDocument` followed by
`X3DExecutionContext::buildSceneGraph` confirms the behavioral failure. With
`DEF First VP {}` before `DEF Second Viewpoint {}`, where VP's first node is a
Viewpoint, `/tmp/x3d-proto-order-binding` exits 0 and prints `Second` before the
fix. The expected initially bound DEF is `First`.

While CI ran for placement order, the remaining prototype inventory identified
a requirement previously mentioned only in the fixed PROTO-PRIMARY-ORDER note.
A compiled `parseDocument` probe, `/tmp/x3d-proto-nested-route`, exits 0 with:

```text
authored routes: 1
resolved routes: 0
warnings: 0
```

Its XML scene is reproducible from these declarations and instance:

```xml
<ProtoDeclare name="Inner">
  <ProtoInterface><field name="translation" type="SFVec3f"
    accessType="inputOutput" value="0 0 0"/></ProtoInterface>
  <ProtoBody><Transform><IS><connect nodeField="translation"
    protoField="translation"/></IS></Transform></ProtoBody>
</ProtoDeclare>
<ProtoDeclare name="Outer"><ProtoBody>
  <Group/><ProtoInstance name="Inner" DEF="A"/><Transform DEF="B"/>
  <ROUTE fromNode="A" fromField="translation" toNode="B" toField="translation"/>
</ProtoBody></ProtoDeclare>
<ProtoInstance name="Outer"/>
```

Wrap that content in `<X3D profile="Immersive" version="4.0"><Scene>...</Scene></X3D>`.
The probe inspects Outer's authored `body.routes`, the scene's
`resolvedProtoRoutes`, and document `protoWarnings`. `expandInstance` resolves
body routes before expanding nested instances and only looks in its clone map;
that omits A. PROTO-NESTED-ROUTE now records this open gap independently.
A complete fix also needs interface endpoint mapping and validation, not merely
adding a route count. The pre-resolved route path currently bypasses the scene
bridge's ordinary endpoint mapping logic.

After integration, rebuilding the same binding probe against the current headers
and runtime changes its output to `First` (exit 0). The first CI attempt stopped
on a compiler `No space left on device` error in /tmp. Old generated scratch
build directories were removed. A retry with TMPDIR under the build directory
then hit an evidence-scanner test that excludes paths containing `build`;
verification was restarted with the normal temporary directory and compiler
parallelism reduced to four. Neither failed run is counted as a passing gate.

The additions review found zero unjustified additions. The generalized weak
placement link reuses the existing template and node vectors. The ownership
check distinguishes removed authored slots from never-linked programmatic
records. `instanceAtPlacement` keeps structural source edits authoritative for
raw/failed templates in all four writers, and is only consulted for templates.
No separate scene ordering list or persistent expansion cache was introduced.

The full run subsequently found one obsolete Classic raw-reader assertion:
unexpanded Surf now occupies a root slot before the recovered Shape. The test
now verifies both ordered slots, the exact structural placement link, and the
Shape's retained Box geometry. Its standalone command ends
`ALL CLASSICVRML READER CHECKS PASSED`.

Final integrated verification:

```text
CMAKE_BUILD_PARALLEL_LEVEL=4 mise run ci
[test] 767 passed, 12 skipped in 20.02s
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 50
[build-ci] 100% tests passed out of 2
[cli-gate-regression] PASS — no regressions (Tier-1 100%; all baseline-PASS items still pass; tier-2/3 executed).
Finished in 49.25s
```

The command exits 0. Both original CLI conversions now report the DEF order
`['First', 'Ordinary', 'Last']`, at scene root and in Group.children. The actual
runtime binding probe prints `First`. The strict docs build before this evidence
append completed in 3.46 seconds. Advisory docs-drift could not connect to
localhost:8080 and supplied no assessment.

The regenerated register has 291 findings: 37 open and 254 closed.
PROTO-SCENE-ORDER is fixed. The newly independent PROTO-NESTED-ROUTE finding
records the verified missing route; it is the next prototype execution gap.
Full X3D 4.0 and native SAI conformance remains unfinished.

### Nested prototype routes and interface endpoints

The next batch revalidated `/tmp/x3d-proto-nested-route` against the current
runtime: one authored route, zero resolved routes, and zero warnings. Correct
resolution must happen after nested expansion and must respect the nested
prototype's public interface, including inputOutput event aliases and both
prototype endpoints. The bridge also needs physical field direction/type
validation for pre-resolved routes.

Nested and outer instances can share a concrete primary pointer. Each expansion
therefore captures nested interface mappings locally before publishing its own
outer mapping. Body routes are flattened to concrete endpoints while those
private mappings are available. The outer interface replaces the inner map at
the shared primary, and the bridge must not reinterpret already-concrete body
endpoints through that outer interface. No additional persistent scene cache is
needed for this step.

A separate valid interface-state probe, `/tmp/x3d-proto-unmapped-field`, declares
Relay with an inputOutput SFVec3f `value` and a Group body with no IS connection.
Source.translation_changed routes to RelayNode.set_value; RelayNode.value_changed
routes to Dest.set_translation. Posting Source.translation=(1,2,3) followed by a
context tick should deliver the value through the prototype field. The current
runtime command exits 0 and prints:

```text
routes: 0 rejected: 2 destination: 0,0,0
```

[Field semantics, 4.4.2.2](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/concepts.html)
requires inputOutput fields to emit received values; the interface declaration
itself creates the field. An IS target is not a prerequisite for that field's
existence. PROTO-INTERFACE-STATE records the missing per-instance runtime field
state and event endpoint. Fixing mapped nested routes does not close this gap.

The mapped-route implementation now captures nested interface targets before an
outer instance can overwrite the shared-primary map. Both nested body endpoints
and both scene endpoints can resolve through IS mappings. Side-specific aliases,
nominal interface access/types, and concrete physical endpoints are checked.
Inherited metadata retains its previous primary-node storage; independent
prototype metadata identity is not established by this change. EXTERNPROTO
lookup uses the authored interface, and unrelated native primary fields are not
public prototype endpoints.

The regression fixtures exercise XML and Classic parsing, actual event delivery,
isolation between Outer instances, a nested-first primary, physical and nested IS
event aliases, scene PROTO-to-PROTO routing, inherited metadata, and rejection of
invalid routes. Two older audit fixtures contained invalid ROUTEs: one used an
initializeOnly SFNode source with an MFNode sink, and another used an inputOnly
source while testing an outputOnly sink. They now use valid sources with matching
types while retaining the original identity and sink-rejection assertions.

The candidate's focused commands completed with:

```text
./build/x3d_event_scene_bridge runtime/parse/tests/data/x3dv
all scene-bridge tests passed
./build/x3d_proto_expand_audit
all proto-expand audit tests passed
./build/x3d_parse_tests
[doctest] Status: SUCCESS!
```

Each exits 0; the parse suite reports 64 passed cases and 17,202 passed assertions.
The register now contains 292 findings: 37 open and 255 closed.
PROTO-NESTED-ROUTE is fixed for IS-mapped endpoints. PROTO-INTERFACE-STATE remains
open, as do native SAI prototype support and other previously recorded gaps.
Dynamic Script IS lookup still uses static reflection, and runtime Inline route
installation is separate from the validated pre-resolved bridge path.

Integrated verification (`CMAKE_BUILD_PARALLEL_LEVEL=4 mise run ci`, exit 0):

```text
[conformance-gate] Conformance view OK: committed docs/conformance/ matches sources.
[test] 767 passed, 12 skipped in 28.24s
[build-ci] 100% tests passed out of 50
[build] 100% tests passed out of 56
[build-ci] 100% tests passed out of 2
[cli-gate-regression] PASS — no regressions (all baseline-PASS items still pass).
[cli-gate-regression] PASS — no regressions (Tier-1 100%; all baseline-PASS items still pass; tier-2/3 executed).
Finished in 226.91s
```

The original nested-route probe was rebuilt against the integrated headers and
runtime library. `/tmp/x3d-proto-nested-route` exits 0 and now prints:

```text
authored routes: 1
resolved routes: 1
warnings: 0
```

The separately rebuilt `/tmp/x3d-proto-unmapped-field` still exits 0 with
`routes: 0 rejected: 2 destination: 0,0,0`, confirming the remaining interface-state
gap. The strict docs build before this evidence append completed in 3.18 seconds.
Advisory docs-drift could not connect to its localhost:8080 embedding service and
supplied no assessment. The 37 unfinished findings comprise 15 open and 22 deferred
items; this register is not a measured percentage of specification compliance.

After integrating H-Anim PRs #126 and #127, the regenerated combined register
contains 297 findings: 37 open or deferred and 260 closed. The earlier count
above records the audit branch before those five H-Anim findings were added.

## Interchange reference host acceptance (2026-10-02)

The closeout target is the headless CPU reference host, reading X3D 4.0 XML,
with the stb image decoder and curl asset adapters enabled. Build and exercise
that configuration with `mise run interchange`. `mise run ci-all` supplies the
shared runtime, codec, event, sanitizer and example-consumer gates. This is
implementation acceptance against Annex B of ISO/IEC 19775-1:2023; no external
certification is asserted. Dynamic CLI runs use `--animate --fps 30 --duration 1
--frames-dir <directory>` to attach the standard runtime and advance events;
without `--animate`, the executable exports one static snapshot. The earlier Full-profile audit remains historical
evidence and does not describe the current CPU host in every detail.

| Requirement | Concrete implementation and acceptance evidence |
|---|---|
| Core 1, metadata and field events | Generated nodes and shared reflection/event engine; codec, field-write and ROUTE alias regression suites. `interchange_test` exercises 50-byte DEF names, 30,000-byte SFString, ten 30,000-byte MFStrings, and the numeric MF storage minima through FieldValueIO. |
| Time 1 and Interpolation 2 | Standard runtime TimeSensor and interpolator registration; time-origin, cycle/pause/stop and interpolation regression suites. A capacity case drives a fraction through 1,000 keys with 15,000 coordinates per key and checks the emitted array. |
| Grouping 1 | SceneExtractor walks Group/Transform and updates descendants after events. Acceptance extracts 500 children and retains all output items. UNIT tests cover authored translation, rotation and subsequent writes. |
| Rendering 3, Geometry3D 2 | MeshBuilder plus CPU triangle/line/point paths. Acceptance checks emitted indices at the TriangleSet/fan/strip, indexed triangle/fan/strip, line and point minima, and 5,000 ten-vertex IFS faces with 65,535 coordinate/texture-coordinate entries. Existing geometry tests cover primitive meshes, normals, winding and default UVs. |
| Shape 1 and Lighting 1 | Material descriptors feed unlit/Phong shading, including material-free textured Appearance. Eight authored lights survive alongside the headlight. Ambient emission remains independent of direct light intensity. CPU triangle queries apply shadows, shadowIntensity, castShadow and visibility; rendered assertions include equivalent small/large scenes. The Interchange minimum does not require interactive navigation. |
| Texturing 2 and explicit Annex B nodes | PNG/JPEG fetched bytes and 512×512 PixelTexture with transparent/opaque pixels; all coordinate-generator modes; boundary/filter/mipmap state; per-channel and named UV transforms, including live transform and nested coordinate/generator edits. MultiTexture combines every listed mode and source/function control. Annex B explicitly includes MultiTexture despite the Texturing component-level mismatch, so both the conformance view and CLI include that node exception. |
| Networking 1 / URL fields | CPU host composes confined file, HTTP/HTTPS and FTP adapters with ordered texture URL fallback. Curl backend tests exercise HTTP failure/redirect/address policies and successful passive FTP, including byte limits and case-insensitive schemes. Decode tests use PNG/JPEG bytes with misleading URL extensions. Private network destinations remain an explicit embedder policy opt-in. |
| Navigation 1 / EnvironmentalEffects 1 | Existing bindable stack, Viewpoint projection, NavigationInfo headlight and Background presentation. Background angle UNIT conversion now joins the existing viewpoint/geometry/texture-transform conversions. Optional presentation fields retain shared field-change event handling. |

The headless executable has no interactive navigation controls: it does not use
`NavigationInfo.avatarSize`, `speed` or `type` for user movement, and does not use
`Viewpoint.description` or `WorldInfo.info/title` for a user interface. These are
optional or ignored presentation fields in Annex B; their stored values and
field-change events remain available in the shared runtime. Geometry bbox hints
are not used to override computed mesh bounds.

The CPU uses deterministic Perlin noise and refract-based mapping for generator
modes whose published definitions leave algorithm details unspecified. For the
contradictory MultiTexture entries, REPLACE selects the stage texture and
SELECTARG2 selects the previous result. These interpretations are tested and
recorded in the ledger; they should accompany interoperability reports.

Open global findings TXF-2, REQ-MULTITEXTURE and REQ-SHADOW now describe the
OpenGL PoC's remaining gaps. They do not negate the CPU implementations above.
REQ-UNIT remains open for dimensional fields outside this checked profile and
for writing a scene after runtime normalization while preserving authoring
units. This host acceptance covers reading and presentation; it does not make
an authoring, JSON, ClassicVRML, SAI or Full-profile claim. FTP was incorrectly
classified as Full-only in the earlier audit; Annex B.6 requires it too.

## Interactive reference host acceptance (2026-10-10)

The closeout target is the headless runtime host for Annex C: an X3D 4.0 XML
scene loaded into `RuntimeSession` with the standard runtime and
`SessionOptions::interactive`, driven only through the public input seam
(pointer ray, button and normalized screen position, navigation keys,
key-device events and bind events). The suite is
`runtime/extract/tests/interactive_profile_test.cpp`, run by
`ctest --preset dev -R x3d_extract_tests` and therefore by `mise run ci`. The
Interchange host acceptance above still covers the presentation minima the
two profiles share. This is implementation acceptance against Annex C of
ISO/IEC 19775-1:2023; no external certification is asserted.

| Requirement | Concrete implementation and acceptance evidence |
|---|---|
| Pointing device sensor 1 | TouchSensor under a scaled Transform reports sensor-local hitPoint, hitNormal and hitTexCoord, isOver/isActive, and touchTime only for a release over the geometry; disabled sensors are silent. The lowest sensors on the hit path win and sibling sensors fire together; disabling them hands the hit to the next enabled ancestor. PlaneSensor clamps per axis (unclamped where min > max), reports the unclamped trackPoint, honors axisRotation and stores autoOffset. CylinderSensor covers cylinder and disk drags, minAngle/maxAngle and offset. SphereSensor covers both drag axes and offset. |
| Key device sensor 1 | KeySensor keyPress/keyRelease, actionKeyPress/Release, shiftKey and isActive; disabled sensors ignore input. StringSensor enteredText with deletion, finalText on termination and isActive. |
| Environmental sensor 1 | ProximitySensor under a translated Transform reports enterTime, isActive, sensor-local position_changed and orientation_changed, updates while the viewer moves inside, and sends exitTime on leaving. VisibilitySensor reports a region in view; disabled sensors send nothing. Annex C would also accept an always-visible implementation. |
| Navigation 1 | Viewpoint bind stack with isBound/bindTime, orientation, animated transitions with transitionComplete, and jump FALSE keeping the view. NavigationInfo EXAMINE orbits about centerOfRotation, FLY moves at `speed` along the view, LOOKAT animates toward the picked object and sets centerOfRotation. ANY resolves to EXAMINE, and `NavigationSystem::setForcedMode` is the host's mode switch. |
| Event utilities 1 | A routed TouchSensor → BooleanFilter → BooleanToggle → IntegerTrigger → Switch state machine with BooleanTrigger and TimeTrigger outputs, and TimeSensor-driven Integer/BooleanSequencers that step once per key interval. |
| Networking 2 / Anchor | A click binds the first resolvable `#Viewpoint` url; other urls reach `SessionOptions::anchorHandler` with their parameter list. The headless host loads no replacement world itself. |

Gaps the suite found and this change fixed:

- CylinderSensor rotated the wrong way: dragging from +Z toward +X produced a
  negative rotation about +Y, so routed geometry turned against the pointer and
  the angle clamp acted on the wrong side (IACC-1).
- BooleanToggle computed two TRUE inputs in one cascade from the same stored
  value, and sequencers emitted on every fraction instead of once per key
  interval (IACC-2, IACC-3; carried over from #163).
- A `RuntimeSession` host had no way to receive Anchor activations for
  non-fragment urls (IACC-4).

The same change extends REQ-UNIT to every length and angle field of the
Interactive profile's nodes (bounding boxes, PlaneSensor axisRotation, light
attenuation) and fixes the OpenGL PoC's COORD-EYE and reflection-vector texture
coordinate generation from issue #140.

Interpretations recorded with this acceptance:

- PlaneSensor `translation_changed` and `trackPoint_changed` are expressed in
  the local sensor coordinate system that `axisRotation` creates. §20.4.2
  defines the tracking plane and sign in that system but never names the
  output frame explicitly.
- Light attenuation coefficients convert under UNIT by 1/length (linear) and
  1/length² (quadratic) so the falloff is unchanged at the same physical
  distance. Clause 4.3.6 does not list per-field dimensions.

Optional Annex C fields remain stored and evented without user-interface use:
NavigationInfo `avatarSize`, `speed` (used by FLY) and `visibilityLimit`,
Viewpoint `description` and `retainUserOffsets`. Inline `load` is optional in
the profile and is not part of this acceptance.

## Shader selection and execution (2026-10-10)

Step 3 of the completion order starts with shader execution (REQ-SHADER). The
SDK now selects each Appearance's program and the reference hosts run it.

| Requirement | Concrete implementation and acceptance evidence |
|---|---|
| §31.2.2.3 selection | `ShaderExtract.hpp::selectShader` takes the first ComposedShader or ProgramShader in `Appearance.shaders` whose language is GLSL and whose program is valid. HLSL/Cg candidates and PackagedShader are skipped and stay inert; later candidates are not evaluated. No valid candidate leaves `RenderItem::shaderProgram` empty and the material draws. |
| §31.2.4 / §31.4 sources | Inline CDATA (`ShaderPart` and `ShaderProgram` `sourceCode`), then the first resolving url: `data:` urls (percent-encoded or base64) decode in the SDK, other urls go to `ShaderOptions::resolver` with `AssetKind::Shader`. A Pending url defers validity and the System retries each tick. |
| §31.3.2 outputs | `ShaderSystem` emits `isValid` when a candidate is first evaluated, when its validity changes and after each `activate` TRUE, and `isSelected` when the selection changes. The host's compiler decides validity through `ShaderOptions::validator` (default: a VERTEX and a FRAGMENT part). |
| Live uniforms and edits | Author `<field>` values (SF scalar, vector, colour, rotation and matrix types) are copied onto `ShaderProgramDesc::fields`. Uniform events, url/sourceCode edits and `activate` reach the incremental delta as `updatedMaterial`. |
| Reference hosts | cpu_raster runs the selected fragment stage in its GLSL interpreter and validates with that interpreter (`author_shader_test`: uniform colour on screen, uniform event, fall-through past a program that does not compile, material fallback). The OpenGL PoC validates by compiling and linking in its context and uploads every SF uniform type (`author_shader_gl_test.py`, the same cases under Xvfb). |

Interpretations recorded with this acceptance:

- Edits to `parts`, `programs`, a part's `url`, `load`, `sourceCode` or `type`
  take effect immediately. `activate` TRUE re-resolves urls and re-runs the
  validator, so a host can reload a changed file. §31.3.2 leaves the
  activation conditions to each language binding.
- `isValid` is reported only for candidates the selection evaluated. A shader
  after the selected one in the list sends nothing.

REQ-SHADER-2 records what is still ignored: SFNode texture uniforms and MF
array uniforms carry no value, the PoC links only the vertex and fragment
stages, and the CPU host interprets only the fragment stage.

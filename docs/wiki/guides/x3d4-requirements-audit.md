---
title: X3D 4.0 requirements audit
summary: Baseline requirement inventory, interpretations, and host acceptance scope.
tags: [conformance, audit]
updated: 2026-11-05
related: []
---

# X3D 4.0 requirements audit

This reference keeps the requirement scope of the audit at baseline `a8aefc5`.
Current conformance judgments and acceptance conditions live in
`docs/conformance/findings.yaml`. The baseline is not a release status report
or a certification of the SDK or a complete browser.

The [node inventory](x3d4-requirements-nodes.csv) records the vendored 4.0 UOM's
260 concrete nodes and 4,191 field entries, including inherited entries repeated
on concrete nodes. It is a baseline inventory; its finding associations are not
a live status register. A node without a finding still needs semantic evidence.
The [source manifest](x3d4-requirements-sources.json) retains official URLs,
retrieval dates, and hashes. Source HTML is not vendored.

## Target and claim boundary

The target is published ISO/IEC 19775-1:2023, X3D 4.0, including the
[Full profile](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fullProfile.html).
X3D 4.1 additions are outside this audit.
[Clause 6](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/conformance.html)
distinguishes valid files, generators, and browsers. A browser claim must name
a complete host configuration, including renderer, audio, language engines,
loaders, fonts, and enabled components.

| Deliverable | Acceptance boundary |
|---|---|
| Authoring and codecs | Declared version/profile and encoding; assess conversion loss against the destination grammar. |
| Runtime and extraction | Fields, events, lifecycles, graph changes, geometry, and resource descriptors assigned to the SDK. |
| Complete host | Required resources, languages, graphics, audio, and interaction working together. |

SAI (19775-2), language bindings (19777), and encodings (19776) need separate,
versioned requirements lists. The SAI SDK is described by
[its subsystem page](../subsystems/sai-sdk.md) and
[ADR-0076](../decisions/0076-fold-sai-into-x3d-cpp.md), and native SAI
integration by the [provider subsystem](../subsystems/sai-provider.md).

## Baseline coverage and source reconciliation

The baseline audit separated structural inventory from semantic evidence.
Generated node presence, source parity, parsing success, and detected System
wiring do not establish conformance. The current classifier likewise leaves
behavior unverified unless a finding supplies more specific evidence.

Use the published profile tables when reviewing component levels:

| Profile | Reference-data interpretation |
|---|---|
| [Interchange](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/interchange.html) | Networking 1; explicitly included MultiTexture needs separate treatment from the Texturing level. FTP is required by B.6. |
| [Interactive](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/interactive.html) | EnvironmentalSensor 1, Navigation 1, and EnvironmentalEffects 1. |
| [Immersive](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/immersive.html) | Lighting 2 and EnvironmentalEffects 2; CubeMapTexturing is not listed in D.2. |

The Full appendix and component chapters need reconciliation where level or
node lists disagree. UOM membership is not a replacement for normative prose.
The published TextureProjection chapter uses
[textureProjection.html](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/textureProjection.html).
The Sound chapter's OscillatorSource grammar names `Oscillator`; the UOM names
`OscillatorSource`. ProtoInstance requirements belong to prototype machinery,
so a node-grammar scan alone misses them.

## Component requirements

These are requirement families to audit, not implementation assessments. Full
levels below come from the appendix, subject to the reconciliation above.
Subsystem pages own implementation details; the findings register owns gaps.

| Component / Full level | Normative obligations |
|---|---|
| [Core](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html) / 2 | Unit declarations, field events, metadata, and host capability handling. |
| [Time](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/time.html) / 2 | Activation, pause/resume, loop boundaries, timestamps, and large time jumps. |
| [Networking](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html) / 4 | URL fallback, relative bases, required protocols, Inline replacement, IMPORT/EXPORT, and Anchor activation. |
| [Grouping](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/grouping.html) / 3 | Grouping, child changes, visibility, bounding hints, and DEF/USE placements. |
| [Rendering](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html) / 5 | Geometry attributes, primitive topology, clipping, and minimum capacities. |
| [Shape](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html) / 4 | Material channels, alpha, back materials, FillProperties, and appearance changes. |
| [Geometry3D](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html) / 4 | Indexing, winding, concavity, normals, texture coordinates, bounds, and runtime changes. |
| [Geometry2D](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html) / 2 | Closure, solid and angle options, bounds, and changes for the eight geometry types. |
| [Text](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/text.html) / 1 | Character repertoire, glyph presentation, language, direction, style, layout, and capacities. |
| [Sound](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html) / 3 | Source and processor lifecycles, channel routing, spatialization, destinations, and decoded media. |
| [Lighting](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/lighting.html) / 3 | Light scope, attenuation, material interaction, simultaneous-light capacity, and shadow controls. |
| [Texturing](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html) / 4 | Image and movie resources, samplers, coordinate generation, transforms, and MultiTexture combiners. |
| [Interpolation](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html) / 5 | Interpolation families, endpoints, duplicate keys, output sizes, and numeric tolerances. |
| [PointingDeviceSensor](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html) / 1 | Sensor scope, transformed hits, shared instances, dragging, offsets, and deactivation. |
| [KeyDeviceSensor](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/keyDeviceSensor.html) / 2 | Host key mapping, modifiers, Unicode input, and StringSensor editing. |
| [EnvironmentalSensor](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalSensor.html) / 3 | Transformed and instanced activation regions, exit events, and permitted visibility latitude. |
| [Navigation](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html) / 3 | Navigation modes, transitions, avatar clearance, collision, and viewpoint binding. |
| [EnvironmentalEffects](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html) / 4 | Background resources, fog scope, authored fog coordinates, and presentation. |
| [Geospatial](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html) / 2 | Coordinate conventions, geographic events, tile transitions, and geoid-relative heights. |
| [HAnim](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html) / 3 | Joint and segment deformation, motion, bounds, picking, and the referenced H-Anim edition. |
| [NURBS](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html) / 4 | Curve and surface evaluation, trimming, texture coordinates, weight conventions, and capacities. |
| [DIS](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/dis.html) / 2 | Transport, PDU encoding/decoding, entity mapping, and network-driven events. |
| [Scripting](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/scripting.html) / 1 | Language availability, Script lifecycle, field marshalling, and independently audited language bindings. |
| [EventUtilities](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html) / 1 | Filters, sequencers, triggers, boundary values, timestamps, and ROUTE feedback. |
| [Shaders](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html) / 1 | Program selection, language handling, source loading, outputs, uniforms, and host execution. |
| [CADGeometry](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/CADGeometry.html) / 2 | CAD hierarchy, face placement, visibility, quad geometry, and live changes. |
| [Texturing3D](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texture3D.html) / 2 | Image, pixel, and composed 3D textures; coordinates, transforms, sampling, and updates. |
| [CubeMapTexturing](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalTexturing.html) / 3 | Composed, image, and generated cube maps; coordinate frames, updates, and sampling. |
| [Layering](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layering.html) / 1 | Layer-local binding/navigation, active layer, viewports, composition, and repeated layer order. |
| [Layout](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layout.html) / 2 | Layout rectangles, screen transforms, pixel units, and screen text. |
| [RigidBodyPhysics](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html) / 2 | Collidables, joints, mass models, solver parameters, contacts, and event readback. |
| [Picking](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html) / 3 | Targets, pick geometry, matching, intersections, sorting, and output events. |
| [Followers](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html) / 1 | Initialization, field families, order, tolerance, tau/duration, destinations, and activation. |
| [ParticleSystems](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html) / 3 | Particle lifecycle, emitters, forces, geometry, ramps, and presentation. |
| [VolumeRendering](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html) / 4 | Voxel resources, volume nodes, render-style combinations, and presentation. |
| [TextureProjection](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/textureProjection.html) / 2 | Perspective and parallel projectors, range, scope, transforms, textures, and outputs. |

## Common semantics, encodings, and SAI

Use the applicable editions and separate evidence for these obligations:

| Requirement | Acceptance evidence |
|---|---|
| [Concepts §4](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html) | Identity, contexts, DEF/USE, PROTO, IS, ROUTEs, ordering, fan-out, and event cycles. |
| [Fields §5](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fieldTypes.html) | Types, defaults, access, ranges, and event aliases in every claimed encoding and language binding. |
| Host capabilities | Distinguish document validity from the host's ability to execute and present required features. |
| Encoding contracts | Pin editions and test representable semantics; JSON compatibility tests do not establish a finalized ISO encoding claim. |
| SAI and language bindings | Service-specific assertions and integration evidence; register schema and test-name checks establish consistency only. |
| Optional configuration | Exercise every backend needed by the named claim, including resource wiring and failure policy. |

## Capacity and resource acceptance suite

Derive fixtures from component support tables and the selected profile, including
[Full minimum capacities](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fullProfile.html#t-OtherLimitations).
Check actual stored values, events, and presentation for silent truncation.

| Family | Boundary fixtures to run in the claimed host |
|---|---|
| Geometry | 15,000 coordinates/colors/normals/indices where specified; 5,000 faces with ten vertices each; 16,000 grid heights; 2,500 Extrusion cross-section/spine products. |
| Scene/event scale | 500 children; 1,000 interpolation keys; five nested PROTO definitions and 30 fields per access category; 25 Script fields per category. |
| Presentation | Eight simultaneous lights; six clipping planes; six layers; 512×512 image/pixel textures; 512³ volume data. |
| NURBS | Order 30, 1,500 control points, and ten trimming contours. |
| Text and names | Required Latin-1 repertoire; 100 strings of 100 characters; 50-byte identifiers and the field-specific string/array bounds. |
| Resources | Ten-URL fallback; relative URLs and file/http/ftp; JPEG/PNG, the specified MPEG-1 movie variants and 30-second uncompressed PCM WAV. |
| Field storage | Exercise every SF/MF type at its stated numeric precision and array limit, including matrices, doubles, images and UTF-8 strings. |


Inspect `runtime/RecursionLimits.hpp` alongside the fixtures. Resource limits
may exceed normative minima; their existence alone is not a violation.

## Interpretations and grammar boundaries

H-Anim joint limits and stiffness are application hints under §26.3.3; an IK
solver is not a prerequisite. Reconcile motion evidence with the H-Anim edition
referenced by the selected X3D standard.

Scripting generally permits language choice, but Full table F.3 names Java and
ECMAScript. Shader support requires at least one supported language, not every
listed language. Unsupported-language nodes retain scene/event behavior under
§31.2.4. Review these obligations at the complete-host boundary.

[Architecture §4.3.6](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#Standardunitscoordinates)
supports conversion to initial units for processing. Keeping omitted built-in
defaults canonical while converting authored values is an implementation
interpretation. Do not infer dimensions from arbitrary numeric types or custom
PROTO field names. A custom interface can feed targets with distinct dimensions.
Resolve external UNIT wording separately from runtime conversion evidence.

[Prototype semantics §§4.4.4.3–4.4.4.4](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#PROTODefinitionSemantics)
require preserving primary/peer order and instance identity. An unresolved first
instance must not promote a later peer into the primary position.
[Interface semantics §4.4.4.2](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#PROTOInterfaceDeclarationSemantics)
require per-instance interface state; an IS target is not a prerequisite for an
interface field to exist. PROTO and EXTERNPROTO inherit `metadata`; interface
redeclaration is forbidden by §§4.4.4.2 and 4.4.5.2.

[Interface-default semantics](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/concepts.html#PROTOinterfaceDeclarationSemantics)
require independent copies per instance while retaining aliases within one
instance. [Field semantics §4.4.2.2](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/concepts.html)
require inputOutput fields to emit received values.
[Core §7.2.2](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/components/core.html)
initially binds the first encountered bindable node, including a local
prototype's primary node.

The published
[ClassicVRML Annex A](https://www.web3d.org/documents/specifications/19776-2/V3.3/Part02/grammar.html)
and [4.0 Classic draft](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19776-2v4.0-CD/Part02/grammar.html)
distinguish declarations in node bodies from node statements in MFNode lists.
Reader leniency does not make declarations directly inside those lists valid
output. The [4.0 XML schema](https://www.web3d.org/specifications/x3d-4.0.xsd)
permits declarations interspersed with grouping content. Review JSON placement
against the [JSON draft](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19776-5v4.0-WD1/Part05/X3D_JSON.html)
separately. Cross-encoding loss needs a grammar-specific explanation.

NURBS weight conventions must be named in numeric fixtures; peer-browser
agreement alone does not resolve ambiguous normative text. Geoid-relative
heights require an actual geoid model in the tested configuration.
Viewport clipping and viewport remapping are distinct obligations; see
[ADR-0035](../decisions/0035-viewport-clipboundary-glviewport.md).

<a id="interchange-reference-host-acceptance-2026-10-02"></a>
## Interchange reference host acceptance

The Annex B acceptance configuration is the headless CPU reference host,
reading X3D 4.0 XML with stb image decoding and curl asset adapters.
`mise run interchange` builds and exercises it. The executable exports a static
snapshot unless `--animate` is supplied; animation uses `--fps`, `--duration`,
and `--frames-dir` to drive runtime frames.

`examples/cpu_raster/tests/interchange_test.cpp` checks grouping, lighting,
geometry and field-storage capacities, texturing, resource behavior, and
rendered results. Shared runtime and codec suites provide additional evidence.
Inspect the assertions and required configuration before extending this scope.

The acceptance boundary is reading and presentation. It does not establish
Full-profile, authoring, JSON, ClassicVRML, or SAI conformance. Optional UI fields
can remain stored and evented without a user interface. Computed mesh bounds
are independent of authored bbox hints.

The CPU host uses deterministic noise and refract-based coordinate generation
where published algorithms are unspecified. For contradictory MultiTexture
entries, REPLACE selects the stage texture and SELECTARG2 selects the previous
result. Keep those interpretations with interoperability reports.

<a id="interactive-reference-host-acceptance-2026-10-10"></a>
## Interactive reference host acceptance

The Annex C configuration uses `RuntimeSession`, the standard runtime, and
`SessionOptions::interactive`. The public input seam supplies pointer rays,
buttons, normalized screen positions, navigation keys, key-device events, and
bind events. Presentation minima shared with Interchange use the CPU host
configuration above.

`runtime/extract/tests/interactive_profile_test.cpp` checks sensor-local hits,
Touch/Plane/Cylinder/Sphere sensors, key input, proximity and visibility events,
viewpoint binding and transitions, EXAMINE/FLY/LOOKAT navigation, routed event
utilities, and Anchor activation. Run its grouped suite with:

```sh
ctest --preset dev -R x3d_extract_tests --output-on-failure
```

Non-fragment Anchor URLs reach `SessionOptions::anchorHandler`; the session
does not load a replacement world. Inline loading is outside this acceptance.

PlaneSensor translation and track-point outputs use the sensor frame created
by `axisRotation`. §20.4.2 defines the plane and sign without explicitly naming
the output frame. Light attenuation coefficients convert by inverse length
and inverse length squared, preserving falloff at the same physical distance;
§4.3.6 does not list per-field dimensions.

<a id="shader-selection-and-execution-2026-10-10"></a>
## Shader selection and execution

`ShaderExtract.hpp::selectShader` evaluates Appearance candidates in order and
selects the first supported valid GLSL ComposedShader or ProgramShader.
Sources use inline `sourceCode`, then resolving URLs; data URLs decode locally
and external URLs use the shader resolver. A host validator determines actual
compiler acceptance; the default validator checks stage structure only.

`ShaderSystem` supplies `isValid`, `isSelected`, and `activate` behavior.
The CPU host validates and interprets fragment programs; the OpenGL host
compiles and links programs in its context. Their author-shader tests exercise
rendered output, uniform changes, failed-program fall-through, and fallback.
[Shaders](../subsystems/shaders.md) owns the execution and binding details;
remaining obligations live in the findings register.

Edits to parts, programs, URL, load, sourceCode, or type trigger reevaluation.
`activate` TRUE re-resolves and revalidates; §31.3.2 leaves activation conditions
to the language binding. Candidates after the selected shader are not evaluated
and emit no validity result.

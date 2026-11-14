---
title: SDK capabilities
summary: Index of the current SDK surface, one row per capability, with the page that states its contract.
tags: [reference, sdk, capabilities]
updated: 2026-11-14
related:
  - native-host-contract.md
  - seam-matrix.md
---

# SDK capabilities

This is an index of the current SDK surface. Subsystem pages describe contracts
and configuration; the findings register (`docs/conformance/findings.yaml`) owns
conformance status and remaining gaps. Availability does not imply complete
component or profile conformance.

The render feed is experimental. See the
[native host contract](native-host-contract.md) for ownership, mutation,
resource, and identity rules.

| Capability | Current surface | Reference |
|---|---|---|
| Parsing | XML, ClassicVRML, VRML97, JSON, and gzip input. | [Parsing](subsystems/parse-readers.md) |
| Document model and bindings | Typed X3D 4.0 nodes, fields, and reflection. | [Document model and bindings](subsystems/generated-bindings.md) |
| Node inspection | `sdk::forEachNode` visits each reachable node once; `sdk::readField<T>` returns an owned copy of a named field. | [SDK facade](subsystems/sdk-facade.md) |
| Scene authoring | `SceneBuilder` creates nodes by C++ type or X3D type name, attaches children by containerField, sets fields from text or typed values, and adds ROUTEs, rejecting unknown fields, malformed values, and ROUTEs with the wrong direction or type. Child node types and value ranges are not checked by the builder. | [Asset conversion](subsystems/asset-import.md#scenebuilder) |
| Serialization | XML, canonical XML, JSON, and ClassicVRML writers. | [Serialization](subsystems/codecs-writers.md) |
| Prototype expansion | Local and resolved external declarations, instance fields, IS connections, and nested routes. | [Prototype expansion](subsystems/proto-expand.md) |
| Inline composition | Parse-time expansion, IMPORT/EXPORT, and resolver-driven runtime replacement. Asset-loaded deferred Inlines keep source ancestry to bound self and mutual cycles, and fetched documents forward their reader, range, and expansion warnings with source context. | [Inline composition](subsystems/inline-expand.md) |
| Execution | Host-driven ticks, routes, event cascades, dirty tracking, and optional per-phase tick timings. | [Execution](subsystems/execution-context.md) |
| Time and media lifecycle | TimeSensor and shared start/stop/pause/resume handling. | [Time and media lifecycle](subsystems/system-time.md) |
| Interpolation | Scalar, vector, color, orientation, spline, geospatial, and NURBS systems. | [Interpolation](subsystems/system-interpolators.md) |
| Event utilities | Filters, toggles, sequencers, and triggers. | [Event utilities](subsystems/system-eventutility.md) |
| Followers | Chasers and dampers registered through the standard runtime. | [Followers](subsystems/system-followers.md) |
| Navigation | EXAMINE, FLY, WALK, LOOKAT, NONE, host head pose, an absolute viewer world-pose override, and a default viewer at the world origin when no Viewpoint is bound. | [Navigation](subsystems/system-navigation.md) |
| Input and sensors | Pointer, drag, key-device, environmental, and load-state systems. Pointing-device sensors can be installed without navigation for hosts that own navigation. | [Input and sensors](subsystems/sensors.md) |
| Extraction | Per-path render items, shared meshes, packed output, and incremental deltas. Switch and LOD selection changes keep unaffected placement IDs and GeomIds. | [Extraction pipeline](subsystems/extract.md) |
| glTF export | `sdk::writeGlb` and `x3d extract -o out.glb` write extracted items as binary glTF 2.0: world-matrix nodes, shared meshes, lines and points, PBR/unlit materials, textures (PNG/JPEG files embedded through a host read callback or referenced by url, PixelTextures and host-decoded images embedded as PNG), lights as `KHR_lights_punctual`, viewpoints as cameras, and Position/OrientationInterpolator animation of Transforms. No skins and no other animation; movie, multi, cube and 3D textures are dropped. | [glTF export](subsystems/gltf-export.md) |
| Materials and lights | Phong, Physical, and Unlit descriptors with scoped lights and render state. | [Materials and lights](subsystems/materials.md) |
| Textures | Texture descriptors, multi-UV data, cube-map images, host decoding, and base-URL-aware resolver callbacks. | [Textures](subsystems/extract-textures.md) |
| Text | Font metrics callback, layout, glyph meshes, and optional stb atlas. | [Text](subsystems/extract-text.md) |
| Author shaders | GLSL selection, resolved stage sources, and scalar, array, and texture field bindings. | [Author shaders](subsystems/shaders.md) |
| Geospatial | Context-owned projection, geospatial nodes, and resolver-driven GeoLOD tiles. | [Geospatial](subsystems/geospatial.md) |
| H-Anim | Skin bindings, pose updates, displacers, and motion playback. | [H-Anim](subsystems/hanim.md) |
| Audio | AudioBackend interface and dependency-free BuiltinDsp implementation. | [Audio](subsystems/sound.md) |
| Physics | PhysicsBackend interface and optional Jolt implementation. | [Physics](subsystems/physics.md) |
| Asset loading | Host byte resolution with file and optional libcurl adapters, and resolver callbacks that receive the requesting node's source base URL. | [Asset loading](subsystems/system-asset-io.md) |
| Image decoding | Optional stb and Wuffs implementations of TextureResolver. | [Image decoding](subsystems/system-texture-decode.md) |
| Script execution | ScriptEngine interface, Duktape implementation, optional QuickJS comparison engine, bounded script heaps, and length-preserving string fields and scene creation. | [Script execution](subsystems/system-script-sai.md) |
| SAI SDK | Portable C++20 SAI provider contract (`sai/`) with a reference backend, reference-only kernel extensions and its own UOM-driven generator; builds and installs separately from the runtime. | [SAI SDK](../sai/index.md) |
| Native SAI provider | Opt-in portable service over a bounded native node subset, with named-node discovery, presentation materials and deltas, session adoption, and queued live ROUTE addition. | [Native SAI provider](../sai/reference/native-provider.md) |
| Asset conversion | ImportSource backends, X3D emission, and consumer texture processing. | [Asset conversion](subsystems/asset-import.md) |
| Extensions | Opt-in foreign geometry support and nonstandard physical-room and tracked-device sensors behind a separate header and target. | [Extensions](subsystems/ext-firewall.md) |

## Related

- [Seam matrix](seam-matrix.md)

# SDK capabilities

This is an index of the current SDK surface. Subsystem pages describe contracts
and configuration; the [findings register](../conformance/findings.yaml) owns
conformance status and remaining gaps. Availability does not imply complete
component or profile conformance.

The render feed is experimental. See the
[native host contract](../wiki/guides/native-host.md) for ownership, mutation,
resource, and identity rules.

| Capability | Current surface | Reference |
|---|---|---|
| Parsing | XML, ClassicVRML, VRML97, JSON, and gzip input. | [Parsing](../wiki/subsystems/parse-readers.md) |
| Document model and bindings | Typed X3D 4.0 nodes, fields, and reflection. | [Document model and bindings](../wiki/subsystems/generated-bindings.md) |
| Node inspection | `sdk::forEachNode` visits each reachable node once; `sdk::readField<T>` returns an owned copy of a named field. | [SDK facade](../wiki/subsystems/sdk-facade.md) |
| Serialization | XML, canonical XML, JSON, and ClassicVRML writers. | [Serialization](../wiki/subsystems/codecs-writers.md) |
| Prototype expansion | Local and resolved external declarations, instance fields, IS connections, and nested routes. | [Prototype expansion](../wiki/subsystems/proto-expand.md) |
| Inline composition | Parse-time expansion, IMPORT/EXPORT, and resolver-driven runtime replacement. Asset-loaded deferred Inlines keep source ancestry to bound self and mutual cycles, and fetched documents forward their reader, range, and expansion warnings with source context. | [Inline composition](../wiki/subsystems/inline-expand.md) |
| Execution | Host-driven ticks, routes, event cascades, dirty tracking, and optional per-phase tick timings. | [Execution](../wiki/subsystems/execution-context.md) |
| Time and media lifecycle | TimeSensor and shared start/stop/pause/resume handling. | [Time and media lifecycle](../wiki/subsystems/system-time.md) |
| Interpolation | Scalar, vector, color, orientation, spline, geospatial, and NURBS systems. | [Interpolation](../wiki/subsystems/system-interpolators.md) |
| Event utilities | Filters, toggles, sequencers, and triggers. | [Event utilities](../wiki/subsystems/system-eventutility.md) |
| Followers | Chasers and dampers registered through the standard runtime. | [Followers](../wiki/subsystems/system-followers.md) |
| Navigation | EXAMINE, FLY, WALK, LOOKAT, NONE, host head pose, an absolute viewer world-pose override, and a default viewer at the world origin when no Viewpoint is bound. | [Navigation](../wiki/subsystems/system-navigation.md) |
| Input and sensors | Pointer, drag, key-device, environmental, and load-state systems. Pointing-device sensors can be installed without navigation for hosts that own navigation. | [Input and sensors](../wiki/subsystems/sensors.md) |
| Extraction | Per-path render items, shared meshes, packed output, and incremental deltas. Switch and LOD selection changes keep unaffected placement IDs and GeomIds. | [Extraction](../wiki/subsystems/extract.md) |
| Materials and lights | Phong, Physical, and Unlit descriptors with scoped lights and render state. | [Materials and lights](../wiki/subsystems/materials.md) |
| Textures | Texture descriptors, multi-UV data, cube-map images, host decoding, and base-URL-aware resolver callbacks. | [Textures](../wiki/subsystems/extract-textures.md) |
| Text | Font metrics callback, layout, glyph meshes, and optional stb atlas. | [Text](../wiki/subsystems/extract-text.md) |
| Author shaders | GLSL selection, resolved stage sources, and scalar, array, and texture field bindings. | [Author shaders](../wiki/subsystems/shaders.md) |
| Geospatial | Context-owned projection, geospatial nodes, and resolver-driven GeoLOD tiles. | [Geospatial](../wiki/subsystems/geospatial.md) |
| H-Anim | Skin bindings, pose updates, displacers, and motion playback. | [H-Anim](../wiki/subsystems/hanim.md) |
| Audio | AudioBackend interface and dependency-free BuiltinDsp implementation. | [Audio](../wiki/subsystems/sound.md) |
| Physics | PhysicsBackend interface and optional Jolt implementation. | [Physics](../wiki/subsystems/physics.md) |
| Asset loading | Host byte resolution with file and optional libcurl adapters, and resolver callbacks that receive the requesting node's source base URL. | [Asset loading](../wiki/subsystems/system-asset-io.md) |
| Image decoding | Optional stb and Wuffs implementations of TextureResolver. | [Image decoding](../wiki/subsystems/system-texture-decode.md) |
| Script execution | ScriptEngine interface, Duktape implementation, and optional QuickJS comparison engine. | [Script execution](../wiki/subsystems/system-script-sai.md) |
| Native SAI provider | Opt-in portable service over a bounded native node subset, with named-node discovery, presentation materials and deltas, session adoption, and queued live ROUTE addition. | [Native SAI provider](../wiki/subsystems/sai-provider.md) |
| Asset conversion | ImportSource backends, X3D emission, and consumer texture processing. | [Asset conversion](../wiki/subsystems/asset-import.md) |
| Extensions | Opt-in foreign geometry support and nonstandard physical-room and tracked-device sensors behind a separate header and target. | [Extensions](../wiki/subsystems/ext-firewall.md) |

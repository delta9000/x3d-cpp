// ─── include/x3d/sdk.hpp ─────────────────────────────────────────────────────
// Umbrella façade for the x3d-cpp-gen headless SDK (v1).
//
// Include this ONE header; link the CMake target x3d_cpp::sdk. Everything an
// embedder needs is re-exported into namespace `x3d::sdk`:
//
//   • Load     — parse any of the 4 encodings (+ gzip, PROTO, lenient read) into
//                an X3DDocument with conformance/range diagnostics.
//   • Runtime  — an X3DExecutionContext you tick once per frame; input setters,
//                event injection, behavior/Script systems, a pull surface.
//   • Extract  — a SceneExtractor that turns the live graph into renderer-ready
//                RenderItems (meshes/materials/lights/camera/background) + deltas.
//   • Seams    — embedder-supplied IO callbacks: AssetResolver, TextureResolver,
//                FontMetrics, GeoProjection, ScriptEngine. The SDK does NO file
//                IO, decoding, or rasterization — that stays in the embedder.
//
// Stability: symbols are marked [STABLE] (frozen pre-v2, breaking change = major)
// or [EXPERIMENTAL] (shape may evolve; embedder wiring still maturing). See
// docs/sdk/README.md for the capability matrix and the post-v1 exclusion list.
//
// Canonical usage:
//   #include "x3d/sdk.hpp"
//   namespace sdk = x3d::sdk;
//   sdk::X3DDocument doc = sdk::parseFile("scene.x3dv");
//   sdk::X3DExecutionContext ctx;
//   ctx.buildSceneGraph(doc.scene);
//   ctx.buildFrom(doc.scene);
//   sdk::SceneExtractor ex(ctx, doc.scene);
//   sdk::RenderDelta f0 = ex.fullSnapshot();          // upload f0.added
//   while (running) { ctx.tick(now); auto d = ex.delta(); /* apply d */ }
// ─────────────────────────────────────────────────────────────────────────────
#ifndef X3D_SDK_HPP
#define X3D_SDK_HPP

// Implementation headers (all reachable bare-name via the x3d_cpp include path).
#include "X3DParse.hpp"            // x3d::codec   — parseFile/parseDocument/Encoding
#include "X3DCodecs.hpp"           // x3d::codec   — XmlWriter/JsonWriter/VrmlWriter (serialize)
#include "X3DRuntime.hpp"          // x3d::runtime — X3DDocument/Scene/Head/Route/Profile
#include "X3DExecutionContext.hpp" // x3d::runtime — X3DExecutionContext
#include "X3DSceneBridge.hpp"      // x3d::runtime — BridgeResult/RouteError
#include "X3DSystem.hpp"           // x3d::runtime — System (abstract)
#include "DirtyTracker.hpp"        // x3d::runtime — DirtyTracker
#include "PickSystem.hpp"          // x3d::runtime — PickResult
#include "Ray.hpp"                 // x3d::runtime — Ray
#include "Mat4.hpp"                // x3d::runtime — Mat4
#include "Aabb.hpp"                // x3d::runtime — Aabb
#include "SceneExtractor.hpp"      // x3d::runtime::extract — SceneExtractor
#include "RenderItem.hpp"          // x3d::runtime::extract — descriptor POD
#include "MeshBuilder.hpp"         // x3d::runtime::extract — MeshBuildOptions/GeoProjection
#include "AssetResolver.hpp"       // x3d::runtime::extract — AssetResolver seam   [STABLE] (proven generic: libcurl + S3; see ADR-0023)
#include "TextureResolver.hpp"     // x3d::runtime::extract — TextureResolver seam [STABLE] (proven generic: stb_image + wuffs; see ADR-0024)
#include "FontMetrics.hpp"         // x3d::runtime::extract — FontMetrics seam     [STABLE] (proven generic: stb_truetype + FreeType; see ADR-0025)
#include "ScriptEngine.hpp"        // x3d::runtime — ScriptEngine (abstract)       [STABLE]
#include "ScriptSystem.hpp"        // x3d::runtime — ScriptSystem                  [STABLE]
#include "SaiContext.hpp"          // x3d::runtime — SaiContext                    [STABLE]

/// The curated v1 embedder surface. Everything here is supported; internals are
/// intentionally NOT pulled in. Prefer writing `x3d::sdk::Foo`.
namespace x3d::sdk {

// ── Loading ──────────────────────────────────────────────────────────── [STABLE]
using x3d::codec::parseFile;             ///< X3DDocument parseFile(const std::string& path)
using x3d::codec::parseDocument;         ///< X3DDocument parseDocument(text, hint={}, base={}, resolver=...)
using x3d::codec::Encoding;              ///< enum class { XML, ClassicVRML, VRML97, JSON, Unknown }
using x3d::codec::localFileProtoResolver;///< default file-local EXTERNPROTO resolver (cycle-safe)
using x3d::codec::ProtoDeclarationResolver; ///< the resolver callback type for parseDocument

using x3d::runtime::X3DDocument;         ///< { version, profile, head, scene, rangeWarnings, protoWarnings }
using x3d::runtime::Scene;               ///< { rootNodes, defs, routes, protoDeclarations, imports/exports, ... }
using x3d::runtime::Profile;             ///< enum class { Core, Interchange, ..., Full }
using x3d::runtime::Head;                ///< { components, units, meta }
using x3d::runtime::Component;           ///< { name, level }
using x3d::runtime::Unit;                ///< { category, name, conversionFactor }
using x3d::runtime::Meta;                ///< { name, content }
using x3d::runtime::Route;               ///< { fromNode, fromField, toNode, toField } (DEF-name strings)
using x3d::nodes::X3DNode;               ///< generated node base (fields(), get/set, getDEF/setDEF) — x3d::nodes
using x3d::core::RangeDiagnostic;        ///< one out-of-range value kept by the lenient read path — x3d::core
using x3d::runtime::ProtoWarning;        ///< a PROTO/EXTERNPROTO expansion diagnostic

// Serialization — reflection-driven writers (writeDocument(doc) -> std::string).
using x3d::codec::XmlWriter;             ///< X3D-XML serializer
using x3d::codec::JsonWriter;            ///< X3D-JSON serializer
using x3d::codec::VrmlWriter;            ///< ClassicVRML serializer
using x3d::codec::CanonicalXmlWriter;    ///< X3D Canonical Form (X3DC14N) serializer

// ── Execution context (runtime / tick) ───────────────────────────────── [STABLE]
// Construct one per loaded document, default-constructed. Setup order:
//   ctx.buildSceneGraph(doc.scene);   // index transforms/bindings/pick tree
//   ctx.buildFrom(doc.scene);         // resolve DEF-named ROUTEs + IS redirects
//   ctx.addSystem(...) / ctx.addScriptSystem(...);  // before the first tick()
// Then once per frame: set inputs, ctx.tick(now), read the pull surface.
using x3d::runtime::X3DExecutionContext;
using x3d::runtime::BridgeResult;        ///< { routesAdded, rejected[] } returned by buildFrom
using x3d::runtime::RouteError;          ///< { index, reason } a single rejected ROUTE
using x3d::runtime::System;              ///< abstract behavior System: attach() + update()
using x3d::runtime::DirtyTracker;        ///< per-tick change set (read via ctx.dirtyTracker())
using x3d::runtime::PickResult;          ///< result of ctx.pick(ray)
using x3d::runtime::Ray;                 ///< { origin, direction, pointAt(t) }
using x3d::runtime::Mat4;                ///< column-major 4×4
using x3d::runtime::Aabb;                ///< axis-aligned bounds

// ── Extraction / render feed ─────────────────────────────────────────── [STABLE]
using x3d::runtime::extract::SceneExtractor; ///< fullSnapshot()/delta()/item()/camera()/lights()/...
using x3d::runtime::extract::RenderItem;     ///< { path, worldTransform, geometry, material, mesh, lights, ... }
using x3d::runtime::extract::RenderDelta;    ///< { added/removed/updatedTransform/Geometry/Material, *Changed }
using x3d::runtime::extract::RenderItemId;   ///< std::uint32_t dense id
using x3d::runtime::extract::kInvalidRenderItemId;
using x3d::runtime::extract::PathKey;        ///< vector<const X3DNode*> root..leaf chain
using x3d::runtime::extract::PathKeyHash;
using x3d::runtime::extract::PathKeyEqual;
using x3d::runtime::extract::GeomId;         ///< { node*, contentVersion } GPU cache key
using x3d::runtime::extract::GeomIdHash;
using x3d::runtime::extract::MeshData;       ///< positions/indices/normals/texcoords/colors/topology/...
using x3d::runtime::extract::Topology;       ///< enum class { Triangles, Lines, Points }
using x3d::runtime::extract::MaterialDesc;   ///< model/baseColor/metallic/roughness/.../textures[] + toRGBA()
using x3d::runtime::extract::MaterialModel;  ///< enum class { Phong, Physical, Unlit }
using x3d::runtime::extract::AlphaMode;      ///< enum class { Opaque, Mask, Blend }
using x3d::runtime::extract::LightDesc;      ///< Directional/Point/Spot light descriptor
using x3d::runtime::extract::CameraDesc;     ///< { viewMatrix, fieldOfView, near/far, ortho... }
using x3d::runtime::extract::BackgroundDesc; ///< { skyColor/skyAngle/groundColor/groundAngle }

// ── Seams (embedder-supplied IO) ─────────────────────────────────── [EXPERIMENTAL]
// Shapes are usable but may gain fields; embedder wiring is still maturing.

// Tessellation density + geodesy + font-metrics build knobs.
using x3d::runtime::extract::MeshBuildOptions; ///< { sphereRings, sphereSegments, radialSlices, geoProjection, fontMetrics }

// Asset resolution seam (B8). One type, two invocation contracts:
//   render-time may return Pending (retry next frame); parse-time (Inline/
//   ExternProto) must return Ready/Failed synchronously.
using x3d::runtime::extract::AssetResolver;  ///< function<AssetResult(url, AssetKind)>
using x3d::runtime::extract::AssetResult;    ///< { status, bytes } + makeReady/makePending/makeFailed
using x3d::runtime::extract::AssetStatus;    ///< enum class { Ready, Pending, Failed }
using x3d::runtime::extract::AssetKind;      ///< enum class { Texture, Movie, Inline, ExternProto, ExternalGeometry }

// Texture decode seam (T-TEX) — [STABLE] (proven generic: stb_image + wuffs,
// byte-equal decode swap-test; ADR-0024). Embedder maps a url to decoded RGBA
// pixels; the SDK threads the result onto TextureRef so a consumer binds without
// re-resolving. makeMultiFormatTextureResolver composes per-format decoders.
using x3d::runtime::extract::TextureResolver;      ///< function<TexturePixelResult(const std::string& url)>
using x3d::runtime::extract::TexturePixelResult;   ///< { status, pixels }
using x3d::runtime::extract::TexturePixels;        ///< { width, height, RGBA bytes }
using x3d::runtime::extract::TextureResolveStatus; ///< enum class { Ready, Pending, Failed }
using x3d::runtime::extract::TextureRef;           ///< per-channel texture descriptor (url/repeat/sampler/...)
using x3d::runtime::extract::SamplerParams;        ///< descriptor-level sampler state

// FontMetrics seam (T-TEXT) — [STABLE] (proven generic: stb_truetype + FreeType,
// exact-equal advanceEm swap-test; see ADR-0025).
// Embedder supplies per-codepoint advance + optional atlas UV / outline; the SDK
// does all Text layout. Default = monospaced stub.
using x3d::runtime::extract::FontMetrics;     ///< function<GlyphResult(const FontKey&)>
using x3d::runtime::extract::FontKey;         ///< { family, style, codepoint, ... }
using x3d::runtime::extract::GlyphMetrics;    ///< advance + atlas/outline data
using x3d::runtime::extract::GlyphResult;     ///< { status, metrics }
using x3d::runtime::extract::GlyphStatus;     ///< enum class { Ready, Pending, Failed }
using x3d::runtime::extract::makeMonospaceStub; ///< default monospaced FontMetrics (advanceEm=0.6)

// Geo-projection seam (B5). Supply via MeshBuildOptions::geoProjection; empty =>
// flat-fallback (geographically unanchored grid).
using x3d::runtime::extract::GeoProjection;   ///< function<SFVec3f(SFVec3d geoCoord, double elev, GeoSystemDesc)>
using x3d::runtime::extract::GeoSystemDesc;   ///< { geoSystem, geoGridOrigin }

// ── Script / SAI seam (T-SCRIPT) ─────────────────────────────────────── [STABLE]
// Frozen pre-v2: the ScriptEngine abstract interface carried two independent
// backends — Duktape (EcmaScriptBackend) and QuickJS (QuickJsBackend, behind
// X3D_CPP_BUILD_QUICKJS) — with NO signature change, proven by the x3d_quickjs_swap
// behavioral parity test (genericity proof; see ADR-0022 + docs/wiki/seam-status.md).
// The whole surface (ScriptEngine / ScriptSystem / SaiContext) is part of the same
// frozen seam. Implement ScriptEngine, construct a ScriptSystem, then
// ctx.addScriptSystem(ss). SaiContext is the backend↔runtime channel.
using x3d::runtime::ScriptEngine;             ///< abstract: load/initialize/shutdown/prepareEvents/invoke/...
using x3d::runtime::ScriptSystem;             ///< System subclass; ctor(engine, browserName, version)
using x3d::runtime::SaiContext;               ///< getField/setField/addRoute/deleteRoute/getName/getVersion
using x3d::runtime::ScriptHandle;             ///< std::uint64_t; 0 == kInvalidScriptHandle
using x3d::runtime::kInvalidScriptHandle;

} // namespace x3d::sdk

#endif // X3D_SDK_HPP

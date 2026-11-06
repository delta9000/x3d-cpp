// Include this header and link x3d_cpp::sdk for the headless SDK surface.
// Simulation and extraction perform no resource I/O; embedders supply callbacks.
// parseFile() and the default Inline/EXTERNPROTO resolvers can read local files.
// Inject both parse resolvers for a fully host-controlled resource boundary.
// See docs/wiki/seam-status.md for interface stability.
//
// Canonical usage:
//   #include "x3d/sdk.hpp"
//   namespace sdk = x3d::sdk;
//   sdk::X3DDocument doc = sdk::parseFile("scene.x3dv");
//   sdk::X3DExecutionContext ctx;
//   ctx.buildSceneGraph(doc.scene);
//   ctx.buildFrom(doc.scene);
//   x3d::runtime::attachStandardRuntime(doc.scene, ctx);
//   sdk::SceneExtractor ex(ctx, doc.scene);
//   sdk::RenderDelta f0 = ex.fullSnapshot();          // upload f0.added
//   // each frame, with `now` = seconds since start (any monotonic clock):
//   while (running) { ctx.tick(now); auto d = ex.delta(); /* apply d */ }
#ifndef X3D_SDK_HPP
#define X3D_SDK_HPP

// Implementation headers (all reachable bare-name via the x3d_cpp include path).
#include "X3DParse.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"
#include "X3DSystem.hpp"
#include "DirtyTracker.hpp"
#include "PickSystem.hpp"
#include "Ray.hpp"
#include "Mat4.hpp"
#include "Aabb.hpp"
#include "RuntimeSession.hpp"
#include "SceneExtractor.hpp"
#include "RenderItem.hpp"
#include "MeshBuilder.hpp"
#include "AssetResolver.hpp"
#include "TextureResolver.hpp"
#include "FontMetrics.hpp"
#include "ScriptEngine.hpp"
#include "ScriptSystem.hpp"
#include "SaiContext.hpp"

namespace x3d::sdk {

// Loading
using x3d::codec::parseFile;
using x3d::codec::parseDocument;
using x3d::codec::Encoding;
using x3d::codec::localFileProtoResolver; ///< default file-local EXTERNPROTO resolver (cycle-safe)
using x3d::codec::ProtoDeclarationResolver;

using x3d::runtime::X3DDocument;
using x3d::runtime::Scene;
using x3d::runtime::Profile;
using x3d::runtime::Head;
using x3d::runtime::Component;
using x3d::runtime::Unit;
using x3d::runtime::Meta;
using x3d::runtime::Route; ///< endpoints use DEF-name strings
using x3d::nodes::X3DNode;
using x3d::core::RangeDiagnostic; ///< out-of-range value retained by lenient reads
using x3d::runtime::ProtoWarning;

// Serialization — reflection-driven writers (writeDocument(doc) -> std::string).
using x3d::codec::XmlWriter;
using x3d::codec::JsonWriter;
using x3d::codec::VrmlWriter;
using x3d::codec::CanonicalXmlWriter;

// Execution context (runtime / tick)
// Construct one per loaded document, default-constructed. Setup order:
//   ctx.buildSceneGraph(doc.scene);   // index transforms/bindings/pick tree
//   ctx.buildFrom(doc.scene);         // resolve DEF-named ROUTEs + IS redirects
//   x3d::runtime::attachStandardRuntime(doc.scene, ctx); // built-in behaviors
//   ctx.addSystem(...) / ctx.addScriptSystem(...);  // optional; before first tick()
// Then once per frame: set inputs, ctx.tick(now) [now = seconds since start],
// read the pull surface.
using x3d::runtime::X3DExecutionContext;
using x3d::runtime::FieldWriteResult; ///< ctx.writeField's [[nodiscard]] outcome
using x3d::runtime::fieldWriteResultName;
using x3d::runtime::BridgeResult;
using x3d::runtime::RouteError;
using x3d::runtime::System;
using x3d::runtime::DirtyTracker; ///< per-tick change set
using x3d::runtime::PickResult;
using x3d::runtime::Ray;
using x3d::runtime::Mat4; ///< column-major 4×4
using x3d::runtime::Aabb;

// Session — the recommended entry point
// Owns the document, context and extractor, and wires the standard runtime.
// The context and extractor cannot outlive the owned scene.
//
//   auto session = sdk::RuntimeSession::create(sdk::parseFile("scene.x3dv"));
//   sdk::RenderDelta f0 = session->fullSnapshot();     // upload f0.added
//   while (running) { session->tick(now); auto d = session->delta(); }
using x3d::runtime::RuntimeSession;
using x3d::runtime::SessionOptions;

// Extraction / render feed
// In-process descriptors, not a wire/ABI contract. See docs/wiki/guides/native-host.md.
using x3d::runtime::extract::SceneExtractor;
using x3d::runtime::extract::RenderItem;
using x3d::runtime::extract::RenderDelta;
using x3d::runtime::extract::RenderItemId;
using x3d::runtime::extract::kInvalidRenderItemId;
using x3d::runtime::extract::PathKey;
using x3d::runtime::extract::PathKeyHash;
using x3d::runtime::extract::PathKeyEqual;
using x3d::runtime::extract::GeomId; ///< in-process, baseline-scoped content key
using x3d::runtime::extract::GeomIdHash;
using x3d::runtime::extract::MeshData;
using x3d::runtime::extract::Topology;
using x3d::runtime::extract::MaterialDesc;
using x3d::runtime::extract::MaterialModel;
using x3d::runtime::extract::AlphaMode;
using x3d::runtime::extract::LightDesc;
using x3d::runtime::extract::CameraDesc;
using x3d::runtime::extract::BackgroundDesc;
using x3d::runtime::extract::FogDesc; ///< bound Fog (§24.4.2)

// Seams (embedder-supplied IO)

// Tessellation density and font-metrics build knobs.
using x3d::runtime::extract::MeshBuildOptions;

// Asset resolution (ADR-0023):
//   render-time may return Pending (retry next frame); parse-time (Inline/
//   ExternProto) must return Ready/Failed synchronously.
using x3d::runtime::extract::AssetResolver;
using x3d::runtime::extract::AssetResult;
using x3d::runtime::extract::AssetStatus;
using x3d::runtime::extract::AssetKind;

// Embedder returns decoded RGBA pixels, retained on TextureRef (ADR-0024).
using x3d::runtime::extract::TextureResolver;
using x3d::runtime::extract::TexturePixelResult;
using x3d::runtime::extract::TexturePixels;
using x3d::runtime::extract::TextureResolveStatus;
using x3d::runtime::extract::TextureRef;
using x3d::runtime::extract::SamplerParams;

// Embedder supplies glyph metrics; the SDK performs Text layout (ADR-0025).
// The default is a monospaced stub.
using x3d::runtime::extract::FontMetrics;
using x3d::runtime::extract::FontKey;
using x3d::runtime::extract::GlyphMetrics;
using x3d::runtime::extract::GlyphResult;
using x3d::runtime::extract::GlyphStatus;
using x3d::runtime::extract::makeMonospaceStub; ///< default glyph advance is 0.6 em

// Geospatial worlds retain a constructor-fixed projection through
// SessionOptions::geoProjection or X3DExecutionContext(fields, projection).
// Pure helpers require an explicit projection reference; use
// runtime::geo::builtinProjection() for standalone built-in conversions
// (ADR-0058).

// Script / SAI
// ScriptEngine backend contract: ADR-0022.
// Bind a parsed scene with X3DExecutionContext(doc.scene.authorFields), or let
// an untouched default context bind once at its first scene build. Register
// author fields through that owner and use effectiveFields(node, store).
// ScriptSystem/SaiContext use the bound context; no process-global store exists.
// See ADR-0057 for author-field ownership.
using x3d::runtime::ScriptEngine;
using x3d::runtime::ScriptSystem;
using x3d::runtime::SaiContext;
using x3d::runtime::ScriptHandle; ///< 0 == kInvalidScriptHandle
using x3d::runtime::kInvalidScriptHandle;

} // namespace x3d::sdk

#endif // X3D_SDK_HPP

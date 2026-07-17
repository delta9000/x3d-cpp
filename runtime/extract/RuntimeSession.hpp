// ─── runtime/extract/RuntimeSession.hpp ──────────────────────────────────────
// RuntimeSession — the one-call path from a parsed document to a render feed.
//
// The low-level path is five steps whose LIFETIME rules are invisible in the
// call itself, and one of which is silent to omit:
//
//   sdk::X3DExecutionContext ctx;
//   ctx.buildSceneGraph(doc.scene);         // transforms/bounds/bindings/pick
//   ctx.buildFrom(doc.scene);               // resolve DEF-named ROUTEs
//   sdk::attachStandardRuntime(doc.scene, ctx);  // §8/§19/§39/§30/§22/§23/§21/§9
//   sdk::SceneExtractor ex(ctx, doc.scene); // scene passed a SECOND time
//   // ...and doc + ctx must both outlive ex, which holds references to them.
//
// Nothing there is wrong, and an embedder that wants the pieces should keep
// using them — they stay public. But it exposes initialisation phases the SDK
// can just own, and it hands the caller three objects with an unwritten lifetime
// contract between them. RuntimeSession owns all three in one place and cannot
// outlive its own document.
//
//   auto session = sdk::RuntimeSession::create(sdk::parseFile("scene.x3dv"));
//   sdk::RenderDelta f0 = session->fullSnapshot();     // upload f0.added
//   while (running) {
//     session->tick(now);
//     sdk::RenderDelta d = session->delta();           // apply d
//   }
//
// WHAT THIS IS NOT: a policy layer. It adds no behaviour, no defaults you cannot
// see, and no ordering the low-level path does not already have. Every piece
// stays reachable — context(), extractor(), scene(), document() — so dropping to
// the low-level API for one call never means abandoning the session.
//
// ON "ORDER MATTERS": it does not, and this type does not pretend otherwise.
// buildSceneGraph() and buildFrom() write DISJOINT state (the former touches
// transforms_/bounds_/bindings_/pick_, the latter only graph_), so they commute.
// What actually bites is OMITTING one: skip buildSceneGraph and no Viewpoint
// binds, so viewMatrix() silently returns identity; skip buildFrom and authored
// ROUTEs never fire, so the scene renders but never animates. Both are silent.
// The session's value is that neither is a step you can forget, not that it
// sequences something delicate.
// ─────────────────────────────────────────────────────────────────────────────
#ifndef X3D_RUNTIME_SESSION_HPP
#define X3D_RUNTIME_SESSION_HPP

#include "AssetResolver.hpp"       // AssetResolver (LoadSensor byte oracle)
#include "MeshBuilder.hpp"         // MeshBuildOptions
#include "SceneExtractor.hpp"      // SceneExtractor, RenderDelta
#include "TextureResolver.hpp"     // TextureResolver, makeNullTextureResolver
#include "X3DDocument.hpp"         // X3DDocument
#include "X3DExecutionContext.hpp" // X3DExecutionContext
#include "X3DSceneBridge.hpp"      // BridgeResult (buildFrom's return type)

#include <memory>
#include <utility>

namespace x3d::runtime {

/// Everything RuntimeSession::create can be told, in one named bag. A struct
/// rather than positional parameters so a call site reads as what it turns on.
struct SessionOptions {
  /// Attach the standard behavior systems: §8 TimeSensor, §19 interpolators,
  /// §39 followers, §30 event utilities, §22/§23 LOD/Billboard/Proximity/
  /// Visibility, §21 key-device sensors, §9 LoadSensor, §23.3.1 viewpoint bind.
  ///
  /// Default ON, and this is NOT the session smuggling in policy: without these
  /// the ROUTEs resolve but nothing drives them, so an authored scene loads,
  /// renders one static frame and never animates — silently. That is the single
  /// most forgettable step on the low-level path, which is the whole reason this
  /// type exists. Turn it OFF to drive the graph entirely from your own Systems.
  bool standardRuntime = true;

  /// Attach the pointer-driven systems (PointingSensorSystem + NavigationSystem)
  /// in arbitration order. Default OFF: they are only meaningful once an embedder
  /// feeds real pointer/key input, and a headless or offline consumer wants
  /// neither. Access the NavigationSystem afterwards via navigation().
  bool interactive = false;

  /// Forwarded verbatim to SceneExtractor, defaulted identically. The session
  /// introduces no hidden I/O: with the default resolver the SDK decodes nothing.
  extract::MeshBuildOptions meshOptions{};
  extract::TextureResolver textureResolver = extract::makeNullTextureResolver();

  /// The byte oracle §9 LoadSensor resolves its watched children through
  /// (attached as part of the standard runtime). The session introduces no hidden
  /// I/O: null is the IO-free null stub (Failed), mirroring `textureResolver`.
  /// The SDK ships no concrete backend — an embedder injects one (e.g. a
  /// confined local-file resolver) to make watched children actually load.
  extract::AssetResolver assetResolver = nullptr;
};

class RuntimeSession {
public:
  /// Take ownership of `doc`, wire the runtime, and return a ready session.
  ///
  /// Returned by unique_ptr, not by value, and deliberately: the extractor holds
  /// REFERENCES to the context and scene that live inside this object, so a
  /// RuntimeSession must not move once built. Pinning it on the heap makes that
  /// structural instead of a comment nobody reads.
  static std::unique_ptr<RuntimeSession> create(X3DDocument doc,
                                                SessionOptions options = {}) {
    return std::unique_ptr<RuntimeSession>(
        new RuntimeSession(std::move(doc), std::move(options)));
  }

  RuntimeSession(const RuntimeSession &) = delete;
  RuntimeSession &operator=(const RuntimeSession &) = delete;
  RuntimeSession(RuntimeSession &&) = delete;
  RuntimeSession &operator=(RuntimeSession &&) = delete;

  // ── the per-frame loop ────────────────────────────────────────────────────

  /// Advance the behavior model to `now` (seconds, any monotonic clock).
  void tick(double now) { ctx_.tick(now); }

  /// Every item, all in `added` — the frame-0 upload. Also re-baselines the
  /// incremental channel, so a later delta() diffs against this walk.
  extract::RenderDelta fullSnapshot() { return extractor_.fullSnapshot(); }

  /// This tick's incremental changes. Total: calling it before any
  /// fullSnapshot() returns the snapshot, and calling it twice without an
  /// intervening tick() returns an empty delta. See SceneExtractor::delta().
  extract::RenderDelta delta() { return extractor_.delta(); }

  // ── the pieces, still yours ───────────────────────────────────────────────

  X3DExecutionContext &context() { return ctx_; }
  const X3DExecutionContext &context() const { return ctx_; }

  extract::SceneExtractor &extractor() { return extractor_; }
  const extract::SceneExtractor &extractor() const { return extractor_; }

  Scene &scene() { return doc_.scene; }
  const Scene &scene() const { return doc_.scene; }

  X3DDocument &document() { return doc_; }
  const X3DDocument &document() const { return doc_; }

  /// The ROUTE-resolution result from construction: how many ROUTEs bound, and
  /// each one that did not, with a reason. Worth checking once after create() —
  /// a scene whose ROUTEs were all rejected parses and renders fine, and simply
  /// never animates.
  const BridgeResult &routes() const { return bridge_; }

  /// The NavigationSystem, iff SessionOptions::interactive was set — the handle
  /// for setForcedMode etc. Null otherwise.
  const std::shared_ptr<NavigationSystem> &navigation() const { return nav_; }

private:
  RuntimeSession(X3DDocument doc, SessionOptions options)
      // Member ORDER is the contract: doc_ first (ctx_ and extractor_ both refer
      // into its scene), then ctx_, then extractor_ last (it captures both by
      // reference). Declaration order below fixes this; do not reorder.
      : doc_(std::move(doc)),
        extractor_(ctx_, doc_.scene, std::move(options.meshOptions),
                   std::move(options.textureResolver)) {
    ctx_.buildSceneGraph(doc_.scene);
    bridge_ = ctx_.buildFrom(doc_.scene);
    // Systems are attached AFTER both builds and BEFORE any tick(), which is the
    // one ordering here that is real: attachStandardRuntime walks the scene the
    // buildSceneGraph pass just sanitized, and a System added after the first
    // tick() misses that tick's update.
    if (options.standardRuntime)
      attachStandardRuntime(doc_.scene, ctx_, std::move(options.assetResolver));
    if (options.interactive) nav_ = attachInteractive(doc_.scene, ctx_);
  }

  X3DDocument doc_;
  X3DExecutionContext ctx_;
  BridgeResult bridge_;
  std::shared_ptr<NavigationSystem> nav_; // set iff options.interactive.
  extract::SceneExtractor extractor_; // MUST stay last: refs ctx_ and doc_.scene.
};

} // namespace x3d::runtime
#endif

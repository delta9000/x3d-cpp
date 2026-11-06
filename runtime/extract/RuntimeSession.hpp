// Owns the document, execution context and extractor together so their borrowed
// references remain valid for the session's lifetime.
//
//   auto session = sdk::RuntimeSession::create(sdk::parseFile("scene.x3dv"));
// sdk::RenderDelta f0 = session->fullSnapshot();
//   while (running) {
//     session->tick(now);
// sdk::RenderDelta d = session->delta();
//   }
#ifndef X3D_RUNTIME_SESSION_HPP
#define X3D_RUNTIME_SESSION_HPP

#include "AssetResolver.hpp"
#include "MeshBuilder.hpp"
#include "SceneExtractor.hpp"
#include "TextureResolver.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"

#include <memory>
#include <utility>

namespace x3d::runtime {

struct SessionOptions {
  /// One constructor-fixed backend for transforms, geometry, camera and events.
  /// Null selects the stable built-in backend once, before any scene attachment.
  std::shared_ptr<const geo::GeoProjection> geoProjection;

  /// Attach the standard behavior systems: §8 TimeSensor, §19 interpolators,
  /// §39 followers, §30 event utilities, §22/§23 LOD/Billboard/Proximity/
  /// Visibility, §21 key-device sensors, §9 LoadSensor, §23.3.1 viewpoint bind,
  /// §31 shader selection events (with meshOptions.shaders).
  /// Enabled by default to drive authored behavior. Disable when the host
  /// supplies its own systems.
  bool standardRuntime = true;

  /// Attach the pointer-driven systems (PointingSensorSystem + NavigationSystem)
  /// in arbitration order. Disabled by default; the embedder supplies pointer/key
  /// input. Access the NavigationSystem afterwards via navigation().
  bool interactive = false;

  /// With `interactive`: receives each activated Anchor whose url is not an
  /// in-scene "#Viewpoint" (§9.4.1), with its url and parameter lists. The
  /// headless session loads nothing itself; null ignores such activations.
  AnchorSystem::AnchorHandler anchorHandler;

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

  /// Resolver and base URL used when an Inline receives load=TRUE after parse.
  InlineResolver inlineResolver = {};
  std::string baseUrl;
};

class RuntimeSession {
public:
  /// Take ownership of `doc`, wire the runtime, and return a ready session.
  /// The extractor borrows the context and scene stored in this object, so the
  /// session is heap-pinned and cannot move after construction.
  static std::unique_ptr<RuntimeSession> create(X3DDocument doc,
                                                SessionOptions options = {}) {
    return std::unique_ptr<RuntimeSession>(
        new RuntimeSession(std::move(doc), std::move(options)));
  }

  /// The host must finish all native calls before destruction (including calls
  /// entered from a node handler). Retire before extractor/context teardown;
  /// ctx_'s own destructor also covers partially constructed sessions.
  ~RuntimeSession() { if (!ctx_.retireCallbacks()) std::terminate(); }

  RuntimeSession(const RuntimeSession &) = delete;
  RuntimeSession &operator=(const RuntimeSession &) = delete;
  RuntimeSession(RuntimeSession &&) = delete;
  RuntimeSession &operator=(RuntimeSession &&) = delete;

  /// Advance the behavior model to `now` (seconds, any monotonic clock).
  void tick(double now) { ctx_.tick(now); }

  /// Every item, all in `added` — the frame-0 upload. Also re-baselines the
  /// incremental channel, so a later delta() diffs against this walk.
  extract::RenderDelta fullSnapshot() { return extractor_.fullSnapshot(); }

  /// This tick's incremental changes. Total: calling it before any
  /// fullSnapshot() returns the snapshot, and calling it twice without an
  /// intervening tick() returns an empty delta. See SceneExtractor::delta().
  extract::RenderDelta delta() { return extractor_.delta(); }

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
      // Member order is the contract: doc_ first (ctx_ and extractor_ both refer
      // into its scene), then ctx_, then extractor_ last (it captures both by
      // reference). Declaration order below fixes this; do not reorder.
      : doc_(std::move(doc)),
        ctx_(doc_.scene.authorFields, std::move(options.geoProjection)),
        extractor_(ctx_, doc_.scene, options.meshOptions,
                   std::move(options.textureResolver)) {
    ctx_.buildSceneGraph(doc_.scene);
    bridge_ = ctx_.buildFrom(doc_.scene);
    // Systems are attached after both builds and before any tick(), which is the
    // one ordering here that is real: attachStandardRuntime walks the scene the
    // buildSceneGraph pass just sanitized, and a System added after the first
    // tick() misses that tick's update.
    if (options.standardRuntime)
      attachStandardRuntime(doc_.scene, ctx_, std::move(options.assetResolver),
                            std::move(options.inlineResolver),
                            std::move(options.baseUrl),
                            // The extractor's copy selects the bound program;
                            // this one drives the matching §31 events.
                            options.meshOptions.shaders);
    if (options.interactive)
      nav_ = attachInteractive(doc_.scene, ctx_, std::move(options.anchorHandler));
  }

  X3DDocument doc_;
  X3DExecutionContext ctx_;
  BridgeResult bridge_;
  std::shared_ptr<NavigationSystem> nav_; // set iff options.interactive.
  extract::SceneExtractor extractor_; // Must stay last: borrows ctx_ and doc_.scene.
};

} // namespace x3d::runtime
#endif

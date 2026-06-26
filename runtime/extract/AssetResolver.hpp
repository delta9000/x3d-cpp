// AssetResolver.hpp — B8 asset-resolver seam (the BYTES path). namespace
// x3d::runtime::extract. Header-only, golden-untouched, pure POD + one
// std::function type. Depends ONLY on std; introduces no node/transform header
// dependency, so it stays a leaf alongside RenderItem.hpp.
//
// WHY A SEAM: the SDK NEVER does I/O. TextureRef (RenderItem.hpp) already
// surfaces a texture's URL VERBATIM and PixelTexture pixels INLINE; this header
// adds the embedder-supplied callback by which a CONSUMER turns a URL into
// bytes. The SDK does not own this callback's lifetime, does not call it itself
// for the texture path, and never opens a file — MaterialSystem keeps its
// bytes-not-loaded-here invariant.
//
// ONE CALLBACK TYPE, TWO INVOCATION CONTRACTS (locked B8 design). The SAME
// AssetResolver signature serves two distinct call sites with DIFFERENT
// lifetimes. Do NOT assume one async lifetime covers both:
//
//   (A) TEXTURE / MOVIE bytes — consumed at RENDER time, in the CONSUMER layer,
//       AFTER extraction. The consumer holds the resolver and calls it per
//       TextureRef::Source::Url. It MAY return Status::Pending (the bytes are
//       not ready yet — e.g. a future async/http hook); the consumer simply
//       retries on a later frame and renders the flat material color meanwhile.
//       A Pending texture NEVER blocks a frame. (PixelTexture is Source::Inline
//       and is decoded directly from inlinePixels — the resolver is NOT called.)
//
//   (B) Inline / EXTERNPROTO bytes — resolved at PARSE / scene-build time,
//       BEFORE buildSceneGraph, SYNCHRONOUSLY. A pending result here is
//       INCOHERENT: nodes must exist before extraction can walk them, so this
//       call site requires Status::Ready (or Status::Failed) and treats Pending
//       as a hard error. This site is OUTSIDE this milestone's PoC wiring; the
//       type is shared so a single embedder resolver covers both, but the
//       contract difference is documented here, not papered over.
//
// The PoC wires only contract (A): a local-file resolver (relative to the scene
// dir, owned by the PoC, NOT the SDK) feeds stb_image at upload time.
#ifndef X3D_RUNTIME_EXTRACT_ASSET_RESOLVER_HPP
#define X3D_RUNTIME_EXTRACT_ASSET_RESOLVER_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

// What KIND of asset is being requested. Lets one resolver branch on intent
// (e.g. a texture cache vs an Inline scene-graph fetch) and pick a sync-vs-async
// policy per the two invocation contracts documented above.
enum class AssetKind {
  Texture,       // ImageTexture / Appearance.texture URL — contract (A), render-time.
  Movie,         // MovieTexture URL — contract (A), render-time (descriptor-only PoC).
  Inline,        // Inline node url — contract (B), parse-time SYNC.
  ExternProto,   // EXTERNPROTO url — contract (B), parse-time SYNC.
  ExternalGeometry, // external mesh blob for an ExternalGeometry node — contract (A), lazy materialize.
};

// The lifecycle state of a resolve. Pending is ONLY legal on the render-time
// texture path (contract A); the parse-time path (contract B) must resolve to
// Ready or Failed synchronously.
enum class AssetStatus {
  Ready,   // `bytes` carry the resolved asset.
  Pending, // not ready yet; retry next frame (contract A only). `bytes` empty.
  Failed,  // resolution failed (missing file, bad URL, IO error). `bytes` empty.
};

// The result of resolving ONE url. POD: a status + the raw bytes (still encoded,
// e.g. a PNG/JPEG byte stream for a Texture — the consumer DECODES; the SDK
// never decodes either). For Pending/Failed `bytes` is empty.
struct AssetResult {
  AssetStatus status = AssetStatus::Failed;
  std::vector<std::uint8_t> bytes;

  bool ready() const { return status == AssetStatus::Ready; }
  bool pending() const { return status == AssetStatus::Pending; }
  bool failed() const { return status == AssetStatus::Failed; }

  // Convenience constructors for the common cases.
  static AssetResult makeReady(std::vector<std::uint8_t> b) {
    return AssetResult{AssetStatus::Ready, std::move(b)};
  }
  static AssetResult makePending() {
    return AssetResult{AssetStatus::Pending, {}};
  }
  static AssetResult makeFailed() { return AssetResult{AssetStatus::Failed, {}}; }
};

// THE callback type. (url, kind) -> AssetResult. ONE type, TWO invocation
// contracts (see the file header). Owned by the CONSUMER; the SDK never invokes
// it for the texture path and never does I/O. Copyable value type (std::function
// is copyable) so it threads through value-type option carriers identically to
// the B5 GeoProjection seam.
using AssetResolver =
    std::function<AssetResult(const std::string &url, AssetKind kind)>;

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_ASSET_RESOLVER_HPP

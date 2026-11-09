// Caller-owned asset-byte callback; see ADR-0023.
// Render-time requests may return Pending and are retried without blocking.
// Inline/EXTERNPROTO requests run synchronously before buildSceneGraph: they
// require Ready or Failed, because scene nodes must exist before extraction.
// AssetDocumentResolvers supplies fragment-free URLs relative to the source
// directory and aborts the recursive load on Pending.
#ifndef X3D_RUNTIME_EXTRACT_ASSET_RESOLVER_HPP
#define X3D_RUNTIME_EXTRACT_ASSET_RESOLVER_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

// What kind of asset is being requested. Lets one resolver branch on intent
// (e.g. a texture cache vs an Inline scene-graph fetch) and pick a sync-vs-async
// policy per the two invocation contracts documented above.
enum class AssetKind {
  Texture,       // ImageTexture / Appearance.texture URL — contract (A), render-time.
  Movie,         // MovieTexture URL — contract (A), render-time (descriptor-only PoC).
  Inline,        // Inline node url — contract (B), parse-time SYNC.
  ExternProto,   // EXTERNPROTO url — contract (B), parse-time SYNC.
  ExternalGeometry, // external mesh blob for an ExternalGeometry node — contract (A), lazy materialize.
  Audio,         // AudioClip url — contract (A); decoded app-side via AudioDecoder.
  Shader,        // ShaderPart / ShaderProgram url — contract (A): Pending retries.
};

// The lifecycle state of a resolve. Pending is only legal on the render-time
// texture path (contract A); the parse-time path (contract B) must resolve to
// Ready or Failed synchronously.
enum class AssetStatus {
  Ready,   // `bytes` carry the resolved asset.
  Pending, // not ready yet; retry next frame (contract A only). `bytes` empty.
  Failed,  // resolution failed (missing file, bad URL, IO error). `bytes` empty.
};

// The result of resolving one url. POD: a status + the raw bytes (still encoded,
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

// Caller-owned, copyable callback with the render-time and parse-time contracts above.
using AssetResolver =
    std::function<AssetResult(const std::string &url, AssetKind kind)>;

// Contextual render-time variant, currently used by LoadSensor. Authored URL,
// intent, and the watched node's source directory; the host owns resolution.
// Parse-time loaders already pass resolved URLs to the legacy AssetResolver.
using AssetResolverWithBase = std::function<AssetResult(
    const std::string &url, AssetKind kind, const std::string &baseUrl)>;

// default stub: makeNullAssetResolver() returns a callable that always answers
// makeFailed() — the IO-free default, mirroring makeNullTextureResolver(). The
// SDK ships no concrete backend (a local-file/HTTP/S3 resolver is a backend, kept
// out of the public headers, ADR — IO-free SDK); an app or embedder injects one.
// A null std::function is not legal to call — always use this stub as the default.
inline AssetResolver makeNullAssetResolver() {
  return [](const std::string &, AssetKind) { return AssetResult::makeFailed(); };
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_ASSET_RESOLVER_HPP

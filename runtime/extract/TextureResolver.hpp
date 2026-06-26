// TextureResolver.hpp — T-TEX texture-decode seam (consumer-supplied decoded pixels).
// namespace x3d::runtime::extract. Header-only, std-only, leaf.
//
// IO CONTRACT: the SDK NEVER decodes image bytes. The consumer (or an embedder
// adapter around stb_image / libpng) supplies this callback; the SDK threads the
// result onto the RenderItem surface. Same Pending semantics as AssetResolver
// contract (A): Pending = not ready yet, retry next frame; never blocks a frame.
//
// PixelTexture (TextureRef::Source::Inline) is already decoded from SFImage::data
// and NEVER goes through this resolver. This resolver handles Source::Url only.
//
// PIXEL FORMAT: RGBA8, tightly packed; stride = width * 4 bytes; origin =
// bottom-left (GL convention — matches MeshData::texcoords; no V-flip here).
//
// URL SELECTION: the caller applies the MFString fallback order from TextureRef::url
// and passes the single resolved URL to this resolver. URL selection (try-first,
// fall-through) is the caller's responsibility, exactly as in AssetResolver.
//
// DEFAULT STUB: makeNullTextureResolver() returns a callable that always returns
// makeFailed(). Legal to pass as a default; the PoC renders a white fallback.
// A null std::function is NOT legal to call; always use the stub.
//
// SEAM PLACEMENT: this header is included by the forthcoming TextureExtract.hpp,
// NOT by SceneExtractor.hpp directly — thin dispatch hooks in SceneExtractor will
// forward to TextureExtract, keeping the shared-file edit surface minimal.
#ifndef X3D_RUNTIME_EXTRACT_TEXTURE_RESOLVER_HPP
#define X3D_RUNTIME_EXTRACT_TEXTURE_RESOLVER_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

// ---------------------------------------------------------------------------
// TexturePixels — decoded RGBA8 surface.
//
// rgba is tightly packed: stride = width * 4 bytes, origin = bottom-left
// (GL convention — matches MeshData::texcoords; no V-flip needed here).
// width and height are 0 on Pending and Failed results.
// ---------------------------------------------------------------------------
struct TexturePixels {
    std::uint32_t width  = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba; // width * height * 4 bytes when Ready.
};

// ---------------------------------------------------------------------------
// TextureResolveStatus — lifecycle state of a texture resolve.
//
// Pending is ONLY legal on the render-time path (same contract-A restriction as
// AssetResolver). A Pending texture NEVER blocks a frame; the consumer renders
// the flat material colour meanwhile and retries on a later frame.
// ---------------------------------------------------------------------------
enum class TextureResolveStatus { Ready, Pending, Failed };

// ---------------------------------------------------------------------------
// TexturePixelResult — POD result carrier.
// ---------------------------------------------------------------------------
struct TexturePixelResult {
    TextureResolveStatus status = TextureResolveStatus::Failed;
    TexturePixels        pixels;

    bool ready()   const { return status == TextureResolveStatus::Ready;   }
    bool pending() const { return status == TextureResolveStatus::Pending; }
    bool failed()  const { return status == TextureResolveStatus::Failed;  }

    static TexturePixelResult makeReady(TexturePixels p) {
        return {TextureResolveStatus::Ready, std::move(p)};
    }
    static TexturePixelResult makePending() {
        return {TextureResolveStatus::Pending, {}};
    }
    static TexturePixelResult makeFailed() {
        return {TextureResolveStatus::Failed, {}};
    }
};

// ---------------------------------------------------------------------------
// TextureResolver — THE callback type.
//
// (url) -> TexturePixelResult. `url` is the single resolved URL — the caller
// has already applied the MFString fallback order from TextureRef::url.
// Copyable value type (std::function) — same threading contract as AssetResolver:
// owned by the CONSUMER; the SDK never calls it during scene build.
// ---------------------------------------------------------------------------
using TextureResolver = std::function<TexturePixelResult(const std::string& url)>;

// ---------------------------------------------------------------------------
// makeNullTextureResolver — default stub.
//
// Always returns makeFailed(). The PoC renders a white fallback for failed
// textures. Legal to use as the default when no real resolver is wired in.
// ---------------------------------------------------------------------------
inline TextureResolver makeNullTextureResolver() {
    return [](const std::string&) {
        return TexturePixelResult::makeFailed();
    };
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_TEXTURE_RESOLVER_HPP

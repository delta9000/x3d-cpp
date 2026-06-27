// asset_resolver_b8_test.cpp — Browser-level B8 acceptance (SDK side): the
// asset-resolver seam TYPE + AssetResult COMPILE and behave per the locked
// contract. The PoC owns the actual file I/O + stb_image decode; this test pins
// the SDK contract that the seam introduces.
//
// Proofs:
//   1) AssetResult constructs in all three lifecycle states (Ready/Pending/
//      Failed) and the ready()/pending()/failed() predicates agree.
//   2) AssetResolver is an assignable std::function (copyable value type) that a
//      CONSUMER can wire and invoke as (url, AssetKind) -> AssetResult; the SDK
//      never invokes it itself (this test stands in for the consumer).
//   3) Contract (A) render-time texture path: a resolver MAY return Pending and
//      then Ready on a later call (deferred bytes), and the consumer reads the
//      bytes only when ready() — Pending never yields bytes.
//   4) Contract (B) parse-time path: a resolver asked for AssetKind::Inline
//      resolves SYNCHRONOUSLY to Ready/Failed; this test asserts the resolver
//      can distinguish the kind so a single embedder callback serves both sites.
//   5) A MaterialDesc's TextureRef carries the URL VERBATIM (Source::Url) and a
//      PixelTexture carries pixels INLINE (Source::Inline) — the descriptor side
//      the resolver consumes, proving bytes are NOT loaded in the SDK.
#include "AssetResolver.hpp"
#include "MaterialSystem.hpp"
#include "RenderItem.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

TEST_CASE("asset_resolver_b8_test") {
  // ---- 1. AssetResult lifecycle states + predicates --------------------------
  {
    AssetResult r = AssetResult::makeReady({1, 2, 3, 4});
    CHECK((r.ready() && !r.pending() && !r.failed()));
    CHECK((r.status == AssetStatus::Ready));
    CHECK((r.bytes.size() == 4));

    AssetResult p = AssetResult::makePending();
    CHECK((p.pending() && !p.ready() && !p.failed()));
    CHECK((p.bytes.empty()));

    AssetResult f = AssetResult::makeFailed();
    CHECK((f.failed() && !f.ready() && !f.pending()));
    CHECK((f.bytes.empty()));
  }

  // ---- 2+3. Contract (A): render-time resolver MAY defer (Pending -> Ready) ---
  {
    int calls = 0;
    // A toy "async" resolver: first call Pending, second Ready. This is exactly
    // the deferred-bytes lifecycle a future http hook needs — the consumer
    // retries next frame and never blocks.
    AssetResolver resolver = [&calls](const std::string &url,
                                      AssetKind kind) -> AssetResult {
      CHECK((kind == AssetKind::Texture));
      CHECK((url == "brick.png"));
      if (++calls == 1) return AssetResult::makePending();
      return AssetResult::makeReady({0xAB, 0xCD});
    };

    AssetResult first = resolver("brick.png", AssetKind::Texture);
    CHECK((first.pending() && first.bytes.empty())); // frame N: not ready, retry.

    AssetResult second = resolver("brick.png", AssetKind::Texture);
    CHECK((second.ready() && second.bytes.size() == 2)); // frame N+1: bytes arrive.
    CHECK((second.bytes[0] == 0xAB && second.bytes[1] == 0xCD));

    // The resolver is a copyable value type (std::function): copy it and the
    // copy shares the same callable, proving it threads by value like B5's seam.
    AssetResolver copy = resolver;
    CHECK((static_cast<bool>(copy)));
  }

  // ---- 4. Contract (B): parse-time Inline resolves SYNC (Ready/Failed only) ---
  {
    // One embedder resolver branching on AssetKind: Inline resolves immediately,
    // never Pending (a pending Inline would be incoherent — nodes must exist
    // before extraction).
    AssetResolver embedder = [](const std::string &url,
                                AssetKind kind) -> AssetResult {
      if (kind == AssetKind::Inline) {
        if (url == "sub.x3d") return AssetResult::makeReady({'<', 'X', '3', 'D'});
        return AssetResult::makeFailed();
      }
      return AssetResult::makeFailed();
    };

    AssetResult inl = embedder("sub.x3d", AssetKind::Inline);
    CHECK((inl.ready() && !inl.pending())); // SYNC: never Pending on contract (B).
    CHECK((inl.bytes.size() == 4));

    AssetResult missing = embedder("nope.x3d", AssetKind::Inline);
    CHECK((missing.failed()));
  }

  // ---- 5. Descriptor side the resolver consumes (URL verbatim / pixels inline) -
  {
    // ImageTexture under an Appearance -> TextureRef::Source::Url, url verbatim.
    auto app = createX3DNode("Appearance");
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "url", std::any(std::vector<std::string>{"brick.png", "fallback.jpg"}));
    setF(app, "texture", std::any(std::static_pointer_cast<X3DNode>(tex)));

    MaterialDesc m = materialOf(app.get());
    CHECK((!m.textures.empty()));
    const TextureRef &t = m.textures.front();
    CHECK((t.source == TextureRef::Source::Url));
    CHECK((t.url.size() == 2 && t.url[0] == "brick.png")); // VERBATIM, not loaded.

    // PixelTexture -> Source::Inline; pixels carried inline (resolver NOT used).
    auto app2 = createX3DNode("Appearance");
    auto px = createX3DNode("PixelTexture");
    SFImage img;
    img.width = 1;
    img.height = 1;
    img.numComponents = 3;
    img.data = {0xFF, 0x00, 0x00};
    setF(px, "image", std::any(img));
    setF(app2, "texture", std::any(std::static_pointer_cast<X3DNode>(px)));

    MaterialDesc m2 = materialOf(app2.get());
    CHECK((!m2.textures.empty()));
    const TextureRef &t2 = m2.textures.front();
    CHECK((t2.source == TextureRef::Source::Inline));
    CHECK((t2.inlinePixels.width == 1 && t2.inlinePixels.height == 1));
    CHECK((t2.inlinePixels.numComponents == 3));
  }

  return;
}

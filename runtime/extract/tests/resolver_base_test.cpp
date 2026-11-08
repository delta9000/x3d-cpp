// Resolver URL provenance through the public session wiring. No files or decoders.
#include "x3d/sdk.hpp"
#include "AssetDocumentResolvers.hpp"
#include "TextureExtract.hpp"
#include "doctest/doctest.h"

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {
namespace sdk = x3d::sdk;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

std::string document(const std::string &body) {
  return "<X3D profile='Full' version='4.0'><Scene>" + body + "</Scene></X3D>";
}

const std::string shape = R"(<Shape><Appearance>
  <ImageTexture DEF='Image' url='"missing" "shared.png"'/>
  </Appearance><Box/></Shape>)";

TexturePixelResult pixel() {
  return TexturePixelResult::makeReady(TexturePixels{1, 1, {1, 2, 3, 255}});
}

TEST_CASE("resolver base: Inline and PROTO origins reach texture and asset callbacks") {
  const std::map<std::string, std::string> files{
      {"/scene/child/child.x3d", document(shape + R"(
        <LoadSensor><ImageTexture containerField='children' url='"shared.png"'/>
          <MovieTexture containerField='children' url='"shared.png"'/></LoadSensor>)")},
      {"/scene/library/widget.x3d", document(
          "<ProtoDeclare name='Widget'><ProtoBody>" + shape +
          "</ProtoBody></ProtoDeclare>")}};
  sdk::AssetResolver assets = [&](const auto &url, sdk::AssetKind) {
    auto it = files.find(url);
    return it == files.end() ? sdk::AssetResult::makeFailed()
        : sdk::AssetResult::makeReady({it->second.begin(), it->second.end()});
  };
  auto loaders = x3d::codec::assetResolversFrom(assets);
  auto doc = sdk::parseDocument(document(shape + R"(
    <ExternProtoDeclare name='Widget' url='"library/widget.x3d#Widget"'/>
    <ProtoInstance name='Widget'/>
    <Inline url='"child/child.x3d"'/>)"), sdk::Encoding::XML,
      "/scene", loaders.proto, loaders.inlineScene);
  REQUIRE(doc.protoWarnings.empty());
  REQUIRE(doc.inlineWarnings.empty());

  // A host-authored node with no provenance must use the session fallback.
  auto rootImage = doc.scene.resolve("Image");
  REQUIRE(rootImage);
  doc.scene.nodeBaseUrls.erase(rootImage);
  std::map<std::pair<std::string, std::string>, int> textureCalls;
  std::set<std::tuple<std::string, sdk::AssetKind, std::string>> assetCalls;
  sdk::SessionOptions options;
  options.baseUrl = "/fallback";
  options.textureResolver = [](const auto &) {
    FAIL("contextual texture callback must take precedence");
    return TexturePixelResult::makeFailed();
  };
  options.assetResolver = [](const auto &, auto) {
    FAIL("contextual asset callback must take precedence");
    return AssetResult::makeFailed();
  };
  options.textureResolverWithBase = [&](const auto &url, const auto &base) {
    ++textureCalls[{url, base}];
    return url == "missing" ? TexturePixelResult::makeFailed() : pixel();
  };
  options.assetResolverWithBase = [&](const auto &url, auto kind, const auto &base) {
    assetCalls.emplace(url, kind, base);
    return sdk::AssetResult::makeReady({1});
  };
  auto session = sdk::RuntimeSession::create(std::move(doc), options);
  auto snapshot = session->fullSnapshot();
  REQUIRE(snapshot.added.size() == 3);
  std::set<std::string> bases;
  for (auto id : snapshot.added) {
    const auto &ref = session->extractor().item(id).material.textures.front();
    bases.insert(ref.baseUrl);
    CHECK(ref.url == x3d::core::MFString{"missing", "shared.png"});
    CHECK(ref.resolvedPixels.ready());
  }
  CHECK(bases == std::set<std::string>{"/fallback", "/scene/child", "/scene/library"});
  for (const auto &base : bases) {
    CHECK(textureCalls[{"missing", base}] == 1);
    CHECK(textureCalls[{"shared.png", base}] == 1);
  }
  session->tick(0);
  CHECK(assetCalls == decltype(assetCalls){
      {"shared.png", sdk::AssetKind::Texture, "/scene/child"},
      {"shared.png", sdk::AssetKind::Movie, "/scene/child"}});
}

TEST_CASE("resolver base: runtime Inline additions retain their texture base") {
  auto loaders = x3d::codec::assetResolversFrom([](const auto &, auto) {
    const auto text = document(shape);
    return AssetResult::makeReady({text.begin(), text.end()});
  });
  auto doc = sdk::parseDocument(document(
      R"(<Inline DEF='Lazy' load='false' url='"child/child.x3d"'/>)"),
      sdk::Encoding::XML, "/scene", loaders.proto, loaders.inlineScene);
  sdk::SessionOptions options;
  options.baseUrl = "/fallback";
  options.inlineResolver = loaders.inlineScene;
  options.textureResolverWithBase = [](const auto &url, const auto &base) {
    CHECK(base == "/scene/child");
    return url == "missing" ? TexturePixelResult::makeFailed() : pixel();
  };
  auto session = sdk::RuntimeSession::create(std::move(doc), options);
  CHECK(session->fullSnapshot().added.empty());
  REQUIRE(session->context().writeField(session->scene().resolve("Lazy").get(),
                                       "load", true) == FieldWriteResult::Ok);
  session->tick(0);
  const auto loaded = session->fullSnapshot();
  REQUIRE(loaded.added.size() == 1);
  CHECK(session->extractor().item(loaded.added.front())
            .material.textures.front().resolvedPixels.ready());
}

TEST_CASE("resolver base: recursive refs, movie descriptors and legacy callbacks") {
  auto doc = sdk::parseDocument(document(R"(
    <Shape><Appearance><MultiTexture>
      <ImageTexture DEF='Still' url='"shared.png"'/>
      <ComposedCubeMapTexture><ImageTexture DEF='Face' containerField='frontTexture'
        url='"shared.png"'/></ComposedCubeMapTexture>
      <MovieTexture DEF='Movie' url='"movie.mpg"'/>
    </MultiTexture></Appearance><Box/></Shape>
    <Background DEF='Backdrop' frontUrl='"panorama.png"'/>)"), sdk::Encoding::XML);
  doc.scene.nodeBaseUrls[doc.scene.resolve("Still")] = "/still";
  doc.scene.nodeBaseUrls[doc.scene.resolve("Face")] = "/face";
  doc.scene.nodeBaseUrls.erase(doc.scene.resolve("Movie"));
  doc.scene.nodeBaseUrls[doc.scene.resolve("Backdrop")] = "/panorama";
  sdk::SessionOptions options;
  options.baseUrl = "/fallback";
  std::set<std::string> bases;
  options.textureResolverWithBase = [&](const auto &, const auto &base) {
    bases.insert(base);
    return pixel();
  };
  auto session = sdk::RuntimeSession::create(std::move(doc), options);
  const auto snapshot = session->fullSnapshot();
  REQUIRE(snapshot.added.size() == 1);
  CHECK(bases == std::set<std::string>{"/still", "/face"});
  // appendSlot flattens the outer MultiTexture into per-channel material refs.
  // The cube stage still retains its nested face refs.
  const auto &stages = session->extractor().item(snapshot.added.front())
                           .material.textures;
  REQUIRE(stages.size() == 3);
  CHECK(stages[0].source == TextureRef::Source::Url);
  CHECK(stages[0].baseUrl == "/still");
  CHECK(stages[0].resolvedPixels.ready());
  REQUIRE(stages[1].source == TextureRef::Source::Cube);
  REQUIRE(stages[1].cubeFaces.size() == 6);
  CHECK(stages[1].cubeFaces[0].baseUrl == "/face");
  CHECK(stages[1].cubeFaces[0].resolvedPixels.ready());
  CHECK(stages[2].source == TextureRef::Source::Movie);
  CHECK(stages[2].baseUrl == "/fallback");
  CHECK(stages[2].url == x3d::core::MFString{"movie.mpg"});
  CHECK(session->extractor().background().front.baseUrl == "/panorama");

  int calls = 0;
  sdk::SessionOptions legacy;
  legacy.textureResolver = [&](const std::string &url) {
    ++calls;
    CHECK(url == "shared.png");
    return pixel();
  };
  auto old = sdk::RuntimeSession::create(sdk::parseDocument(document(
      "<Shape><Appearance><ImageTexture url='\"shared.png\"'/></Appearance><Box/></Shape>")), legacy);
  old->fullSnapshot();
  CHECK(calls == 1);
}

TEST_CASE("resolver base: LoadSensor memo separates node bases and uses fallback") {
  auto doc = sdk::parseDocument(document(R"(
    <LoadSensor><ImageTexture containerField='children' DEF='A' url='"shared.png"'/>
      <ImageTexture containerField='children' DEF='B' url='"shared.png"'/>
      <ImageTexture containerField='children' DEF='Empty' url='"shared.png"'/></LoadSensor>)"),
      sdk::Encoding::XML);
  doc.scene.nodeBaseUrls[doc.scene.resolve("A")] = "/a";
  doc.scene.nodeBaseUrls.erase(doc.scene.resolve("B"));
  doc.scene.nodeBaseUrls[doc.scene.resolve("Empty")] = "";
  sdk::SessionOptions options;
  options.baseUrl = "/fallback";
  std::set<std::string> bases;
  options.assetResolverWithBase = [&](const auto &url, auto kind, const auto &base) {
    CHECK(url == "shared.png");
    CHECK(kind == sdk::AssetKind::Texture);
    bases.insert(base);
    return sdk::AssetResult::makeReady({1});
  };
  auto session = sdk::RuntimeSession::create(std::move(doc), options);
  session->tick(0);
  CHECK(bases == std::set<std::string>{"", "/a", "/fallback"});
}

TEST_CASE("resolver base: Pending retries and Ready memo is scoped by base") {
  std::vector<TextureRef> refs(3);
  for (auto &ref : refs) ref.url = {"same.png"};
  refs[0].baseUrl = refs[1].baseUrl = "/a";
  refs[2].baseUrl = "/b";
  std::unordered_map<std::string, TexturePixelResult> memo;
  int calls = 0;
  TextureResolverWithBase resolver = [&](const auto &, const auto &) {
    return ++calls == 1 ? TexturePixelResult::makePending() : pixel();
  };
  resolveTextureRefs(refs, {}, &memo, resolver);
  CHECK(refs[0].resolvedPixels.pending());
  CHECK(calls == 3); // /a Pending is retried; /b cannot reuse /a Ready.
  resolveTextureRefs(refs, {}, &memo, resolver);
  CHECK(calls == 3);
  CHECK(refs[0].resolvedPixels.ready());
  CHECK(refs[0].resolvedPixels.pixels == refs[1].resolvedPixels.pixels);
  CHECK(refs[0].resolvedPixels.pixels != refs[2].resolvedPixels.pixels);
}
} // namespace

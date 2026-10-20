#include "AssetDocumentResolvers.hpp"
#include "RuntimeSession.hpp"
#include "doctest/doctest.h"

#include <algorithm>
#include <map>
#include <string>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
namespace codec = x3d::codec;

namespace {
std::string document(const std::string &body) {
  return "<X3D profile='Full' version='4.0'><Scene>" + body + "</Scene></X3D>";
}

AssetResolver memory(std::map<std::string, std::string> files) {
  return [files = std::move(files)](const std::string &url, AssetKind) {
    auto found = files.find(url);
    if (found == files.end()) return AssetResult::makeFailed();
    return AssetResult::makeReady({found->second.begin(), found->second.end()});
  };
}

template <class Warnings>
bool contains(const Warnings &warnings, const std::string &source,
              const std::string &reason) {
  return std::any_of(warnings.begin(), warnings.end(), [&](const auto &warning) {
    return warning.detail.find(source) != std::string::npos &&
           warning.detail.find(reason) != std::string::npos;
  });
}
} // namespace

TEST_CASE("Inline diagnostics: fetched warnings reach the parent document") {
  const auto loaders = codec::assetResolversFrom(memory({
      {"memory://cave/child.x3d", document(
          "<UnknownChild/><Shape><Appearance><Material specularColor='1.5 0 0'/>"
          "</Appearance></Shape><ProtoInstance name='MissingChildProto'/>"
          "<Inline url='&quot;grand.x3d&quot;'/><Inline url='&quot;missing.x3d&quot;'/>")},
      {"memory://cave/grand.x3d", document("<UnknownGrand/>")},
      {"memory://cave/proto.x3d", document(
          "<UnknownExtern/><ProtoDeclare name='P'><ProtoBody>"
          "<Shape><Box/></Shape></ProtoBody></ProtoDeclare>")},
      {"memory://cave/cycle.x3d", document("<Inline url='&quot;cycle.x3d&quot;'/>")}}));
  auto doc = codec::parseDocument(document(
      "<Inline url='&quot;child.x3d&quot;'/><ExternProtoDeclare name='P' "
      "url='&quot;proto.x3d#P&quot;'/><ProtoInstance name='P'/>"
      "<Inline url='&quot;cycle.x3d&quot;'/>"),
      codec::Encoding::XML, "memory://cave", loaders.proto, loaders.inlineScene);
  CHECK(contains(doc.inlineWarnings, "memory://cave/child.x3d", "UnknownChild"));
  CHECK(contains(doc.inlineWarnings, "memory://cave/grand.x3d", "UnknownGrand"));
  CHECK(contains(doc.inlineWarnings, "memory://cave/child.x3d", "specularColor"));
  CHECK(contains(doc.inlineWarnings, "memory://cave/child.x3d", "MissingChildProto"));
  CHECK(contains(doc.inlineWarnings, "memory://cave/child.x3d", "missing.x3d"));
  CHECK(contains(doc.inlineWarnings, "memory://cave/cycle.x3d", "Inline cycle rejected"));
  CHECK(contains(doc.protoWarnings, "memory://cave/proto.x3d", "UnknownExtern"));
}

TEST_CASE("Inline diagnostics: runtime cycles reach document warnings once") {
  for (bool fallback : {false, true}) {
    const std::string urls = fallback ? "&quot;a.x3d&quot; &quot;clean.x3d&quot;"
                                      : "&quot;sub/../a.x3d&quot;";
    const auto loaders = codec::assetResolversFrom(memory({
        {"memory://cave/a.x3d", document(
            "<UnknownRuntime/><Inline DEF='Again' load='false' url='" + urls + "'/>")},
        {"memory://cave/clean.x3d", document("<Shape><Box/></Shape>")}}));
    auto doc = codec::parseDocument(document(
        "<Inline DEF='Start' load='false' url='&quot;a.x3d&quot;'/>"),
        codec::Encoding::XML, "memory://cave", loaders.proto, loaders.inlineScene);
    SessionOptions options;
    options.inlineResolver = loaders.inlineScene;
    options.baseUrl = "memory://cave";
    auto session = RuntimeSession::create(std::move(doc), options);
    auto &ctx = session->context();
    REQUIRE(ctx.writeField(session->scene().resolve("Start").get(), "load", true) ==
            FieldWriteResult::Ok);
    session->tick(0);
    CHECK(contains(session->document().inlineWarnings, "memory://cave/a.x3d",
                   "UnknownRuntime"));
    x3d::runtime::detail::forEachNode(session->scene(), [&](x3d::nodes::X3DNode *node) {
      if (node->getDEF() == "Again")
        REQUIRE(ctx.writeField(node, "load", true) == FieldWriteResult::Ok);
    });
    session->tick(1);
    const auto &warnings = session->document().inlineWarnings;
    CHECK(contains(warnings, "memory://cave/a.x3d", "Inline cycle rejected") == !fallback);
    if (!fallback) {
      REQUIRE(warnings.size() == 2);
      CHECK(warnings.back().kind == InlineWarning::Kind::Cycle);
      CHECK(warnings.back().inlineDEF == "Again");
    } else {
      CHECK(session->fullSnapshot().added.size() == 1);
    }
    const auto count = warnings.size();
    session->tick(2);
    CHECK(session->document().inlineWarnings.size() == count);
  }
}

TEST_CASE("EXTERNPROTO diagnostics: a fetched document's warnings are reported once") {
  const auto loaders = codec::assetResolversFrom(memory({
      {"memory://cave/proto.x3d", document(
          "<UnknownExtern/><ProtoDeclare name='P'><ProtoBody>"
          "<Shape><Box/></Shape></ProtoBody></ProtoDeclare>"
          "<ProtoDeclare name='Q'><ProtoBody><Group/></ProtoBody></ProtoDeclare>")}}));
  auto doc = codec::parseDocument(document(
      "<ExternProtoDeclare name='P' url='&quot;proto.x3d#P&quot;'/>"
      "<ProtoInstance name='P'/><ProtoInstance name='P'/><ProtoInstance name='P'/>"),
      codec::Encoding::XML, "memory://cave", loaders.proto, loaders.inlineScene);
  const auto count = std::count_if(doc.protoWarnings.begin(), doc.protoWarnings.end(),
      [](const ProtoWarning &w) {
        return w.kind == ProtoWarning::Kind::SourceWarning &&
               w.detail.find("UnknownExtern") != std::string::npos;
      });
  CHECK(count == 1);
  CHECK(contains(doc.protoWarnings, "memory://cave/proto.x3d", "UnknownExtern"));

  // The selected declaration carries the document's warning exactly once.
  const std::vector<std::string> urls{"proto.x3d#P"};
  auto p = loaders.proto(urls, "memory://cave");
  REQUIRE(p);
  CHECK(p->sourceWarnings.size() == 1);
}

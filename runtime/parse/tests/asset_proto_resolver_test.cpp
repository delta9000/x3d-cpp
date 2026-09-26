// asset_proto_resolver_test.cpp — EXTERNPROTO resolution over the AssetResolver
// seam (protoResolverFrom). A fake in-memory AssetResolver serves an .x3d
// document with two ProtoDeclares; the adapter must fetch, parse, and return the
// matching declaration (by '#fragment' when present, else the first), honoring
// the Pending/Failed parse-time contract and the url-list order.
#include "AssetProtoResolver.hpp"
#include "SchemeRouter.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::codec;
using x3d::runtime::extract::AssetKind;
using x3d::runtime::extract::AssetResolver;
using x3d::runtime::extract::AssetResult;

static const char *kDoc =
    "<X3D version='4.0'><Scene>"
    "<ProtoDeclare name='Alpha'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
    "<ProtoDeclare name='Beta'><ProtoBody><Sphere/></ProtoBody></ProtoDeclare>"
    "</Scene></X3D>";

// In-memory backend: `docs` maps url -> body; urls in `pending` answer Pending;
// everything else is Failed. Records the urls it was asked for, in order.
static AssetResolver inMemory(const std::map<std::string, std::string> &docs,
                              const std::vector<std::string> &pending,
                              std::vector<std::string> *calls) {
  return [docs, pending, calls](const std::string &url,
                                AssetKind) -> AssetResult {
    if (calls)
      calls->push_back(url);
    for (const auto &u : pending)
      if (u == url)
        return AssetResult::makePending();
    const auto it = docs.find(url);
    if (it == docs.end())
      return AssetResult::makeFailed();
    const std::string &b = it->second;
    return AssetResult::makeReady(
        std::vector<std::uint8_t>(b.begin(), b.end()));
  };
}

TEST_CASE("asset_proto_resolver_test") {
  const std::map<std::string, std::string> docs = {
      {"http://mem/protos.x3d", kDoc}};

  SUBCASE("fragment selects the named ProtoDeclare") {
    std::vector<std::string> calls;
    ProtoDeclarationResolver r =
        protoResolverFrom(inMemory(docs, {}, &calls));
    auto d = r({"http://mem/protos.x3d#Beta"}, "");
    REQUIRE(d != nullptr);
    CHECK(d->name == "Beta");
    // The fragment is stripped before the fetch.
    CHECK(calls == std::vector<std::string>{"http://mem/protos.x3d"});
  }

  SUBCASE("no fragment -> first ProtoDeclare in the document") {
    ProtoDeclarationResolver r = protoResolverFrom(inMemory(docs, {}, nullptr));
    auto d = r({"http://mem/protos.x3d"}, "");
    REQUIRE(d != nullptr);
    CHECK(d->name == "Alpha");
  }

  SUBCASE("failed candidate is skipped; url-list order is honored") {
    std::vector<std::string> calls;
    ProtoDeclarationResolver r =
        protoResolverFrom(inMemory(docs, {}, &calls));
    auto d = r({"http://mem/missing.x3d", "http://mem/protos.x3d#Beta"}, "");
    REQUIRE(d != nullptr);
    CHECK(d->name == "Beta");
    CHECK(calls ==
          std::vector<std::string>{"http://mem/missing.x3d",
                                   "http://mem/protos.x3d"});
  }

  SUBCASE("all candidates failed -> null (lenient, no throw)") {
    ProtoDeclarationResolver r = protoResolverFrom(inMemory(docs, {}, nullptr));
    CHECK(r({"ftp://mem/x.x3d", "http://mem/missing.x3d"}, "") == nullptr);
  }

  SUBCASE("Pending is a hard error at parse time -> null, later urls untouched") {
    std::vector<std::string> calls;
    ProtoDeclarationResolver r = protoResolverFrom(
        inMemory(docs, {"http://mem/slow.x3d"}, &calls));
    CHECK(r({"http://mem/slow.x3d", "http://mem/protos.x3d#Beta"}, "") ==
          nullptr);
    // Stopped at the Pending candidate; never reached the resolvable one.
    CHECK(calls == std::vector<std::string>{"http://mem/slow.x3d"});
  }

  SUBCASE("null resolver -> Failed -> null") {
    ProtoDeclarationResolver r = protoResolverFrom(AssetResolver{});
    CHECK(r({"http://mem/protos.x3d#Beta"}, "") == nullptr);
  }

  SUBCASE("self-referencing EXTERNPROTO terminates -> null") {
    // The document's own EXTERNPROTO names the document it lives in; without a
    // cycle guard the adapter re-fetches and re-parses it without bound.
    const std::map<std::string, std::string> cycle = {
        {"memA", "<X3D version='4.0'><Scene>"
                 "<ExternProtoDeclare name='PA' url='memA#PA'/>"
                 "<ProtoInstance name='PA'/></Scene></X3D>"}};
    ProtoDeclarationResolver r =
        protoResolverFrom(inMemory(cycle, {}, nullptr));
    CHECK(r({"memA#PA"}, "") == nullptr);
  }

  SUBCASE("mutually-referencing EXTERNPROTO (A->B->A) terminates -> null") {
    const std::map<std::string, std::string> cycle = {
        {"memA", "<X3D version='4.0'><Scene>"
                 "<ExternProtoDeclare name='PA' url='memB#PB'/>"
                 "<ProtoInstance name='PA'/></Scene></X3D>"},
        {"memB", "<X3D version='4.0'><Scene>"
                 "<ExternProtoDeclare name='PB' url='memA#PA'/>"
                 "<ProtoInstance name='PB'/></Scene></X3D>"}};
    ProtoDeclarationResolver r =
        protoResolverFrom(inMemory(cycle, {}, nullptr));
    CHECK(r({"memA#PA"}, "") == nullptr);
  }

  SUBCASE("a scheme router plugs the http backend; urn stays unresolved") {
    std::vector<std::string> calls;
    AssetResolver router = x3d::runtime::extract::makeSchemeRouter(
        {{"http", inMemory(docs, {}, &calls)}});
    ProtoDeclarationResolver r = protoResolverFrom(router);

    auto d = r({"http://mem/protos.x3d#Beta"}, "");
    REQUIRE(d != nullptr);
    CHECK(d->name == "Beta");

    // No "urn" entry in the router -> Failed without a backend call.
    calls.clear();
    CHECK(r({"urn:x3d:proto:Beta"}, "") == nullptr);
    CHECK(calls.empty());
  }
}

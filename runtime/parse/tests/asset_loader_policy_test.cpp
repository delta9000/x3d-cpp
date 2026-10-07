#include "AssetInlineResolver.hpp"
#include "AssetProtoResolver.hpp"
#include "PathConfine.hpp"
#include "doctest/doctest.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <cerrno>
#include <sys/inotify.h>
#include <unistd.h>
#endif

namespace {
using namespace x3d::codec;
using x3d::runtime::extract::AssetKind;
using x3d::runtime::extract::AssetResolver;
using x3d::runtime::extract::AssetResult;
using Calls = std::vector<std::pair<std::string, AssetKind>>;

std::string scene(const std::string &body) {
  return "<X3D version='4.0'><Scene>" + body + "</Scene></X3D>";
}
std::string proto(const std::string &name, const std::string &body) {
  return "<ProtoDeclare name='" + name + "'><ProtoBody>" + body +
         "</ProtoBody></ProtoDeclare>";
}
std::string external(const std::string &name, const std::string &url) {
  return "<ExternProtoDeclare name='" + name + "' url='&quot;" + url +
         "&quot;'/><ProtoInstance name='" + name + "'/>";
}
AssetResolver memory(const std::map<std::string, std::string> &docs, Calls &calls) {
  return [docs, &calls](const std::string &url, AssetKind kind) {
    calls.emplace_back(url, kind);
    const auto found = docs.find(url);
    if (found == docs.end()) return AssetResult::makeFailed();
    return AssetResult::makeReady({found->second.begin(), found->second.end()});
  };
}
// Linux proof observes the generated decoy itself, without tracing processes
// or requiring privileged filesystem/network access. Other platforms retain
// the instrumented-resolver assertions below.
struct ReadWatch {
#if defined(__linux__)
  int fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  explicit ReadWatch(const std::string &file) {
    REQUIRE(fd >= 0);
    REQUIRE(inotify_add_watch(fd, file.c_str(), IN_OPEN | IN_ACCESS) >= 0);
  }
  ~ReadWatch() { if (fd >= 0) close(fd); }
  void expectRead() const {
    alignas(inotify_event) char events[4096];
    CHECK(read(fd, events, sizeof(events)) > 0);
  }
  void expectUnread() const {
    alignas(inotify_event) char events[4096];
    errno = 0;
    const auto count = read(fd, events, sizeof(events));
    CHECK(count == -1);
    CHECK(errno == EAGAIN);
  }
#else
  explicit ReadWatch(const std::string &) {}
  void expectUnread() const {}
  void expectRead() const {}
#endif
};

struct Fixtures {
  std::filesystem::path root;
  Fixtures() {
    root = std::filesystem::current_path() /
        ("asset-loader-policy-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root / "nested");
  }
  ~Fixtures() { std::error_code ec; std::filesystem::remove_all(root, ec); }
  void write(const std::string &name, const std::string &body) const {
    std::ofstream file(root / name); file << body;
    REQUIRE(file.good());
  }
  std::string path(const std::string &name = "") const {
    return name.empty() ? root.generic_string() : (root / name).generic_string();
  }
};
}

TEST_CASE("asset policy: legacy proto helper never reads a nested local Inline") {
  Fixtures files;
  files.write("forbidden-inline.x3d", scene("<Box/>"));
  ReadWatch watch(files.path("forbidden-inline.x3d"));
  Calls calls;
  const auto r = protoResolverFrom(memory({{files.path("library.x3d"),
      scene(proto("Widget", "<Group/>") +
            "<Inline url='&quot;forbidden-inline.x3d&quot;'/>")}}, calls));
  REQUIRE(r({"library.x3d#Widget"}, files.path()) != nullptr);
  CHECK(calls == Calls{{files.path("library.x3d"), AssetKind::ExternProto},
                      {files.path("forbidden-inline.x3d"), AssetKind::Inline}});
  watch.expectUnread(); // existing on-disk decoy was neither opened nor read
}

TEST_CASE("asset policy: explicit rejecting Inline resolver survives nested EXTERNPROTO") {
  Fixtures files;
  files.write("forbidden-inline.x3d", scene("<Box/>"));
  files.write("nested/forbidden-inline.x3d", scene("<Sphere/>"));
  ReadWatch outer(files.path("forbidden-inline.x3d"));
  ReadWatch inner(files.path("nested/forbidden-inline.x3d"));
  Calls calls;
  std::vector<std::string> bases;
  x3d::runtime::InlineResolver reject = [&](const auto &urls, const auto &base) {
    CHECK(urls == std::vector<std::string>{"forbidden-inline.x3d"});
    bases.push_back(base);
    return std::shared_ptr<x3d::runtime::Scene>{};
  };
  const auto r = protoResolverFrom(memory({
      {files.path("library.x3d"), scene(proto("Widget", "<Inline url='&quot;forbidden-inline.x3d&quot;'/>") +
          external("Nested", "nested/other.x3d#Nested") +
          "<Inline url='&quot;forbidden-inline.x3d&quot;'/>")},
      {files.path("nested/other.x3d"), scene(proto("Nested", "<Group/>") +
          "<Inline url='&quot;forbidden-inline.x3d&quot;'/>")}}, calls),
      Encoding::Unknown, reject);
  auto doc = parseDocument(scene(external("Widget", "library.x3d#Widget")),
      Encoding::XML, files.path(), r, reject);
  CHECK(doc.protoWarnings.empty());
  CHECK(doc.inlineWarnings.size() == 1);
  CHECK(bases == std::vector<std::string>{files.path("nested"), files.path(), files.path()});
  CHECK(calls == Calls{{files.path("library.x3d"), AssetKind::ExternProto},
                      {files.path("nested/other.x3d"), AssetKind::ExternProto}});
  outer.expectUnread();
  inner.expectUnread();
}

TEST_CASE("asset policy: paired loaders preserve fetched body and nested source bases") {
  Calls calls;
  const auto pair = assetResolversFrom(memory({
      {"memory://project/scenes/child.x3d", scene(external("Panel", "../protos/outer.x3d#Panel"))},
      {"memory://project/protos/outer.x3d", scene(proto("First", "<Box/>") +
          proto("Panel", "<Group><Inline url='&quot;tiles/panel.x3d&quot;'/>" +
                external("Inner", "parts/inner.x3d#Inner") + "</Group>"))},
      {"memory://project/protos/parts/inner.x3d", scene(proto("Inner",
          "<Inline url='&quot;../tiles/inner.x3d&quot;'/>") )},
      {"memory://project/protos/tiles/panel.x3d", scene("<Box/>")},
      {"memory://project/protos/tiles/inner.x3d", scene("<Sphere/>")}}, calls));
  auto doc = parseDocument(scene("<Inline url='&quot;scenes/child.x3d&quot;'/>") ,
      Encoding::XML, "memory://project", pair.proto, pair.inlineScene);
  CHECK(doc.protoWarnings.empty());
  CHECK(doc.inlineWarnings.empty());
  CHECK(calls == Calls{
      {"memory://project/scenes/child.x3d", AssetKind::Inline},
      {"memory://project/protos/outer.x3d", AssetKind::ExternProto},
      {"memory://project/protos/parts/inner.x3d", AssetKind::ExternProto},
      {"memory://project/protos/tiles/panel.x3d", AssetKind::Inline},
      {"memory://project/protos/tiles/inner.x3d", AssetKind::Inline}});
  // The selected declaration can also be instantiated independently.
  auto inner = parseDocument(scene(external("Inner", "parts/inner.x3d#Inner")),
      Encoding::XML, "memory://project/protos", pair.proto, pair.inlineScene);
  CHECK(inner.inlineWarnings.empty());
  CHECK(calls.back() == std::pair{std::string("memory://project/protos/tiles/inner.x3d"), AssetKind::Inline});
  REQUIRE(inner.scene.expandedInlines.size() == 1);
  CHECK(x3d::runtime::inline_detail::readUrl(*inner.scene.expandedInlines.begin()->second)
        == std::vector<std::string>{"../tiles/inner.x3d"});
}

TEST_CASE("asset policy: cached selected declarations retain origin across importers") {
  Calls calls;
  const auto pair = assetResolversFrom(memory({
      {"memory://library/protos.x3d", scene(proto("First", "<Box/>") +
          proto("Selected", "<Inline url='&quot;child.x3d&quot;'/>") )},
      {"memory://library/child.x3d", scene("<Sphere/>")}}, calls));
  auto cached = pair.proto({"memory://library/protos.x3d#Selected"}, "memory://unused");
  REQUIRE(cached != nullptr);
  REQUIRE(cached->sourceBaseUrl.has_value());
  CHECK(*cached->sourceBaseUrl == "memory://library");
  ProtoDeclarationResolver cache = [cached](const auto &, const auto &) { return cached; };
  for (const auto &base : {"memory://importer/one", "memory://importer/two"}) {
    auto doc = parseDocument(scene(external("Selected", "cached#Selected")),
        Encoding::XML, base, cache, pair.inlineScene);
    CHECK(doc.inlineWarnings.empty());
    REQUIRE(doc.scene.expandedInlines.size() == 1);
    CHECK(x3d::runtime::inline_detail::readUrl(*doc.scene.expandedInlines.begin()->second)
          == std::vector<std::string>{"child.x3d"});
  }
  CHECK(calls == Calls{{"memory://library/protos.x3d", AssetKind::ExternProto},
                      {"memory://library/child.x3d", AssetKind::Inline},
                      {"memory://library/child.x3d", AssetKind::Inline}});
}

TEST_CASE("asset policy: caller URL override keeps caller origin through IS") {
  Calls calls;
  const auto pair = assetResolversFrom(memory({
      {"memory://library/widget.x3d", scene(
          "<ProtoDeclare name='Widget'><ProtoInterface><field name='source' type='MFString' "
          "accessType='inputOutput' value='&quot;default.x3d&quot;'/></ProtoInterface>"
          "<ProtoBody><Inline><IS><connect nodeField='url' protoField='source'/></IS>"
          "</Inline></ProtoBody></ProtoDeclare>")},
      {"memory://caller/chosen.x3d", scene("<Box/>")}}, calls));
  const auto doc = parseDocument(scene(
      "<ExternProtoDeclare name='Widget' url='&quot;memory://library/widget.x3d#Widget&quot;'>"
      "<field name='source' type='MFString' accessType='inputOutput'/></ExternProtoDeclare>"
      "<ProtoInstance name='Widget'><fieldValue name='source' value='&quot;chosen.x3d&quot;'/></ProtoInstance>"),
      Encoding::XML, "memory://caller", pair.proto, pair.inlineScene);
  CHECK(doc.inlineWarnings.empty());
  CHECK(calls == Calls{{"memory://library/widget.x3d", AssetKind::ExternProto},
                      {"memory://caller/chosen.x3d", AssetKind::Inline}});
}

TEST_CASE("asset policy: Inline helper retains an explicit rejecting EXTERNPROTO policy") {
  Calls calls;
  std::vector<std::string> bases;
  ProtoDeclarationResolver reject = [&](const auto &, const auto &base) {
    bases.push_back(base);
    return std::shared_ptr<x3d::runtime::ProtoDeclaration>{};
  };
  const auto r = inlineResolverFrom(memory({
      {"memory://tree/root.x3d", scene("<Inline url='&quot;nested/child.x3d&quot;'/>")},
      {"memory://tree/nested/child.x3d", scene(external("Forbidden", "secret.x3d#Forbidden"))}}, calls),
      Encoding::Unknown, reject);
  REQUIRE(r({"root.x3d"}, "memory://tree") != nullptr);
  CHECK(bases == std::vector<std::string>{"memory://tree/nested"});
  CHECK(calls == Calls{{"memory://tree/root.x3d", AssetKind::Inline},
                      {"memory://tree/nested/child.x3d", AssetKind::Inline}});
}

TEST_CASE("asset policy: mixed-kind and lexical-alias cycles are bounded") {
  Calls calls;
  const auto pair = assetResolversFrom(memory({
      {"memory://tree/a.x3d", scene(proto("A", "<Box/>") +
          "<Inline url='&quot;sub/../b.x3d&quot;'/>")},
      {"memory://tree/b.x3d", scene(external("A", "./a.x3d#A"))}}, calls));
  REQUIRE(pair.proto({"a.x3d#A"}, "memory://tree") != nullptr);
  CHECK(calls == Calls{{"memory://tree/a.x3d", AssetKind::ExternProto},
                      {"memory://tree/b.x3d", AssetKind::Inline}});
  calls.clear();
  REQUIRE(pair.proto({"a.x3d#A"}, "memory://tree") != nullptr);
  CHECK(calls.size() == 2); // cycle state belongs to one call, not the factory
}

TEST_CASE("asset policy: nested Pending aborts parents and candidate fallback") {
  Calls calls;
  AssetResolver assets = [&](const std::string &url, AssetKind kind) {
    calls.emplace_back(url, kind);
    if (url == "memory://tree/slow.x3d") return AssetResult::makePending();
    const auto text = scene(proto("A", "<Box/>") +
        "<Inline url='&quot;slow.x3d&quot; &quot;forbidden-fallback.x3d&quot;'/>");
    return AssetResult::makeReady({text.begin(), text.end()});
  };
  const auto pair = assetResolversFrom(assets);
  CHECK(pair.proto({"root.x3d#A", "forbidden-parent-fallback.x3d#A"}, "memory://tree") == nullptr);
  CHECK(calls == Calls{{"memory://tree/root.x3d", AssetKind::ExternProto},
                      {"memory://tree/slow.x3d", AssetKind::Inline}});
  const auto doc = parseDocument(scene(external("A", "root.x3d#A")),
      Encoding::XML, "memory://tree", pair.proto, pair.inlineScene);
  REQUIRE(doc.protoWarnings.size() == 1);
  CHECK(doc.protoWarnings.front().kind == x3d::runtime::ProtoWarning::Kind::UnresolvedExtern);
}

TEST_CASE("asset policy: failed malformed and throwing candidates fall back without I/O") {
  Calls calls;
  AssetResolver assets = [&](const std::string &url, AssetKind kind) {
    calls.emplace_back(url, kind);
    if (url == "memory://tree/throws.x3d") throw std::runtime_error("backend rejected");
    if (url == "memory://tree/failed.x3d") return AssetResult::makeFailed();
    const auto text = url == "memory://tree/malformed.x3d" ? std::string("<not-valid")
        : scene(proto("Good", "<Box/>"));
    return AssetResult::makeReady({text.begin(), text.end()});
  };
  const auto r = protoResolverFrom(assets);
  REQUIRE(r({"throws.x3d", "failed.x3d", "malformed.x3d", "good.x3d#Good"}, "memory://tree") != nullptr);
  CHECK(calls.size() == 4);
  CHECK(protoResolverFrom(AssetResolver{})({"any.x3d"}, "memory://tree") == nullptr);
  CHECK(inlineResolverFrom(AssetResolver{})({"any.x3d"}, "memory://tree") == nullptr);
}

TEST_CASE("asset policy: local Inline confinement rejects escape and preserves allowed local fixtures") {
  Fixtures files;
  files.write("allowed.x3d", scene("<Box/>"));
  ReadWatch positiveControl(files.path("allowed.x3d"));
  const auto denied = parseDocument(scene("<Inline url='&quot;../allowed.x3d&quot;'/>") ,
      Encoding::XML, files.path("nested"));
  CHECK(denied.inlineWarnings.size() == 1);
  const auto absolute = parseDocument(scene("<Inline url='&quot;" + files.path("allowed.x3d") + "&quot;'/>") ,
      Encoding::XML, files.path());
  CHECK(absolute.inlineWarnings.size() == 1);
  positiveControl.expectUnread();
  const auto allowed = parseDocument(scene("<Inline url='&quot;allowed.x3d&quot;'/>") ,
      Encoding::XML, files.path());
  CHECK(allowed.inlineWarnings.empty());
  CHECK(allowed.scene.expandedInlines.size() == 1);
  positiveControl.expectRead(); // the file-event audit detects a permitted read
}

TEST_CASE("asset policy: URL resolution retains authority query and significant path segments") {
  using asset_document_detail::resolveUrl;
  CHECK(resolveUrl("../p.x3d?rev=2", "memory://host/a/b") == "memory://host/a/p.x3d?rev=2");
  CHECK(resolveUrl("/p.x3d", "memory://host/a") == "memory://host/p.x3d");
  CHECK(resolveUrl("//other/p.x3d", "memory://host/a") == "memory://other/p.x3d");
  CHECK(resolveUrl("urn:example:Proto", "memory://host/a") == "urn:example:Proto");
  CHECK(resolveUrl("a//b.x3d", "memory://host") == "memory://host/a//b.x3d");
  CHECK(resolveUrl("p.x3d", "/") == "/p.x3d");
  CHECK(resolveUrl("/leaf.x3d", "//host/parent") == "//host/leaf.x3d");
  CHECK(resolveUrl("/leaf.x3d", "//host") == "//host/leaf.x3d");
  CHECK(resolveUrl("/leaf.x3d", "//host?query=/unrelated") == "//host/leaf.x3d");
  CHECK(resolveUrl("/leaf.x3d", "/local/tree") == "/leaf.x3d");
}

TEST_CASE("asset policy: explicit empty opposite policies reject without default fallback") {
  Calls calls;
  const auto assets = memory({
      {"memory://tree/proto.x3d", scene(proto("P", "<Box/>") +
          "<Inline url='&quot;forbidden.x3d&quot;'/>")},
      {"memory://tree/inline.x3d", scene(external("P", "forbidden.x3d#P"))}}, calls);
  const auto p = protoResolverFrom(assets, Encoding::Unknown, x3d::runtime::InlineResolver{});
  REQUIRE(p({"proto.x3d#P"}, "memory://tree") != nullptr);
  const auto i = inlineResolverFrom(assets, Encoding::Unknown, ProtoDeclarationResolver{});
  REQUIRE(i({"inline.x3d"}, "memory://tree") != nullptr);
  CHECK(calls == Calls{{"memory://tree/proto.x3d", AssetKind::ExternProto},
                      {"memory://tree/inline.x3d", AssetKind::Inline}});
}

TEST_CASE("asset policy: nested EXTERNPROTO Pending stops its Inline parent") {
  Calls calls;
  AssetResolver assets = [&](const std::string &url, AssetKind kind) {
    calls.emplace_back(url, kind);
    if (kind == AssetKind::ExternProto) return AssetResult::makePending();
    const auto text = scene(external("Slow", "slow.x3d#Slow"));
    return AssetResult::makeReady({text.begin(), text.end()});
  };
  const auto pair = assetResolversFrom(assets);
  auto doc = parseDocument(scene(
      "<Inline url='&quot;parent.x3d&quot; &quot;forbidden-fallback.x3d&quot;'/>") ,
      Encoding::XML, "memory://tree", pair.proto, pair.inlineScene);
  REQUIRE(doc.inlineWarnings.size() == 1);
  CHECK(doc.inlineWarnings.front().kind == x3d::runtime::InlineWarning::Kind::UnresolvedUrl);
  CHECK(calls == Calls{{"memory://tree/parent.x3d", AssetKind::Inline},
                      {"memory://tree/slow.x3d", AssetKind::ExternProto}});
}

TEST_CASE("asset policy: empty captured source base does not inherit a later importer") {
  Calls calls;
  const auto pair = assetResolversFrom(memory({
      {"protos.x3d", scene(proto("P", "<Inline url='&quot;child.x3d&quot;'/>") )},
      {"child.x3d", scene("<Box/>")}}, calls));
  const auto cached = pair.proto({"protos.x3d#P"}, "");
  REQUIRE(cached != nullptr);
  REQUIRE(cached->sourceBaseUrl.has_value());
  CHECK(cached->sourceBaseUrl->empty());
  const auto doc = parseDocument(scene(external("P", "cached")), Encoding::XML,
      "memory://unrelated/importer", [cached](const auto &, const auto &) { return cached; },
      pair.inlineScene);
  CHECK(doc.inlineWarnings.empty());
  CHECK(calls == Calls{{"protos.x3d", AssetKind::ExternProto}, {"child.x3d", AssetKind::Inline}});
}

TEST_CASE("asset policy: URL normalization preserves directory ends and query fragment bytes") {
  using asset_document_detail::normalizeUrl;
  using asset_document_detail::resolveUrl;
  const std::vector<std::pair<std::string, std::string>> cases = {
      {"memory://host/parent/child/.", "memory://host/parent/child/"},
      {"memory://host/parent/child/..", "memory://host/parent/"},
      {"memory://host/.", "memory://host/"},
      {"memory://host/..", "memory://host/"},
      {"memory://host/child/../..", "memory://host/"},
      {"/.", "/"}, {"/..", "/"}, {"/child/..", "/"},
      {"child/.", "child/"}, {"child/..", "./"},
      {".", "./"}, {"..", "../"}, {"../..", "../../"},
      {"//host/parent/child/.", "//host/parent/child/"},
      {"//host/parent/child/..", "//host/parent/"},
      {"//host/..", "//host/"},
      {"//host?query=/a/../b", "//host?query=/a/../b"},
      {"//host#fragment/a/../b", "//host#fragment/a/../b"},
      {"//host?query=/a/../b#fragment/./c", "//host?query=/a/../b#fragment/./c"},
      {"//host/child/.?query=/a/../b#fragment/./c", "//host/child/?query=/a/../b#fragment/./c"},
      {"memory://host/child/..?query=/a/../b#fragment/./c", "memory://host/?query=/a/../b#fragment/./c"},
      {"child/.#fragment/../?query=/a/../b", "child/#fragment/../?query=/a/../b"},
      {"urn:example:a/../b?query=/x/../y#fragment/./z", "urn:example:a/../b?query=/x/../y#fragment/./z"}};
  for (const auto &[source, expected] : cases) {
    CAPTURE(source);
    CHECK(normalizeUrl(source) == expected);
  }
  CHECK(resolveUrl("child/.", "memory://host/parent") == "memory://host/parent/child/");
  CHECK(resolveUrl("child/..", "memory://host/parent") == "memory://host/parent/");
  CHECK(resolveUrl("//host?query=/a/../b", "") == "//host?query=/a/../b");
}

TEST_CASE("asset policy: fetched directory URLs give nested Inline its exact source directory") {
  struct Case { std::string raw, base, fetched, leaf; };
  const std::vector<Case> cases = {
      {"child/.", "memory://host/parent", "memory://host/parent/child/", "memory://host/parent/child/leaf.x3d"},
      {"child/..", "memory://host/parent", "memory://host/parent/", "memory://host/parent/leaf.x3d"},
      {".", "memory://host", "memory://host/", "memory://host/leaf.x3d"},
      {"/..", "memory://host/parent", "memory://host/", "memory://host/leaf.x3d"},
      {"child/..", "", "./", "leaf.x3d"},
      {"/child/..", "", "/", "/leaf.x3d"},
      {"//host?query=/a/../b", "", "//host?query=/a/../b", "//host/leaf.x3d"},
      {"//host/parent/child/..", "", "//host/parent/", "//host/parent/leaf.x3d"}};
  for (const auto &item : cases) {
    for (const bool throughProto : {false, true}) {
      CAPTURE(item.raw);
      CAPTURE(item.base);
      CAPTURE(throughProto);
      Calls calls;
      const std::string body = "<Inline url='&quot;leaf.x3d&quot;'/>";
      const auto pair = assetResolversFrom(memory({
          {item.fetched, scene(throughProto ? proto("P", body) : body)},
          {item.leaf, scene("<Box/>")}}, calls));
      if (throughProto) {
        const auto doc = parseDocument(scene(external("P", item.raw + "#P")),
            Encoding::XML, item.base, pair.proto, pair.inlineScene);
        CHECK(doc.protoWarnings.empty());
        CHECK(doc.inlineWarnings.empty());
        CHECK(doc.scene.expandedInlines.size() == 1);
      } else {
        const auto loaded = pair.inlineScene({item.raw}, item.base);
        REQUIRE(loaded != nullptr);
        CHECK(loaded->expandedInlines.size() == 1);
      }
      CHECK(calls == Calls{{item.fetched, throughProto ? AssetKind::ExternProto : AssetKind::Inline},
                          {item.leaf, AssetKind::Inline}});
    }
  }
}

TEST_CASE("asset policy: rooted Inline URLs keep scheme-relative source authority") {
  for (const auto &source : {"//host/parent/library.x3d", "//host"}) {
    Calls calls;
    const auto pair = assetResolversFrom(memory({
        {source, scene("<Inline url='&quot;/leaf.x3d&quot;'/>")},
        {"//host/leaf.x3d", scene("<Box/>")}}, calls));
    const auto loaded = pair.inlineScene({source}, "");
    REQUIRE(loaded != nullptr);
    CHECK(loaded->expandedInlines.size() == 1);
    CHECK(calls == Calls{{source, AssetKind::Inline},
                        {"//host/leaf.x3d", AssetKind::Inline}});
  }
}

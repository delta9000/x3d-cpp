#include "X3DParse.hpp"
#include "Inflate.hpp"
#include "AssetDocumentResolvers.hpp"
#include "doctest/doctest.h"
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace x3d::codec;
namespace {
struct Files {
  std::filesystem::path dir = std::filesystem::temp_directory_path() /
      ("x3d-input-budget-" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  Files() { std::filesystem::create_directory(dir); }
  ~Files() { std::error_code ec; std::filesystem::remove_all(dir, ec); }
  std::string put(const std::string &name, const std::string &bytes) {
    auto path = dir / name;
    std::ofstream(path, std::ios::binary) << bytes;
    return path.string();
  }
};
std::string scene(const std::string &body) {
  return "<X3D><Scene>" + body + "</Scene></X3D>";
}
std::string inl(const std::string &name) { return "<Inline url='\"" + name + "\"'/>"; }
std::string gzip(const std::string &payload) {
  std::string out("\x1f\x8b\x08\0\0\0\0\0\0\xff", 10);
  out += '\1';
  const auto len = static_cast<unsigned>(payload.size());
  for (unsigned v : {len, (~len & 65535u)}) {
    out += static_cast<char>(v); out += static_cast<char>(v >> 8);
  }
  out += payload;
  const auto crc = x3d::codec::detail::crc32(
      reinterpret_cast<const unsigned char *>(payload.data()), payload.size());
  for (unsigned v : {crc, len})
    for (int i = 0; i < 4; ++i) out += static_cast<char>(v >> (i * 8));
  return out;
}
}
TEST_CASE("input budget: exact memory byte boundary and reset") {
  const auto text = scene("<Group/>");
  CHECK_NOTHROW(parseDocument(text, Encoding::Unknown, "", localFileProtoResolver,
                             localFileInlineResolver, "", {1, text.size()}));
  CHECK_THROWS_AS(parseDocument(text, Encoding::Unknown, "", localFileProtoResolver,
      localFileInlineResolver, "", {1, text.size()-1}), x3d::ParseResourceLimitError);
  CHECK_THROWS_AS(parseDocument(text, Encoding::Unknown, "", localFileProtoResolver,
      localFileInlineResolver, "", {0, text.size()}), x3d::ParseResourceLimitError);
  CHECK_NOTHROW(parseDocument(text));
}
TEST_CASE("input budget: Inline depth boundary, fallback and reset") {
  Files f;
  f.put("c.x3d", scene("<Group/>"));
  f.put("b.x3d", scene(inl("c.x3d")));
  auto a = f.put("a.x3d", scene(inl("b.x3d")));
  CHECK_NOTHROW(parseFile(a, "", {3, 10000}));
  CHECK_THROWS_AS(parseFile(a, "", {2, 10000}), x3d::ParseResourceLimitError);
  CHECK_NOTHROW(parseFile(a, "", {3, 10000}));
  auto fallback = f.put("fallback.x3d", scene("<Inline url='\"missing.x3d\" \"c.x3d\"'/>"));
  CHECK(parseFile(fallback).inlineWarnings.empty());
  const std::string bad = "not an X3D document";
  f.put("bad.x3d", bad);
  const auto fallbackText = scene("<Inline url='\"bad.x3d\" \"c.x3d\"'/>");
  auto malformed = f.put("fallback.x3d", fallbackText);
  const auto total = fallbackText.size() + bad.size() + scene("<Group/>").size();
  CHECK(parseFile(malformed, "", {2, total}).inlineWarnings.empty());
  CHECK_THROWS_AS(parseFile(malformed, "", {2, total-1}), x3d::ParseResourceLimitError);
}
TEST_CASE("input budget: cumulative siblings and raw file boundary") {
  Files f;
  auto leaf = scene("<Group/>");
  f.put("leaf.x3d", leaf);
  auto root = scene(inl("leaf.x3d") + inl("leaf.x3d"));
  auto path = f.put("root.x3d", root);
  const auto total = root.size() + 2 * leaf.size();
  CHECK_NOTHROW(parseFile(path, "", {2, total}));
  CHECK_THROWS_AS(parseFile(path, "", {2, total-1}), x3d::ParseResourceLimitError);
  auto leafPath = (f.dir / "leaf.x3d").string();
  CHECK_NOTHROW(parseFile(leafPath, "", {1, leaf.size()}));
  CHECK_THROWS_AS(parseFile(leafPath, "", {1, leaf.size()-1}), x3d::ParseResourceLimitError);
}
TEST_CASE("input budget: mixed Inline and EXTERNPROTO share fatal depth limit") {
  Files f;
  f.put("c.x3d", "<X3D><Scene><ProtoDeclare name='P'><ProtoBody><Group/></ProtoBody></ProtoDeclare></Scene></X3D>");
  f.put("b.x3d", scene("<ExternProtoDeclare name='P' url='\"c.x3d#P\"'/><ProtoInstance name='P'/>"));
  auto a = f.put("a.x3d", scene(inl("b.x3d")));
  CHECK_NOTHROW(parseFile(a, "", {3, 10000}));
  CHECK_THROWS_AS(parseFile(a, "", {2, 10000}), x3d::ParseResourceLimitError);
}
TEST_CASE("input budget: gzip counts compressed and decoded bytes") {
  Files f;
  const auto text = scene("<Group/>");
  const auto packed = gzip(text);
  auto path = f.put("leaf.x3d.gz", packed);
  CHECK_NOTHROW(parseFile(path, "", {1, packed.size()+text.size()}));
  CHECK_THROWS_AS(parseFile(path, "", {1, packed.size()+text.size()-1}), x3d::ParseResourceLimitError);
  CHECK_THROWS_AS(parseFile(path, "", {1, packed.size()}), x3d::ParseResourceLimitError);
  CHECK_THROWS_AS(inflateGzip(packed, 0), x3d::ParseResourceLimitError);
  auto root = f.put("root.x3d", scene(inl("leaf.x3d.gz")));
  CHECK_THROWS_AS(parseFile(root, "", {2, std::filesystem::file_size(root)+packed.size()+text.size()-1}), x3d::ParseResourceLimitError);
  CHECK_NOTHROW(parseFile(root));
}

TEST_CASE("input budget: asset adapters cannot swallow shared exhaustion") {
  using namespace x3d::runtime::extract;
  const auto leaf = scene("<Group/>");
  auto pair = assetResolversFrom([&](const std::string &, AssetKind) {
    return AssetResult::makeReady({leaf.begin(), leaf.end()});
  });
  auto root = scene(inl("leaf.x3d"));
  CHECK_THROWS_AS(parseDocument(root, Encoding::Unknown, "", pair.proto,
      pair.inlineScene, "", {1, 10000}), x3d::ParseResourceLimitError);
  CHECK_NOTHROW(parseDocument(root, Encoding::Unknown, "", pair.proto,
      pair.inlineScene, "", {2, root.size()+leaf.size()}));
}

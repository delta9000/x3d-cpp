// path_confine_test.cpp
//  /  regression for confineLocalIncludePath (PathConfine.hpp).
//   a file-like Inline/EXTERNPROTO url that is absolute, or that escapes
//          the configured confinement root, is rejected (no arbitrary read).
//   spelling variants of one file canonicalize to a single key, so the
//          cycle guard cannot be aliased past.
// The confinement root is configurable (ADR-0038): a tight root (the source
// file's own dir) blocks `../` cross-directory refs; a broader root (a trusted
// content tree) admits `../` that stays within it. Both still block absolute
// paths and escapes above the root.
// Hermetic: builds a temp directory layout, asserts, tears it down.

#include "doctest/doctest.h"
#include "PathConfine.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace x3d::codec;
namespace fs = std::filesystem;

namespace {
void touch(const fs::path &p) {
  fs::create_directories(p.parent_path());
  std::ofstream(p) << "<X3D/>\n";
}

} // namespace

TEST_CASE("confineLocalIncludePath") {
  // <root>/Content/{base/{ok.x3d, sub/deep.x3d}, Tools/proto.x3d}  and a file
  // <root>/outside.x3d that sits above the Content tree.
  const fs::path root = fs::temp_directory_path() / "x3d_path_confine_test";
  std::error_code ec;
  fs::remove_all(root, ec);
  touch(root / "Content" / "base" / "ok.x3d");
  touch(root / "Content" / "base" / "sub" / "deep.x3d");
  touch(root / "Content" / "Tools" / "proto.x3d");
  touch(root / "outside.x3d");
  const std::string base = (root / "Content" / "base").string();
  const std::string content = (root / "Content").string();

  SUBCASE("a tight root admits the source subtree only") {
    // includes within the source subtree resolve.
    CHECK_MESSAGE((confineLocalIncludePath("ok.x3d", base, base).has_value()),
                  "tight root: sibling within source dir resolves");
    CHECK_MESSAGE((confineLocalIncludePath("sub/deep.x3d", base, base).has_value()),
                  "tight root: subdirectory within source dir resolves");
    // `../` cross-directory and absolute are rejected under a tight root.
    CHECK_MESSAGE((!confineLocalIncludePath("../Tools/proto.x3d", base, base).has_value()),
                  "tight root: ../ cross-directory ref is blocked");
    CHECK_MESSAGE((!confineLocalIncludePath("/etc/passwd", base, base).has_value()),
                  "tight root: absolute path is blocked");
  }

  SUBCASE("a broad root admits ../ inside it but not escapes") {
    // `../` is admitted when it stays within the broader root...
    CHECK_MESSAGE((confineLocalIncludePath("../Tools/proto.x3d", base, content).has_value()),
                  "broad root: ../ within the content tree resolves");
    // ...but absolute paths and escapes above the root are still rejected.
    CHECK_MESSAGE((!confineLocalIncludePath("../../outside.x3d", base, content).has_value()),
                  "broad root: escape above the content root is blocked");
    CHECK_MESSAGE((!confineLocalIncludePath("/etc/passwd", base, content).has_value()),
                  "broad root: absolute path is still blocked");
  }

  SUBCASE("equivalent spellings canonicalize to one key") {
    const auto a = confineLocalIncludePath("ok.x3d", base, base);
    const auto b = confineLocalIncludePath("./ok.x3d", base, base);
    const auto c = confineLocalIncludePath("sub/../ok.x3d", base, base);
    CHECK_MESSAGE((a && b && c && *a == *b && *b == *c),
                  "./x, x and y/../x canonicalize to one cycle-guard key");
  }

  fs::remove_all(root, ec);
}

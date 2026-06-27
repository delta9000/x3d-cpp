// path_confine_test.cpp
// SEC-3 / SEC-4 regression for confineLocalIncludePath (PathConfine.hpp).
//   SEC-3: a file-like Inline/EXTERNPROTO url that is absolute, or that escapes
//          the configured confinement root, is rejected (no arbitrary read).
//   SEC-4: spelling variants of one file canonicalize to a single key, so the
//          cycle guard cannot be aliased past.
// The confinement root is configurable (ADR-0038): a tight root (the source
// file's own dir) blocks `../` cross-directory refs; a broader root (a trusted
// content tree) admits `../` that stays within it. Both still block absolute
// paths and escapes above the root.
// Hermetic: builds a temp directory layout, asserts, tears it down.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "PathConfine.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace x3d::codec;
namespace fs = std::filesystem;

namespace {

int failures = 0;
void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

void touch(const fs::path &p) {
  fs::create_directories(p.parent_path());
  std::ofstream(p) << "<X3D/>\n";
}

} // namespace

int main() {
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

  // ---- Tight root = the source dir (secure default) -----------------------
  // SEC-3: includes within the source subtree resolve.
  check(confineLocalIncludePath("ok.x3d", base, base).has_value(),
        "tight root: sibling within source dir resolves");
  check(confineLocalIncludePath("sub/deep.x3d", base, base).has_value(),
        "tight root: subdirectory within source dir resolves");
  // SEC-3: `../` cross-directory and absolute are rejected under a tight root.
  check(!confineLocalIncludePath("../Tools/proto.x3d", base, base).has_value(),
        "tight root: ../ cross-directory ref is blocked");
  check(!confineLocalIncludePath("/etc/passwd", base, base).has_value(),
        "tight root: absolute path is blocked");

  // ---- Broader root = a trusted content tree ------------------------------
  // SEC-3: `../` is admitted when it stays within the broader root...
  check(confineLocalIncludePath("../Tools/proto.x3d", base, content).has_value(),
        "broad root: ../ within the content tree resolves");
  // ...but absolute paths and escapes ABOVE the root are still rejected.
  check(!confineLocalIncludePath("../../outside.x3d", base, content).has_value(),
        "broad root: escape above the content root is blocked");
  check(!confineLocalIncludePath("/etc/passwd", base, content).has_value(),
        "broad root: absolute path is still blocked");

  // ---- SEC-4: spelling variants share a single canonical key --------------
  const auto a = confineLocalIncludePath("ok.x3d", base, base);
  const auto b = confineLocalIncludePath("./ok.x3d", base, base);
  const auto c = confineLocalIncludePath("sub/../ok.x3d", base, base);
  check(a && b && c && *a == *b && *b == *c,
        "./x, x and y/../x canonicalize to one cycle-guard key");

  fs::remove_all(root, ec);

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all path_confine checks passed\n";
  return 0;
}

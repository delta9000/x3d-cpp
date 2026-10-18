// inline_confine_test.cpp
// SEC-3 regression for the DEFAULT resolvers' call sites (X3DParse.cpp), not
// just confineLocalIncludePath itself (that unit is covered by
// path_confine_test.cpp). localFileInlineResolver used to build
// `baseUrl + "/" + url` with only a leading-'/' check, so an Inline url such as
// `../../outside.x3d` escaped the scene directory and was read verbatim — the
// same arbitrary-read the PROTO resolver was already hardened against.
//
// This test drives the real front door (parseFile) over a temp directory
// layout and asserts:
//   1. an Inline `../` escape above the confine root does NOT resolve (no
//      child scene spliced in, no read of the outside file), and
//   2. Inlines that stay inside the root still resolve (the guard must not
//      over-block), and
//   3. the cycle a -> b -> a still terminates (cycle guard intact on the new
//      canonical keys).
//
// Hermetic: builds a temp directory layout, asserts, tears it down.
// Exit code 0 on success; nonzero on any failed assertion.

#include "X3DParse.hpp"

#include <any>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

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

void write(const fs::path &p, const std::string &content) {
  fs::create_directories(p.parent_path());
  std::ofstream(p) << content;
}

const char *kParent = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<X3D profile=\"Interchange\" version=\"4.0\"><head/>"
                      "<Scene>\n"
                      "  <Inline url=\"%url%\"/>\n"
                      "</Scene></X3D>\n";

std::string parentWith(const std::string &url) {
  std::string s = kParent;
  s.replace(s.find("%url%"), 5, url);
  return s;
}

// Count the Inline nodes anywhere in the parsed root scene (roots included).
// When the default resolver refuses (or fails to load) a candidate url the
// Inline stays un-expanded in the parent scene; when it succeeds the child's
// roots are spliced into a synthetic Group and the Inline node is gone.
int countInlines(const x3d::runtime::X3DDocument &doc) {
  int n = 0;
  std::vector<const x3d::nodes::X3DNode *> stack;
  for (const auto &root : doc.scene.rootNodes) {
    if (!root)
      continue;
    if (root->nodeTypeName() == "Inline")
      ++n;
    stack.push_back(root.get());
  }
  std::unordered_set<const x3d::nodes::X3DNode *> walked; // USE-cycle safe
  while (!stack.empty()) {
    const auto *node = stack.back();
    stack.pop_back();
    if (!walked.insert(node).second)
      continue;
    for (const auto &field : node->fields()) {
      if (!field.isReadable() || !field.isNode() || !field.get)
        continue;
      if (field.type == x3d::core::X3DFieldType::SFNode) {
        if (auto child = std::any_cast<std::shared_ptr<x3d::nodes::X3DNode>>(
                field.get(*node))) {
          if (child->nodeTypeName() == "Inline")
            ++n;
          stack.push_back(child.get());
        }
      } else if (field.type == x3d::core::X3DFieldType::MFNode) {
        for (const auto &child :
             std::any_cast<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>>(
                 field.get(*node))) {
          if (child && child->nodeTypeName() == "Inline")
            ++n;
          if (child)
            stack.push_back(child.get());
        }
      }
    }
  }
  return n;
}

} // namespace

int main() {
  // <root>/Content/{parent_escape.x3d, parent_ok.x3d, cycle_a.x3d,
  // cycle_b.x3d, child.x3d} and <root>/outside.x3d above the Content tree.
  // parseFile defaults the confine root to the top-level file's own directory,
  // so Content/ is the root for every parse below.
  const fs::path root = fs::temp_directory_path() / "x3d_inline_confine_test";
  std::error_code ec;
  fs::remove_all(root, ec);
  const fs::path content = root / "Content";
  write(content / "child.x3d",
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<X3D profile=\"Interchange\" version=\"4.0\"><head/><Scene>\n"
        "  <Shape/>\n"
        "</Scene></X3D>\n");
  // A uniquely identifiable payload so an over-permissive resolver would be
  // observable (its DEF would appear in the parent's scene after splicing).
  write(root / "outside.x3d",
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<X3D profile=\"Interchange\" version=\"4.0\"><head/><Scene>\n"
        "  <Shape DEF='ESCAPED_LEAK'/>\n"
        "</Scene></X3D>\n");
  write(content / "parent_escape.x3d", parentWith("../outside.x3d"));
  write(content / "parent_deep_escape.x3d", parentWith("../../outside.x3d"));
  write(content / "parent_absolute.x3d", parentWith(root / "outside.x3d"));
  write(content / "parent_ok.x3d", parentWith("child.x3d"));
  write(content / "parent_dotted.x3d", parentWith("./child.x3d"));
  write(content / "cycle_a.x3d", parentWith("cycle_b.x3d"));
  write(content / "cycle_b.x3d", parentWith("cycle_a.x3d"));

  // (1) SEC-3: escapes above the confine root are refused by the DEFAULT
  // Inline resolver — the Inline stays un-expanded, and the outside file's
  // DEF must not leak into the parent scene.
  {
    auto doc = parseFile((content / "parent_escape.x3d").string());
    check(countInlines(doc) == 1,
          "default Inline resolver: ../ escape above the root is not expanded");
    check(doc.scene.resolve("ESCAPED_LEAK") == nullptr,
          "default Inline resolver: escaped file's contents do not leak");
  }
  {
    auto doc = parseFile((content / "parent_deep_escape.x3d").string());
    check(countInlines(doc) == 1,
          "default Inline resolver: ../../ escape above the root is not expanded");
  }
  {
    auto doc = parseFile((content / "parent_absolute.x3d").string());
    check(countInlines(doc) == 1,
          "default Inline resolver: absolute url is not expanded");
  }

  // (2) The guard must not over-block: a sibling within the root resolves
  // (the Inline is replaced by the child's roots), and a ./ spelling of the
  // same file resolves identically (SEC-4 canonical key).
  {
    auto doc = parseFile((content / "parent_ok.x3d").string());
    check(countInlines(doc) == 0,
          "default Inline resolver: sibling within the root still expands");
    check(doc.scene.rootNodes.size() == 1,
          "expanded child roots are spliced into the parent scene");
  }
  {
    auto doc = parseFile((content / "parent_dotted.x3d").string());
    check(countInlines(doc) == 0,
          "default Inline resolver: ./ spelling of a sibling still expands");
  }

  // (3) Cycle termination survives the canonical-key change (a -> b -> a).
  {
    auto doc = parseFile((content / "cycle_a.x3d").string());
    check(doc.scene.rootNodes.size() >= 1,
          "inline cycle a -> b -> a terminates");
  }

  fs::remove_all(root, ec);

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all inline_confine checks passed\n";
  return 0;
}
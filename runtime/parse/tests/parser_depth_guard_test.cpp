// JSON and ClassicVRML nesting is bounded to prevent native stack exhaustion.

#include "ClassicVrmlReader.hpp" // direct reader reuse (depth_ is a member)
#include "JsonLite.hpp"
#include "X3DParse.hpp" // codec::parseDocument front door (content-sniffs VRML)

#include "doctest/doctest.h"

#include <stdexcept>
#include <string>

static std::string nestedJson(int depth) {
  return std::string(depth, '[') + std::string(depth, ']');
}

static std::string nestedVrml(int depth) {
  std::string s = "#X3D V3.0 utf8\n";
  for (int i = 0; i < depth; ++i) s += "Transform { children [ ";
  for (int i = 0; i < depth; ++i) s += "] }";
  return s;
}

TEST_CASE("json_rejects_pathological_nesting") {
  CHECK_THROWS_AS((void)x3d::json::parse(nestedJson(50000)), std::runtime_error);
}

TEST_CASE("json_allows_legitimate_nesting") {
  CHECK_NOTHROW((void)x3d::json::parse(nestedJson(100)));
}

TEST_CASE("vrml_rejects_pathological_nesting") {
  CHECK_THROWS_AS((void)x3d::codec::parseDocument(nestedVrml(50000)),
                  std::runtime_error);
}

TEST_CASE("vrml_allows_legitimate_nesting") {
  CHECK_NOTHROW((void)x3d::codec::parseDocument(nestedVrml(50)));
}

TEST_CASE("vrml_reader_depth_counter_resets_after_a_rejected_parse") {
  // The reader holds its nesting counter as a member, and readDocument is a
  // public API an embedder can call repeatedly on one instance. If a rejected
  // (too-deep) parse leaked even one count, it would accumulate and eventually
  // reject valid documents. Reuse one reader: a rejected parse must not poison
  // the counter for a subsequent at-the-cap-boundary document.
  x3d::codec::ClassicVrmlReader reader;
  CHECK_THROWS_AS((void)reader.readDocument(nestedVrml(2000)),
                  std::runtime_error);
  // 1000 levels is the deepest the cap accepts; it must still parse on the same
  // reused reader after rejecting excessive nesting.
  CHECK_NOTHROW((void)reader.readDocument(nestedVrml(1000)));
}

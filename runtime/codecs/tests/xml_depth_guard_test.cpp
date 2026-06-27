// xml_depth_guard_test.cpp
// Regression for SEC-1 (#11), XML half: a pathologically deep document drove
// XmlLite's parseElement<->parseContent mutual recursion past the native stack
// and SIGSEGV'd with no diagnostic. The shared nesting cap turns that into a
// catchable std::runtime_error while leaving legitimately-nested documents
// untouched.
#include "XmlLite.hpp"

#include "doctest/doctest.h"

#include <stdexcept>
#include <string>

static std::string nestedXml(int depth) {
  std::string s = "<r>";
  for (int i = 0; i < depth; ++i) s += "<g>";
  for (int i = 0; i < depth; ++i) s += "</g>";
  s += "</r>";
  return s;
}

TEST_CASE("xml_rejects_pathological_nesting") {
  // Pre-fix: stack overflow (process SIGSEGV). Post-fix: a clean throw.
  CHECK_THROWS_AS((void)x3d::xml::parse(nestedXml(50000)), std::runtime_error);
}

TEST_CASE("xml_allows_legitimate_nesting") {
  // Real authored scenes nest far below the cap; this must keep parsing.
  CHECK_NOTHROW((void)x3d::xml::parse(nestedXml(100)));
}

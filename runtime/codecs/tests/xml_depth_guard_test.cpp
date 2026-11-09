// XML nesting is bounded to prevent native stack exhaustion.

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
  CHECK_THROWS_AS((void)x3d::xml::parse(nestedXml(50000)), std::runtime_error);
}

TEST_CASE("xml_allows_legitimate_nesting") {
  // Real authored scenes nest far below the cap; this must keep parsing.
  CHECK_NOTHROW((void)x3d::xml::parse(nestedXml(100)));
}

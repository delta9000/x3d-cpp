#include "JsonLite.hpp"
#include "doctest/doctest.h"

#include <string>

TEST_CASE("json lone high surrogate does not consume the following escape") {
  using x3d::json::parseComplete;
  // Preserve the parser's existing lenient lone-surrogate representation,
  // while ensuring subsequent valid input is never silently discarded.
  const std::string high = "\xED\xA0\x80";
  CHECK(parseComplete(R"("\uD800\u0041")")->str == high + "A");
  CHECK(parseComplete(R"("\uD800A")")->str == high + "A");
  CHECK(parseComplete(R"("\uD800\u0000")")->str == high + std::string(1, '\0'));
  CHECK(parseComplete(R"("\uD800\u20AC")")->str == high + "\xE2\x82\xAC");
  CHECK(parseComplete(R"("\uD800\uD83D\uDE00")")->str == high + "\xF0\x9F\x98\x80");
  CHECK(parseComplete(R"("\uD800\n")")->str == high + "\n");
}

TEST_CASE("json valid surrogate pairs and malformed hex retain their behavior") {
  using x3d::json::parseComplete;
  CHECK(parseComplete(R"("\uD800\uDC00")")->str == "\xF0\x90\x80\x80");
  CHECK(parseComplete(R"("\uDBFF\uDFFF")")->str == "\xF4\x8F\xBF\xBF");
  CHECK_THROWS_AS(parseComplete(R"("\uD800\uZZZZ")"), std::runtime_error);
  CHECK_THROWS_AS(parseComplete(R"("\uD800\u00")"), std::runtime_error);
}

#include "JsonLite.hpp"
#include "JsonReader.hpp"
#include "XmlLite.hpp"
#include "doctest/doctest.h"

#include <string>

TEST_CASE("JSON numbers require complete RFC8259 digit sequences") {
  for (const auto *s : {"1e", "1e+", "1e-", "01", "-01", "1.", "-.1", "-"}) {
    INFO(s);
    CHECK_THROWS_AS(x3d::json::parseComplete(s), std::runtime_error);
  }
  for (const auto *s : {"0", "-0", "1.0", "-0.25", "1e+2", "1E-2"}) {
    INFO(s);
    CHECK_NOTHROW(x3d::json::parseComplete(s));
  }
}

TEST_CASE("JSON component integer conversion rejects overflow and fractions") {
  x3d::codec::JsonReader reader;
  const auto document = [](const std::string &level) {
    return R"({"X3D":{"head":{"component":[{"@name":"Core","@level":)" + level +
           R"(}]},"Scene":{}}})";
  };
  for (const auto *s :
       {"1e100", "-1e100", "2147483648", "-2147483649", "1.5"}) {
    INFO(s);
    CHECK_THROWS_AS(reader.readDocument(document(s)), std::runtime_error);
  }
  CHECK(reader.readDocument(document("2e0")).head.components.at(0).level == 2);
  CHECK(
      reader.readDocument(document("2147483647")).head.components.at(0).level ==
      2147483647);
}

TEST_CASE("XML numeric references validate syntax and XML character ranges") {
  for (const auto *s :
       {"&#65junk;", "&#x110000;", "&#-1;", "&#;", "&#x;", "&#X41;", "&#+65;",
        "&# 65;", "&#0;", "&#1;", "&#xD800;", "&#xDFFF;", "&#xFFFE;",
        "&#xFFFF;", "&#999999999999999999999999999;"}) {
    INFO(s);
    CHECK_THROWS_AS(x3d::xml::unescape(s), std::runtime_error);
  }
  CHECK(x3d::xml::unescape("&#65;&#x41;") == "AA");
  CHECK(x3d::xml::unescape("&#9;&#10;&#13;") == "\t\n\r");
  CHECK(x3d::xml::unescape("&#x1F600;") == "\xF0\x9F\x98\x80");
  CHECK(x3d::xml::unescape("&#x10FFFF;") == "\xF4\x8F\xBF\xBF");
  CHECK_THROWS_AS(x3d::xml::parse("<X3D profile='&#-1;'/>"),
                  std::runtime_error);
}

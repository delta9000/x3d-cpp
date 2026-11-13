// inline_roundtrip_test.cpp
// Writer round-trip — after Inline expansion, writers must re-emit
// the original <Inline url=.../> and not the synthetic Group's child content.
// Pass the inline fixtures dir as X3D_TEST_DATA_DIR.
#include "doctest/doctest.h"
#include "x3d/sdk.hpp"
#include <iostream>
#include <string>

TEST_CASE("an expanded Inline is written back as an Inline") {
  std::string dir = X3D_TEST_DATA_DIR;
  auto doc = x3d::sdk::parseFile(dir + "/parent.x3d");

  // After expansion, the Inline content (Box) is in the live graph, but each
  // writer must re-emit <Inline url=...> and not the Box.
  SUBCASE("XML") {
    std::string xml = x3d::codec::XmlWriter{}.writeDocument(doc);
    REQUIRE((xml.find("<Inline") != std::string::npos && "Inline must re-emit (XML)"));
    REQUIRE((xml.find("child.x3d") != std::string::npos && "url must be preserved (XML)"));
    REQUIRE((xml.find("<Box") == std::string::npos &&
           "expanded child content must NOT be serialized (XML)"));
  }

  SUBCASE("ClassicVRML") {
    std::string vrml = x3d::codec::VrmlWriter{}.writeDocument(doc);
    REQUIRE((vrml.find("Inline") != std::string::npos && "Inline must re-emit (VRML)"));
    REQUIRE((vrml.find("child.x3d") != std::string::npos && "url must be preserved (VRML)"));
    REQUIRE((vrml.find("Box {") == std::string::npos &&
           "expanded child content must NOT be serialized (VRML): 'Box {' node must be absent"));
  }

  SUBCASE("JSON") {
    std::string json = x3d::codec::JsonWriter{}.writeDocument(doc);
    REQUIRE((json.find("Inline") != std::string::npos && "Inline must re-emit (JSON)"));
    REQUIRE((json.find("child.x3d") != std::string::npos && "url must be preserved (JSON)"));
    REQUIRE((json.find("Box") == std::string::npos &&
           "expanded child content must NOT be serialized (JSON)"));
  }
}

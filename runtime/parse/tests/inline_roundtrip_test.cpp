// inline_roundtrip_test.cpp
// Task 3: Writer round-trip — after Inline expansion, writers must re-emit
// the original <Inline url=.../> and NOT the synthetic Group's child content.
// Pass the inline fixtures dir as argv[1].
#include "x3d/sdk.hpp"
#include <cassert>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  assert(argc >= 2 && "pass the inline fixtures dir as argv[1]");
  std::string dir = argv[1];
  auto doc = x3d::sdk::parseFile(dir + "/parent.x3d");

  // After expansion, the Inline content (Box) is in the live graph...
  // ...but the writer must re-emit <Inline url=...> and NOT the Box.
  std::string xml = x3d::codec::XmlWriter{}.writeDocument(doc);
  assert(xml.find("<Inline") != std::string::npos && "Inline must re-emit (XML)");
  assert(xml.find("child.x3d") != std::string::npos && "url must be preserved (XML)");
  assert(xml.find("<Box") == std::string::npos &&
         "expanded child content must NOT be serialized (XML)");

  // Same for VRML encoding.
  std::string vrml = x3d::codec::VrmlWriter{}.writeDocument(doc);
  assert(vrml.find("Inline") != std::string::npos && "Inline must re-emit (VRML)");
  assert(vrml.find("child.x3d") != std::string::npos && "url must be preserved (VRML)");
  assert(vrml.find("Box {") == std::string::npos &&
         "expanded child content must NOT be serialized (VRML): 'Box {' node must be absent");

  // Same for JSON encoding.
  std::string json = x3d::codec::JsonWriter{}.writeDocument(doc);
  assert(json.find("Inline") != std::string::npos && "Inline must re-emit (JSON)");
  assert(json.find("child.x3d") != std::string::npos && "url must be preserved (JSON)");
  assert(json.find("Box") == std::string::npos &&
         "expanded child content must NOT be serialized (JSON)");

  std::cout << "inline_roundtrip_test OK\n";
  return 0;
}

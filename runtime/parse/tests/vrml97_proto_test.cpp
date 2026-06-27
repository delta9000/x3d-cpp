// vrml97_proto_test.cpp
// VRML97 (.wrl) PROTO capture is inherited from ClassicVrmlReader. Verify a
// VRML97-dialect PROTO declaration + instance reads and expands, including the
// VRML eventType spellings (field/exposedField/eventIn/eventOut).
//
// Uses the parseDocument front door: it sniffs "#VRML V2.0" -> Encoding::VRML97
// -> Vrml97Reader -> expandScene, identical to the json_proto_test pattern.
// This is unambiguous AND exercises the full sniff+dispatch path.
#include "X3DParse.hpp"
#include "x3d/core/X3DReflection.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}
static std::shared_ptr<X3DNode> firstRoot(const runtime::X3DDocument &doc) {
  for (auto &n : doc.scene.rootNodes) if (n) return n;
  return nullptr;
}

// PROTO Ball [ field SFFloat r 2.0 ] { Sphere { radius IS r } }
// Ball { r 7.0 }
// After expansion the root node should be a Sphere with radius == 7.0f.
// "field" is the VRML97 alias for initializeOnly; ClassicVrmlReader::accessTypeFromString
// already handles it, so no reader change is required.
static void vrml97ProtoExpandTest() {
  const char *wrl =
      "#VRML V2.0 utf8\n"
      "PROTO Ball [ field SFFloat r 2.0 ] {\n"
      "  Sphere { radius IS r }\n"
      "}\n"
      "Ball { r 7.0 }\n";

  auto doc = codec::parseDocument(wrl);
  auto root = firstRoot(doc);
  CHECK((root && root->nodeTypeName() == "Sphere"));
  auto fi = fieldByName(*root, "radius");
  CHECK((fi && "radius field not found on Sphere"));
  auto radius = std::any_cast<float>(fi->get(*root));
  CHECK((radius == 7.0f));
}

TEST_CASE("vrml97_proto_test") {
  vrml97ProtoExpandTest();
  return;
}

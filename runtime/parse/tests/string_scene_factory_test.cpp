#include "doctest/doctest.h"
// makeStringSceneFactory(): the parser behind the script services
// Browser.createX3DFromString / createVrmlFromString.
#include "X3DParse.hpp"
#include "x3d/nodes/Inline.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"

#include <memory>
#include <stdexcept>

using x3d::nodes::Shape;
using x3d::nodes::Transform;

TEST_CASE("string scene factory parses a full XML document") {
  auto make = x3d::codec::makeStringSceneFactory();
  auto roots = make("<X3D profile='Interchange' version='4.0'><Scene>"
                    "<Transform translation='1 2 3'><Shape><Box/></Shape></Transform>"
                    "<Shape><Sphere/></Shape></Scene></X3D>");
  REQUIRE(roots.size() == 2);
  auto t = std::dynamic_pointer_cast<Transform>(roots[0]);
  REQUIRE(t);
  CHECK(t->getTranslation().y == doctest::Approx(2));
  CHECK(t->getChildren().size() == 1);
  CHECK(std::dynamic_pointer_cast<Shape>(roots[1]));
}

TEST_CASE("string scene factory accepts fragments") {
  auto make = x3d::codec::makeStringSceneFactory();
  auto xml = make("  <Transform translation='4 5 6'/>");
  REQUIRE(xml.size() == 1);
  CHECK(std::dynamic_pointer_cast<Transform>(xml[0])->getTranslation().x ==
        doctest::Approx(4));

  auto vrml = make("Transform { translation 7 8 9 children Shape { geometry Box { } } }");
  REQUIRE(vrml.size() == 1);
  auto t = std::dynamic_pointer_cast<Transform>(vrml[0]);
  REQUIRE(t);
  CHECK(t->getTranslation().z == doctest::Approx(9));
  CHECK(t->getChildren().size() == 1);

  auto headed = make("#X3D V4.0 utf8\nPROFILE Interchange\nShape { }\n");
  CHECK(headed.size() == 1);
}

TEST_CASE("string scene factory expands a PROTO declared in the text") {
  auto make = x3d::codec::makeStringSceneFactory();
  auto roots = make(
      "<ProtoDeclare name='P'><ProtoBody><Transform translation='1 1 1'/>"
      "</ProtoBody></ProtoDeclare><ProtoInstance name='P'/>");
  REQUIRE(roots.size() == 1);
  CHECK(std::dynamic_pointer_cast<Transform>(roots[0]));
}

TEST_CASE("string scene factory rejects text with no nodes and fetches nothing") {
  auto make = x3d::codec::makeStringSceneFactory();
  CHECK_THROWS_AS(make(""), std::runtime_error);
  CHECK_THROWS_AS(make("   \n"), std::runtime_error);
  CHECK_THROWS(make("<X3D profile='Interchange' version='4.0'><Scene/></X3D>"));
  // An Inline in script-supplied text is kept as a node but never loaded.
  auto roots = make("<Inline url='\"/etc/passwd\" \"missing.x3d\"'/>");
  REQUIRE(roots.size() == 1);
}

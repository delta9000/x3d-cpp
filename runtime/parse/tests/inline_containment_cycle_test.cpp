#include "doctest/doctest.h"
#include "InlineExpand.hpp"
#include "X3DScene.hpp"
#include "scene/CycleBreaker.hpp"
#include "x3d/nodes/Inline.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/sdk.hpp"
#include <memory>
#include <string>

// The readers drop a USE of an enclosing node (READ-CYCLIC-USE), so a parsed
// file cannot carry the self-referential shape any more.
TEST_CASE("a parsed self-referential USE is dropped before Inline expansion") {
  std::string dir = X3D_TEST_DATA_DIR;
  auto doc = x3d::sdk::parseFile(dir + "/cycle_containment.x3d");
  REQUIRE(doc.scene.rootNodes.size() == 1);
  auto a = std::dynamic_pointer_cast<x3d::nodes::Transform>(doc.scene.rootNodes[0]);
  REQUIRE(a);
  CHECK(a->getChildren().empty());
  CHECK(x3d::runtime::breakContainmentCycles(doc.scene) == 0);
}

// A cycle built through the API still reaches expandInlines, which runs before
// breakContainmentCycles: its walk must terminate on it.
TEST_CASE("an Inline containment cycle terminates") {
  using namespace x3d::runtime;
  Scene scene;
  auto a = std::make_shared<x3d::nodes::Transform>();
  auto inl = std::make_shared<x3d::nodes::Inline>();
  inl->setUrl({"child.x3d"});
  a->setChildren({a, inl});
  scene.addRootNode(a);

  InlineResolver resolver = [](const std::vector<std::string> &,
                               const std::string &) { return std::shared_ptr<Scene>(); };
  std::vector<InlineWarning> warnings;
  expandInlines(scene, resolver, "", warnings); // must not overflow the stack

  // This test builds no execution context, so break the cycle before leaving.
  CHECK(breakContainmentCycles(scene) == 1);
}

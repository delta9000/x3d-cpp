// m2c_tick_test.cpp — end-to-end: buildSceneGraph default-binds the first Viewpoint;
// a set_bind event delivered through tick switches the bound node and fires isBound.
#include "X3DExecutionContext.hpp"
#include "X3DBindableNode.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include <memory>
using namespace x3d::runtime;
static X3DBindableNode* bnd(const std::shared_ptr<X3DNode>& n) {
  return dynamic_cast<X3DBindableNode*>(n.get());
}

TEST_CASE("m2c_tick_test") {
  auto vp1 = createX3DNode("Viewpoint");
  auto vp2 = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp1); scene.addRootNode(vp2);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  CHECK((ctx.boundViewpoint() == vp1.get()));
  CHECK((bnd(vp1)->getIsBound() == true));

  ctx.postEvent(vp2.get(), "set_bind", std::any(SFBool(true)));
  ctx.tick(1.0);
  CHECK((ctx.boundViewpoint() == vp2.get()));
  CHECK((bnd(vp2)->getIsBound() == true));
  CHECK((bnd(vp1)->getIsBound() == false));

  ctx.postEvent(vp2.get(), "set_bind", std::any(SFBool(false)));
  ctx.tick(2.0);
  CHECK((ctx.boundViewpoint() == vp1.get()));
  CHECK((bnd(vp1)->getIsBound() == true));
  return;
}

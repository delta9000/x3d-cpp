// binding_system_test.cpp — enrol bindable nodes, default-bind, then fire set_bind
// through the wired handler (which posts isBound via the supplied poster/clock).
// BindingSystem is decoupled from X3DExecutionContext via callbacks; here the
// callbacks drive a real context so the posted isBound events get delivered.
#include "BindingSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DBindableNode.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <string>
using namespace x3d::runtime;

static X3DBindableNode* bindable(const std::shared_ptr<X3DNode>& n) {
  return dynamic_cast<X3DBindableNode*>(n.get());
}

TEST_CASE("binding_system_test") {
  auto vp1 = createX3DNode("Viewpoint");
  auto vp2 = createX3DNode("Viewpoint");
  auto nav = createX3DNode("NavigationInfo");
  Scene scene;
  scene.addRootNode(vp1); scene.addRootNode(vp2); scene.addRootNode(nav);

  X3DExecutionContext ctx;
  BindingSystem bs;
  bs.enrollScene(scene,
      [&](X3DNode* n, const std::string& f, std::any v){ ctx.postEvent(n, f, std::move(v)); },
      [&]{ return ctx.now(); });
  bs.bindDefaults();

  // Defaults: first of each category bound.
  CHECK((bs.bound("Viewpoint") == vp1.get()));
  CHECK((bs.bound("NavigationInfo") == nav.get()));
  CHECK((bindable(vp1)->getIsBound() == true));
  CHECK((bindable(vp2)->getIsBound() == false));

  // Fire set_bind TRUE on vp2 via its onSet_bind (as the cascade would), then drain
  // so the posted isBound events are delivered to the emit thunks.
  bindable(vp2)->onSet_bind(true);
  ctx.process();
  CHECK((bs.bound("Viewpoint") == vp2.get()));
  CHECK((bindable(vp2)->getIsBound() == true));
  CHECK((bindable(vp1)->getIsBound() == false));
  // NavigationInfo category untouched by a Viewpoint bind.
  CHECK((bs.bound("NavigationInfo") == nav.get()));

  // set_bind FALSE on vp2 -> back to vp1.
  bindable(vp2)->onSet_bind(false);
  ctx.process();
  CHECK((bs.bound("Viewpoint") == vp1.get()));
  CHECK((bindable(vp1)->getIsBound() == true));
  return;
}

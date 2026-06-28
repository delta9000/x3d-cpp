#include "doctest/doctest.h"
// nav_pointer_screen_test.cpp — regression for the interactive mouse-nav blow-up.
//
// EXAMINE/FLY drag MUST be driven by the camera-INDEPENDENT normalized screen
// pointer (ctx.setPointerScreen), NOT by the world-space ray origin. The world
// ray is recomputed from the camera each frame, so the same screen cursor maps
// to a different world point whenever the camera rotates — deriving the drag
// delta from it creates a runaway positive-feedback loop (a single click spins
// the camera) and scales the rotation by the scene's near-plane size. The screen
// pointer is the only camera-independent signal, so navigation reads it; the
// world ray stays reserved for picking.

#include "NavigationSystem.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/NavigationInfo.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Viewpoint.hpp"

#include <any>
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {

void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
float dist(const SFVec3f &a, const SFVec3f &b) {
  float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Holds the node keepalives + the context (the context indexes raw node
// pointers; the nodes must outlive it — mirror navigation_test's World).
struct World {
  std::vector<std::shared_ptr<X3DNode>> nodes;
  X3DExecutionContext ctx;
};

std::shared_ptr<World> examineWorld(SFVec3f vpPos) {
  auto w = std::make_shared<World>();
  auto box = createX3DNode("Box");
  setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(vpPos));
  auto nav = createX3DNode("NavigationInfo");
  dynamic_cast<NavigationInfo &>(*nav).setType({"EXAMINE"});
  w->nodes = {box, shape, vp, nav};

  Scene scene;
  scene.addRootNode(shape);
  scene.addRootNode(vp);
  scene.addRootNode(nav);
  w->ctx.buildSceneGraph(scene);
  w->ctx.addSystem(std::make_shared<NavigationSystem>());
  return w;
}

} // namespace

TEST_CASE("examine drag follows the screen pointer and ignores world-ray motion") {
  auto w = examineWorld({0, 0, 10});
  REQUIRE(w->ctx.boundViewpoint() != nullptr);
  w->ctx.setPointerPresent(true);
  w->ctx.setPointerButton(true);

  // Anchor frame: establish the drag origin.
  w->ctx.setPointerScreen(0.5f, 0.5f);
  w->ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}});
  w->ctx.tick(0.0);
  const SFVec3f c0 = w->ctx.cameraWorldPosition();

  // (a) A real screen-pointer drag MUST orbit — even though the world ray below
  //     is held constant. Navigation follows the screen pointer.
  w->ctx.setPointerScreen(0.6f, 0.5f);
  w->ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}}); // world ray UNCHANGED
  w->ctx.tick(0.016);
  const SFVec3f c1 = w->ctx.cameraWorldPosition();
  CHECK(dist(c0, c1) > 1e-3f); // orbited from the screen drag alone

  // (b) The screen pointer held STILL while the world ray jumps wildly (exactly
  //     what happens when the camera rotates and the consumer re-unprojects the
  //     same cursor) MUST NOT orbit — this is the runaway-feedback regression.
  w->ctx.setPointerScreen(0.6f, 0.5f);                  // unchanged
  w->ctx.setPointer(Ray{{0.37f, -0.21f, 10}, {0, 0, -1}}); // world-ray jump
  w->ctx.tick(0.032);
  const SFVec3f c2 = w->ctx.cameraWorldPosition();
  CHECK(dist(c1, c2) < 1e-4f); // world-ray motion alone does not navigate
}

#include "doctest/doctest.h"
// collision_test.cpp — avatar collision in NavigationSystem (ISO/IEC 19775-1
// §23.4.2 Collision, §23.4.4 NavigationInfo avatarSize/WALK). Builds small
// scenes in code, holds the forward/back keys, ticks, and reads the effective
// camera position and the Collision nodes' outputs.
//
// Cases:
//   (1) FLY into a wall inside a Collision group stops avatarSize[0] short of
//       it; isActive TRUE + collideTime fire once at contact, stay active while
//       resting against the wall, and isActive FALSE fires on backing away.
//   (2) enabled FALSE on an outer Collision turns collision off for its whole
//       subtree, a nested enabled Collision included (COL-3): no block, no events.
//   (3) proxy: an invisible proxy blocks; a group whose proxy is elsewhere lets
//       the avatar through its visible children.
//   (4) WALK: gravity brings the eye down to avatarSize[1] above the ground; a
//       step lower than avatarSize[2] is climbed; a taller obstacle blocks.
//   (5) Line geometry is not collidable (§23.4.2).

#include "NavigationSystem.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include "x3d/nodes/Collision.hpp"
#include "x3d/nodes/NavigationInfo.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d;
using namespace x3d::runtime;

namespace {

int failures = 0;
void check(bool cond, const std::string &what) {
  if (!cond) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else        { std::cout << "ok: " << what << "\n"; }
}
bool feq(float a, float b, float e = 2e-2f) { return std::fabs(a - b) < e; }

using NodeP = std::shared_ptr<X3DNode>;

void setF(const NodeP &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
void addChild(const NodeP &p, const NodeP &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<NodeP>>(f.get(*p));
      k.push_back(c); f.set(*p, std::any(std::move(k))); return;
    }
}
// A Box shape of `size` centred at `at`.
NodeP boxAt(SFVec3f size, SFVec3f at) {
  auto box = createX3DNode("Box");
  setF(box, "size", std::any(size));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(NodeP(box)));
  auto t = createX3DNode("Transform");
  setF(t, "translation", std::any(at));
  addChild(t, shape);
  return t;
}
SFVec3f cam(X3DExecutionContext &ctx) { return ctx.cameraWorldPosition(); }

struct World {
  std::vector<NodeP> keep;
  X3DExecutionContext ctx;
};

std::shared_ptr<World> makeWorld(const std::string &type, SFVec3f vpPos,
                                 const std::vector<NodeP> &content, float speed = 10.0f) {
  auto w = std::make_shared<World>();
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(vpPos));
  auto nav = createX3DNode("NavigationInfo");
  auto &ni = dynamic_cast<NavigationInfo &>(*nav);
  ni.setType({type});
  ni.setSpeed(speed);
  Scene scene;
  for (const NodeP &n : content) { scene.addRootNode(n); w->keep.push_back(n); }
  scene.addRootNode(vp);
  scene.addRootNode(nav);
  w->keep.push_back(vp);
  w->keep.push_back(nav);
  w->ctx.buildSceneGraph(scene);
  w->ctx.addSystem(std::make_shared<NavigationSystem>());
  return w;
}

// Tick from t0 to t1 in 50 ms steps.
void run(X3DExecutionContext &ctx, double t0, double t1) {
  for (double t = t0; t <= t1 + 1e-9; t += 0.05) ctx.tick(t);
}

bool activeOf(const NodeP &c) { return dynamic_cast<Collision &>(*c).getIsActive(); }
double collideTimeOf(const NodeP &c) { return dynamic_cast<Collision &>(*c).getCollideTime(); }

} // namespace

TEST_CASE("collision_test") {
  // (1) FLY into a wall inside a Collision group.
  {
    auto group = createX3DNode("Collision");
    addChild(group, boxAt({2, 2, 2}, {0, 0, 0})); // front face at z = 1
    auto w = makeWorld("FLY", {0, 0, 5}, {group});
    w->ctx.setKey(NavigationSystem::kKeyForward, true);
    run(w->ctx, 0.0, 1.0); // unobstructed this would reach z = -5
    check(feq(cam(w->ctx).z, 1.25f), "fly: stops avatarSize[0] (0.25) short of the wall");
    check(activeOf(group), "fly: Collision.isActive TRUE on contact");
    const double first = collideTimeOf(group);
    check(first > 0.0 && first < 1.0, "fly: collideTime is the tick the contact began");
    run(w->ctx, 1.05, 1.5); // keep pushing into the wall
    check(collideTimeOf(group) == first, "fly: collideTime fires once per contact, not per tick");
    w->ctx.setKey(NavigationSystem::kKeyForward, false);
    run(w->ctx, 1.55, 1.7); // rest against the wall
    check(activeOf(group), "fly: resting against the wall stays in collision");
    w->ctx.setKey(NavigationSystem::kKeyBack, true);
    run(w->ctx, 1.75, 2.0);
    check(cam(w->ctx).z > 1.3f, "fly: backing away moves off the wall");
    check(!activeOf(group), "fly: isActive FALSE once the collision no longer occurs");
  }

  // (2) enabled FALSE turns collision off for the subtree, nested Collision too.
  {
    auto outer = createX3DNode("Collision");
    setF(outer, "enabled", std::any(SFBool{false}));
    auto inner = createX3DNode("Collision"); // enabled TRUE by default
    addChild(inner, boxAt({2, 2, 2}, {0, 0, 0}));
    addChild(outer, inner);
    auto w = makeWorld("FLY", {0, 0, 5}, {outer});
    w->ctx.setKey(NavigationSystem::kKeyForward, true);
    run(w->ctx, 0.0, 1.0);
    check(cam(w->ctx).z < -4.0f, "disabled: the avatar passes through the geometry");
    check(!activeOf(inner) && !activeOf(outer), "disabled: no Collision events fire");
  }

  // (3) proxy semantics.
  {
    auto wall = createX3DNode("Collision"); // invisible wall: proxy, no children
    auto proxyBox = createX3DNode("Box");
    setF(proxyBox, "size", std::any(SFVec3f{2, 2, 2}));
    auto proxyShape = createX3DNode("Shape");
    setF(proxyShape, "geometry", std::any(NodeP(proxyBox)));
    setF(wall, "proxy", std::any(NodeP(proxyShape)));
    auto w = makeWorld("FLY", {0, 0, 5}, {wall});
    w->ctx.setKey(NavigationSystem::kKeyForward, true);
    run(w->ctx, 0.0, 1.0);
    check(feq(cam(w->ctx).z, 1.25f), "proxy: an invisible proxy blocks the avatar");
    check(activeOf(wall), "proxy: collision with the proxy fires isActive");

    auto group = createX3DNode("Collision"); // visible children, proxy far away
    addChild(group, boxAt({2, 2, 2}, {0, 0, 0}));
    auto farShape = createX3DNode("Shape");
    auto farBox = createX3DNode("Box");
    setF(farShape, "geometry", std::any(NodeP(farBox)));
    auto farT = createX3DNode("Transform");
    setF(farT, "translation", std::any(SFVec3f{100, 0, 0}));
    addChild(farT, farShape);
    setF(group, "proxy", std::any(NodeP(farT)));
    auto w2 = makeWorld("FLY", {0, 0, 5}, {group});
    w2->ctx.setKey(NavigationSystem::kKeyForward, true);
    run(w2->ctx, 0.0, 1.0);
    check(cam(w2->ctx).z < -4.0f, "proxy: the proxy replaces the children for collision");
  }

  // (4) WALK: gravity, terrain following, step height.
  {
    auto ground = boxAt({100, 1, 100}, {0, -0.5f, 0}); // top at y = 0
    auto step = boxAt({4, 0.5f, 2}, {0, 0.25f, 0});    // top at y = 0.5, z in [-1, 1]
    auto w = makeWorld("WALK", {0, 3, 5}, {ground, step}, 2.0f);
    run(w->ctx, 0.0, 2.0); // fall from y = 3
    check(feq(cam(w->ctx).y, 1.6f), "walk: gravity settles the eye at avatarSize[1] above ground");
    w->ctx.setKey(NavigationSystem::kKeyForward, true);
    run(w->ctx, 2.05, 4.5); // 2 m/s for ~2.5 s: from z = 5 onto the step (z in [-1, 1])
    check(cam(w->ctx).z < 0.5f, "walk: a step lower than avatarSize[2] does not block");
    check(feq(cam(w->ctx).y, 2.1f), "walk: the eye climbs onto the step (0.5 + 1.6)");

    auto ground2 = boxAt({100, 1, 100}, {0, -0.5f, 0});
    auto tall = boxAt({4, 2, 2}, {0, 1, 0}); // top at y = 2 > avatarSize[2]
    auto w2 = makeWorld("WALK", {0, 1.6f, 5}, {ground2, tall}, 2.0f);
    w2->ctx.setKey(NavigationSystem::kKeyForward, true);
    run(w2->ctx, 0.0, 3.0);
    check(feq(cam(w2->ctx).z, 1.25f), "walk: an obstacle taller than avatarSize[2] blocks");
    check(feq(cam(w2->ctx).y, 1.6f), "walk: the eye stays at avatarSize[1] in front of it");
  }

  // (5) Line geometry does not collide.
  {
    auto coord = createX3DNode("Coordinate");
    setF(coord, "point", std::any(MFVec3f{{-5, 0, 0}, {5, 0, 0}, {0, -5, 0}, {0, 5, 0}}));
    auto lines = createX3DNode("IndexedLineSet");
    setF(lines, "coord", std::any(NodeP(coord)));
    setF(lines, "coordIndex", std::any(MFInt32{0, 1, -1, 2, 3, -1}));
    auto shape = createX3DNode("Shape");
    setF(shape, "geometry", std::any(NodeP(lines)));
    auto w = makeWorld("FLY", {0, 0, 5}, {shape});
    w->ctx.setKey(NavigationSystem::kKeyForward, true);
    run(w->ctx, 0.0, 1.0);
    check(cam(w->ctx).z < -4.0f, "lines: an IndexedLineSet is not collidable");
  }

  CHECK(failures == 0);
}

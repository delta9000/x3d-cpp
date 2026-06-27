// view_dependent_test.cpp — M2e: LOD / Billboard / sensors / visibility.
#include "ViewDependentSystem.hpp"
#include "SceneExtractor.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp" // production attachViewDependent
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include "X3DFieldAddress.hpp" // FieldAddress for ROUTE-based change-gate test
#include "x3d/nodes/TransformSensor.hpp"
#include "Aabb.hpp"
#include "TransformSystem.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <iostream>
#include <memory>
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
template <class T>
static T getF(const std::shared_ptr<X3DNode> &n, const char *nm, T def = T{}) {
  for (auto &f : n->fields()) if (f.x3dName == nm && f.get) return std::any_cast<T>(f.get(*n));
  return def;
}

static void testCameraPose() {
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{1, 2, 3}));
  Scene scene; scene.addRootNode(vp);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);     // binds the Viewpoint by default
  SFVec3f eye = ctx.cameraWorldPosition();
  CHECK((feq(eye.x, 1) && feq(eye.y, 2) && feq(eye.z, 3)));
}

// Billboard, axisOfRotation=(0,1,0): a camera on +X must rotate the billboard's
// local +Z to point toward the camera within the XZ plane.
static void testBillboardAxis() {
  Mat4 parent = Mat4::identity();
  SFVec3f cam{10, 0, 0};                 // viewer on +X
  Mat4 r = billboardLocalMatrix(parent, cam, SFVec3f{0, 1, 0}, SFVec3f{0, 1, 0});
  SFVec3f z = r.transformDirection(SFVec3f{0, 0, 1}); // rotated local +Z
  // +Z should now point toward +X (the viewer), i.e. ~ (1,0,0).
  CHECK((feq(z.x, 1) && feq(z.y, 0) && feq(z.z, 0)));
}

// Billboard, axisOfRotation=(0,0,0): viewer-alignment. Camera on +Z keeps +Z
// pointing at the viewer and +Y aligned to the viewer up.
static void testBillboardViewerAlign() {
  Mat4 parent = Mat4::identity();
  SFVec3f cam{0, 0, 5};
  Mat4 r = billboardLocalMatrix(parent, cam, SFVec3f{0, 1, 0}, SFVec3f{0, 0, 0});
  SFVec3f z = r.transformDirection(SFVec3f{0, 0, 1});
  SFVec3f y = r.transformDirection(SFVec3f{0, 1, 0});
  CHECK((feq(z.x, 0) && feq(z.y, 0) && feq(z.z, 1))); // +Z toward viewer
  CHECK((feq(y.x, 0) && feq(y.y, 1) && feq(y.z, 0))); // +Y up
}

static void testBillboardInExtractor() {
  auto bb = createX3DNode("Billboard");
  setF(bb, "axisOfRotation", std::any(SFVec3f{0, 1, 0}));
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(shape, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  // add shape as a child of bb
  for (auto &f : bb->fields())
    if (f.x3dName == "children" && f.set)
      f.set(*bb, std::any(std::vector<std::shared_ptr<X3DNode>>{shape}));
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{10, 0, 0})); // viewer on +X
  Scene scene; scene.addRootNode(vp); scene.addRootNode(bb);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  CHECK((!snap.added.empty()));
  const auto &item = ex.item(snap.added[0]);
  SFVec3f z = item.worldTransform.transformDirection(SFVec3f{0, 0, 1});
  CHECK((feq(z.x, 1) && feq(z.y, 0) && feq(z.z, 0))); // billboard faces +X viewer
}

// Billboard through PickSystem (via X3DExecutionContext::pick, which threads the
// live viewer pose). An elongated Box (depth 4 along local +Z, thin 1 along X)
// sits under a Billboard with axisOfRotation=(0,1,0); the camera is on +X. The
// billboard rotates local +Z toward +X, so the box's depth becomes its world-X
// extent (x in [-2,2], z in [-0.5,0.5]).
//
// A ray from (1.5,0,10) toward -Z misses the UN-rotated box (whose x in
// [-0.5,0.5]) but hits the view-rotated box (x in [-2,2]) on its front face at
// z=+0.5. This asserts pick resolves against the rotated geometry — it would
// fail if the billboard rotation were absent or applied to the wrong frame.
static void testBillboardInPick() {
  auto bb = createX3DNode("Billboard");
  setF(bb, "axisOfRotation", std::any(SFVec3f{0, 1, 0}));
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(box, "size", std::any(SFVec3f{1, 1, 4})); // thin in X, deep in Z
  setF(shape, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  for (auto &f : bb->fields())
    if (f.x3dName == "children" && f.set)
      f.set(*bb, std::any(std::vector<std::shared_ptr<X3DNode>>{shape}));
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{10, 0, 0})); // viewer on +X
  Scene scene; scene.addRootNode(vp); scene.addRootNode(bb);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);

  // Control: same ray against the un-rotated box would miss (x=1.5 outside
  // [-0.5,0.5]); the rotation is what brings the depth into the X extent.
  auto hit = ctx.pick(Ray{{1.5f, 0, 10}, {0, 0, -1}});
  CHECK((hit.hit && hit.node == shape.get()));
  CHECK((feq(hit.point.x, 1.5f) && feq(hit.point.z, 0.5f))); // rotated front face

  // Sanity miss: beyond the rotated X extent (x=2.5 > 2).
  auto miss = ctx.pick(Ray{{2.5f, 0, 10}, {0, 0, -1}});
  CHECK((!miss.hit));
}

static void testLodSelect() {
  // 2 ranges -> 3 levels. center default 0,0,0. ranges [5,10].
  auto lod = createX3DNode("LOD");
  setF(lod, "range", std::any(std::vector<float>{5.0f, 10.0f}));
  // three children: Shapes with Box, Sphere, Cone to tell them apart
  auto mk = [&](const char *g){ auto s=createX3DNode("Shape"); auto geo=createX3DNode(g);
    setF(s,"geometry",std::any(std::static_pointer_cast<X3DNode>(geo))); return s; };
  std::vector<std::shared_ptr<X3DNode>> kids{mk("Box"), mk("Sphere"), mk("Cone")};
  for (auto &f : lod->fields()) if (f.x3dName=="children" && f.set) f.set(*lod, std::any(kids));
  // L(d): d<5 -> level0 (Box); 5<=d<10 -> level1 (Sphere); d>=10 -> level2 (Cone)
  CHECK((lodSelectLevel(*lod, /*distToCenter=*/3.0f) == 0));
  CHECK((lodSelectLevel(*lod, 7.0f) == 1));
  CHECK((lodSelectLevel(*lod, 20.0f) == 2));
  // empty range -> level 0
  auto lod2 = createX3DNode("LOD");
  CHECK((lodSelectLevel(*lod2, 999.0f) == 0));
}

static void testLodLevelChanged() {
  auto lod = createX3DNode("LOD");
  setF(lod, "range", std::any(std::vector<float>{5.0f}));
  auto mk=[&](const char*g){auto s=createX3DNode("Shape");auto x=createX3DNode(g);
    setF(s,"geometry",std::any(std::static_pointer_cast<X3DNode>(x)));return s;};
  std::vector<std::shared_ptr<X3DNode>> kids{mk("Box"), mk("Sphere")};
  for (auto &f : lod->fields()) if (f.x3dName=="children"&&f.set) f.set(*lod,std::any(kids));
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(lod);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(lod.get(), ctx); ctx.addSystem(vds);
  // viewer near (default position 0,0,10 -> d=10 >=5 -> level 1)
  setF(vp, "position", std::any(SFVec3f{0,0,10}));
  int captured = -99;
  vds->setLevelChangedHook([&](X3DNode *n, int lvl){ if (n==lod.get()) captured = lvl; });
  ctx.tick(0.0);
  CHECK((captured == 1));                 // first eval announces level 1
  captured = -99;
  setF(vp, "position", std::any(SFVec3f{0,0,1})); // d=1 <5 -> level 0
  ctx.tick(1.0);
  CHECK((captured == 0));                 // level changed -> announced
}

static void testProximitySensor() {
  auto ps = createX3DNode("ProximitySensor");
  setF(ps, "size", std::any(SFVec3f{4, 4, 4})); // box [-2,2]^3 around origin
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ps);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(ps.get(), ctx); ctx.addSystem(vds);
  bool active = false; double enter = -1, exit = -1;
  vds->setSensorHook([&](X3DNode *n, bool a, double t){ if (n==ps.get()){active=a; if(a)enter=t; else exit=t;} });
  setF(vp, "position", std::any(SFVec3f{0, 0, 1})); // inside box
  ctx.tick(2.0);
  CHECK((active && feq((float)enter, 2.0f)));        // entered at t=2
  setF(vp, "position", std::any(SFVec3f{0, 0, 100})); // outside
  ctx.tick(5.0);
  CHECK((!active && feq((float)exit, 5.0f)));        // exited at t=5
}

static void testProximityLoadTime() {
  auto ps = createX3DNode("ProximitySensor");
  setF(ps, "size", std::any(SFVec3f{100,100,100}));
  auto vp = createX3DNode("Viewpoint"); setF(vp,"position",std::any(SFVec3f{0,0,0}));
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ps);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds=std::make_shared<ViewDependentSystem>(); vds->attach(ps.get(),ctx); ctx.addSystem(vds);
  bool fired=false; vds->setSensorHook([&](X3DNode*n,bool a,double){ if(n==ps.get()&&a)fired=true; });
  ctx.tick(0.0);
  CHECK((fired)); // initial enter at load
}

static void testVisibilitySensorCone() {
  auto vs = createX3DNode("VisibilitySensor");
  setF(vs, "size", std::any(SFVec3f{2, 2, 2}));
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{0, 0, 10})); // looking down -Z toward origin
  Scene scene; scene.addRootNode(vp); scene.addRootNode(vs);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds=std::make_shared<ViewDependentSystem>(); vds->attach(vs.get(),ctx); ctx.addSystem(vds);
  bool active=false; vds->setSensorHook([&](X3DNode*n,bool a,double){ if(n==vs.get())active=a; });
  ctx.tick(0.0);
  CHECK((active)); // box at origin is in front of the camera (within cone) -> visible
  // Move the box behind the camera (viewer still at +10 looking -Z).
  setF(vs, "center", std::any(SFVec3f{0,0,50})); // behind viewer (z>10)
  ctx.tick(1.0);
  CHECK((!active)); // behind camera -> not visible
}

// Frustum seam: a box just outside the bare cone becomes visible once the
// consumer supplies a wider effective half-angle via setViewVolume (aspect>1).
static void testVisibilitySensorFrustumSeam() {
  auto vs = createX3DNode("VisibilitySensor");
  setF(vs, "size", std::any(SFVec3f{0.2f, 0.2f, 0.2f}));     // small box (tiny slack)
  setF(vs, "center", std::any(SFVec3f{6, 0, 0}));            // off-axis in +X
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{0, 0, 10}));         // looking -Z
  setF(vp, "fieldOfView", std::any(SFFloat(0.7854f)));       // ~45deg, half ~22.5deg
  Scene scene; scene.addRootNode(vp); scene.addRootNode(vs);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  // box dir from eye (0,0,10) to (6,0,0): angle off -Z = atan2(6,10) ~ 30.96deg
  // > half-angle 22.5deg -> outside the bare cone.
  auto vds=std::make_shared<ViewDependentSystem>(); vds->attach(vs.get(),ctx); ctx.addSystem(vds);
  bool active=false; vds->setSensorHook([&](X3DNode*n,bool a,double){ if(n==vs.get())active=a; });
  ctx.tick(0.0);
  CHECK((!active)); // outside the bare forward cone
  // Widen the effective half-angle (aspect 2 -> 45deg half) -> now inside.
  vds->setViewVolume(ViewDependentSystem::ViewVolume{true, 2.0f});
  ctx.tick(1.0);
  CHECK((active)); // widened frustum brings the off-axis box into view
}

static void testVisibleFalseSkip() {
  auto grp = createX3DNode("Group");
  setF(grp, "visible", std::any(false)); // X3D 4.0: not displayed
  auto shape = createX3DNode("Shape"); auto box = createX3DNode("Box");
  setF(shape, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  for (auto &f : grp->fields()) if (f.x3dName=="children"&&f.set)
    f.set(*grp, std::any(std::vector<std::shared_ptr<X3DNode>>{shape}));
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(grp);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  CHECK((snap.added.empty())); // visible=FALSE subtree produces no render items
}

static void testVisibilityLimitTag() {
  auto nav = createX3DNode("NavigationInfo");
  setF(nav, "visibilityLimit", std::any(SFFloat(5.0f)));
  auto shape = createX3DNode("Shape"); auto box = createX3DNode("Box");
  setF(shape, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  auto tf = createX3DNode("Transform");
  setF(tf, "translation", std::any(SFVec3f{0, 0, -100})); // far from viewer
  for (auto &f : tf->fields()) if (f.x3dName=="children"&&f.set)
    f.set(*tf, std::any(std::vector<std::shared_ptr<X3DNode>>{shape}));
  auto vp = createX3DNode("Viewpoint"); setF(vp,"position",std::any(SFVec3f{0,0,0}));
  Scene scene; scene.addRootNode(vp); scene.addRootNode(nav); scene.addRootNode(tf);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  CHECK((!snap.added.empty()));
  CHECK((ex.item(snap.added[0]).beyondVisibilityLimit)); // >5 units away -> tagged
}

// Task 8: production wiring. Build a scene with an LOD nested under a Transform
// plus a ProximitySensor, then use only the production entry point
// (attachViewDependent) — no manual per-node attach — and assert both nodes are
// serviced after a tick. This proves the production path discovers and attaches
// the system to every view-dependent node via the scene walk.
static void testProductionWiring() {
  // ProximitySensor at the origin, big box so the default viewer is inside.
  auto ps = createX3DNode("ProximitySensor");
  setF(ps, "size", std::any(SFVec3f{100, 100, 100}));
  // LOD nested under a Transform (exercise the recursive walk past a group).
  auto lod = createX3DNode("LOD");
  setF(lod, "range", std::any(std::vector<float>{10.0f}));
  auto tf = createX3DNode("Transform");
  for (auto &f : tf->fields()) if (f.x3dName == "children" && f.set)
    f.set(*tf, std::any(std::vector<std::shared_ptr<X3DNode>>{lod}));
  auto vp = createX3DNode("Viewpoint"); setF(vp, "position", std::any(SFVec3f{0, 0, 0}));
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ps); scene.addRootNode(tf);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewDependent(scene, ctx); // production wiring — no manual attach
  ctx.tick(0.0);
  // ProximitySensor must have entered (load-time, viewer inside box): isActive=TRUE.
  bool psActive = false;
  for (auto &f : ps->fields()) if (f.x3dName == "isActive" && f.get)
    psActive = std::any_cast<bool>(f.get(*ps));
  CHECK((psActive)); // the system was attached and ran the sensor
  // LOD must have announced a level (viewer at origin, dist 0 < 10 -> level 0).
  SFInt32 lvl = -99;
  for (auto &f : lod->fields()) if (f.x3dName == "level_changed" && f.get)
    lvl = std::any_cast<SFInt32>(f.get(*lod));
  CHECK((lvl == 0)); // level_changed fired via the wired system
}

// LOD-1 (§23.4.3): level_changed must report the index of the child actually
// rendered. With 2 children but 3 range bins, a far viewer (raw level 2) clamps
// to child index 1.
static void testLodLevelClamp() {
  auto lod = createX3DNode("LOD");
  setF(lod, "range", std::any(std::vector<float>{5.0f, 10.0f})); // bins -> raw levels 0,1,2
  std::vector<std::shared_ptr<X3DNode>> kids{createX3DNode("Shape"), createX3DNode("Shape")};
  for (auto &f : lod->fields()) if (f.x3dName == "children" && f.set) f.set(*lod, std::any(kids));
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{0, 0, 20})); // dist 20 >= 10 -> raw level 2
  Scene scene; scene.addRootNode(vp); scene.addRootNode(lod);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>(); vds->attach(lod.get(), ctx); ctx.addSystem(vds);
  int announced = -99; vds->setLevelChangedHook([&](X3DNode *n, int l) { if (n == lod.get()) announced = l; });
  ctx.tick(0.0);
  CHECK((announced == 1)); // clamped to children.size()-1, NOT the raw bin 2
}

// ENV-04 (§22.4.1): position_changed must fire only when the viewer moves, not
// every tick. Routed to a Transform.translation sink; after a no-move tick the
// sentinel must survive (no spurious event).
static void testProximityChangeGate() {
  auto ps = createX3DNode("ProximitySensor"); setF(ps, "size", std::any(SFVec3f{10, 10, 10}));
  auto tf = createX3DNode("Transform");
  auto vp = createX3DNode("Viewpoint"); setF(vp, "position", std::any(SFVec3f{0, 0, 1}));
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ps); scene.addRootNode(tf);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>(); vds->attach(ps.get(), ctx); ctx.addSystem(vds);
  ctx.addRoute(FieldAddress{ps.get(), "position_changed"}, FieldAddress{tf.get(), "translation"});
  ctx.tick(1.0); // inside -> position_changed fires -> tf.translation := eyeLocal
  setF(tf, "translation", std::any(SFVec3f{99, 99, 99})); // sentinel
  ctx.tick(2.0); // viewer did NOT move -> no position_changed -> sentinel survives
  SFVec3f tr = getF<SFVec3f>(tf, "translation");
  CHECK((feq(tr.x, 99) && feq(tr.y, 99) && feq(tr.z, 99)));
}

// ENV-07 (§22.4.1): enabled FALSE->TRUE with the viewer still inside must re-fire
// the enter (isActive TRUE / enterTime).
static void testProximityReEnable() {
  auto ps = createX3DNode("ProximitySensor"); setF(ps, "size", std::any(SFVec3f{10, 10, 10}));
  auto vp = createX3DNode("Viewpoint"); setF(vp, "position", std::any(SFVec3f{0, 0, 0}));
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ps);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>(); vds->attach(ps.get(), ctx); ctx.addSystem(vds);
  int enters = 0; vds->setSensorHook([&](X3DNode *n, bool a, double) { if (n == ps.get() && a) ++enters; });
  ctx.tick(0.0); CHECK((enters == 1));               // initial enter (viewer inside)
  setF(ps, "enabled", std::any(SFBool{false})); ctx.tick(1.0); // disabled
  setF(ps, "enabled", std::any(SFBool{true})); ctx.tick(2.0);  // re-enabled, still inside
  CHECK((enters == 2));                              // ENV-07: re-fires enter
}

// ENV-07 (§22.4.1): disabling a sensor while it is active must deactivate it —
// emit isActive=FALSE/exitTime — not silently go quiet (else routed consumers
// stay stuck TRUE).
static void testProximityDisableFiresExit() {
  auto ps = createX3DNode("ProximitySensor"); setF(ps, "size", std::any(SFVec3f{10, 10, 10}));
  auto vp = createX3DNode("Viewpoint"); setF(vp, "position", std::any(SFVec3f{0, 0, 0}));
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ps);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>(); vds->attach(ps.get(), ctx); ctx.addSystem(vds);
  bool sawExit = false; vds->setSensorHook([&](X3DNode *n, bool a, double) { if (n == ps.get() && !a) sawExit = true; });
  ctx.tick(0.0);                                       // enter (active)
  setF(ps, "enabled", std::any(SFBool{false}));
  ctx.tick(1.0);                                       // disable while active -> exit
  CHECK((sawExit));
}

// ─── ENV-01: TransformSensor (§22.4.2) ───────────────────────────────────────
//
// TransformSensor fires enterTime/exitTime when targetObject's bounding box
// intersects a sensor-local box (center + size), and position_changed /
// orientation_changed while the target is INSIDE the box and pose changes —
// both relative to center, in the sensor's local coordinate frame.
//
// (1) Target inside box at load → isActive TRUE + enterTime fired.
static void testTransformSensorEnter() {
  auto target = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(target, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  auto ts = createX3DNode("TransformSensor");
  setF(ts, "size", std::any(SFVec3f{10, 10, 10})); // big box centered on origin
  setF(ts, "center", std::any(SFVec3f{0, 0, 0}));
  setF(ts, "targetObject", std::any(std::static_pointer_cast<X3DNode>(target)));
  auto vp = createX3DNode("Viewpoint"); // default viewer at (0,0,10)
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ts); scene.addRootNode(target);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(ts.get(), ctx); ctx.addSystem(vds);
  bool active = false; double enter = -1;
  vds->setSensorHook([&](X3DNode *n, bool a, double t){ if (n==ts.get()){ active=a; if(a) enter=t; } });
  ctx.tick(2.0);
  CHECK((active && feq((float)enter, 2.0f)));      // entered at t=2
  // EnterTime field should also be set via the cascade.
  SFTime et = getF<SFTime>(ts, "enterTime", SFTime{-1});
  CHECK((feq((float)et, 2.0f)));
}

// (2) Target leaves box → isActive FALSE + exitTime fired.
static void testTransformSensorExit() {
  auto target = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(target, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  // Wrap target in a Transform so we can move it out of the sensor box.
  auto xf = createX3DNode("Transform");
  for (auto &f : xf->fields()) if (f.x3dName == "children" && f.set)
    f.set(*xf, std::any(std::vector<std::shared_ptr<X3DNode>>{target}));
  auto ts = createX3DNode("TransformSensor");
  setF(ts, "size", std::any(SFVec3f{2, 2, 2}));   // small sensor box at origin
  setF(ts, "center", std::any(SFVec3f{0, 0, 0}));
  setF(ts, "targetObject", std::any(std::static_pointer_cast<X3DNode>(target)));
  setF(xf, "translation", std::any(SFVec3f{0, 0, 0})); // inside initially
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ts); scene.addRootNode(xf);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(ts.get(), ctx); ctx.addSystem(vds);
  bool active = true; double exit = -1;
  vds->setSensorHook([&](X3DNode *n, bool a, double t){ if (n==ts.get()){ active=a; if(!a) exit=t; } });
  ctx.tick(0.0); CHECK((active));                  // entered
  // setF (direct write, no dirty tracking) — worldTransformAny reads via
  // TransformSystem::localMatrix which reads CURRENT field values, so this is
  // picked up immediately even though TransformSystem's worldTransform cache
  // stays stale until the next tick's propagate.
  setF(xf, "translation", std::any(SFVec3f{100, 0, 0}));
  ctx.tick(1.0);
  CHECK((!active && feq((float)exit, 1.0f)));      // exited at t=1
}

// (3) Target inside box at a known translation → position_changed emitted in
// the sensor's local frame, relative to center. We put the sensor at origin
// (size covers), the target at translation (3, 0, 0) via a parent Transform.
// Expected position_changed ≈ (3, 0, 0).
static void testTransformSensorPositionChanged() {
  auto target = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(target, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  auto xf = createX3DNode("Transform");
  setF(xf, "translation", std::any(SFVec3f{3, 0, 0}));
  for (auto &f : xf->fields()) if (f.x3dName == "children" && f.set)
    f.set(*xf, std::any(std::vector<std::shared_ptr<X3DNode>>{target}));
  auto ts = createX3DNode("TransformSensor");
  setF(ts, "size", std::any(SFVec3f{10, 10, 10}));
  setF(ts, "center", std::any(SFVec3f{0, 0, 0}));
  setF(ts, "targetObject", std::any(std::static_pointer_cast<X3DNode>(target)));
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ts); scene.addRootNode(xf);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(ts.get(), ctx); ctx.addSystem(vds);
  ctx.tick(0.0);
  SFVec3f pc = getF<SFVec3f>(ts, "position_changed", SFVec3f{99, 99, 99});
  // Target Shape's world AABB center is at (3, 0, 0); sensor frame is identity,
  // center is (0,0,0), so position_changed (relative) ≈ (3, 0, 0).
  CHECK((feq(pc.x, 3.0f) && feq(pc.y, 0.0f) && feq(pc.z, 0.0f)));
}

// (4) Change-gate: a no-movement tick must NOT re-fire position_changed.
// Routing to a Transform.translation sink; after a no-move tick the sentinel
// must survive (no spurious event).
static void testTransformSensorChangeGate() {
  auto target = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(target, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  auto xf = createX3DNode("Transform");
  setF(xf, "translation", std::any(SFVec3f{0, 0, 0}));
  for (auto &f : xf->fields()) if (f.x3dName == "children" && f.set)
    f.set(*xf, std::any(std::vector<std::shared_ptr<X3DNode>>{target}));
  auto ts = createX3DNode("TransformSensor");
  setF(ts, "size", std::any(SFVec3f{10, 10, 10}));
  setF(ts, "targetObject", std::any(std::static_pointer_cast<X3DNode>(target)));
  auto sink = createX3DNode("Transform");
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ts); scene.addRootNode(xf); scene.addRootNode(sink);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(ts.get(), ctx); ctx.addSystem(vds);
  ctx.addRoute(FieldAddress{ts.get(), "position_changed"}, FieldAddress{sink.get(), "translation"});
  ctx.tick(1.0);                                  // first emit: position_changed fires
  setF(sink, "translation", std::any(SFVec3f{99, 99, 99})); // sentinel
  ctx.tick(2.0);                                  // target did not move -> no re-emit
  SFVec3f tr = getF<SFVec3f>(sink, "translation");
  CHECK((feq(tr.x, 99) && feq(tr.y, 99) && feq(tr.z, 99)));
}

// (5) Disable while active fires exit (ENV-07 mirror).
static void testTransformSensorDisableFiresExit() {
  auto target = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(target, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  auto ts = createX3DNode("TransformSensor");
  setF(ts, "size", std::any(SFVec3f{10, 10, 10}));
  setF(ts, "targetObject", std::any(std::static_pointer_cast<X3DNode>(target)));
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ts); scene.addRootNode(target);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(ts.get(), ctx); ctx.addSystem(vds);
  bool sawExit = false;
  vds->setSensorHook([&](X3DNode *n, bool a, double){ if (n==ts.get() && !a) sawExit = true; });
  ctx.tick(0.0);                                  // target inside -> active
  setF(ts, "enabled", std::any(SFBool{false}));
  ctx.tick(1.0);                                  // disable -> exit
  CHECK((sawExit));
}

// (6) Null targetObject → inactive (degenerate; no enter).
static void testTransformSensorNullTarget() {
  auto ts = createX3DNode("TransformSensor");
  setF(ts, "size", std::any(SFVec3f{10, 10, 10}));
  // no targetObject set
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ts);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(ts.get(), ctx); ctx.addSystem(vds);
  bool active = false;        // sensor hook fires only on CHANGE (default off -> off)
  bool wasCalled = false;
  vds->setSensorHook([&](X3DNode *n, bool a, double){ if (n==ts.get()){ active=a; wasCalled=true; } });
  ctx.tick(0.0);
  CHECK((!active));            // never active
  CHECK((!wasCalled));         // no edge event fired for null target
}

TEST_CASE("view_dependent_test") {
  testCameraPose();
  testLodLevelClamp();
  testProximityChangeGate();
  testProximityReEnable();
  testProximityDisableFiresExit();
  testBillboardAxis();
  testBillboardViewerAlign();
  testBillboardInExtractor();
  testBillboardInPick();
  testLodSelect();
  testLodLevelChanged();
  testProximitySensor();
  testProximityLoadTime();
  testVisibilitySensorCone();
  testVisibilitySensorFrustumSeam();
  testVisibleFalseSkip();
  testVisibilityLimitTag();
  testProductionWiring();
  testTransformSensorEnter();
  testTransformSensorExit();
  testTransformSensorPositionChanged();
  testTransformSensorChangeGate();
  testTransformSensorDisableFiresExit();
  testTransformSensorNullTarget();
}

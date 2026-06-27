#include "doctest/doctest.h"
// pointing_sensor_test.cpp — M2.5 PointingSensorSystem driving a fully
// spec-complete TouchSensor over the input seam (ISO/IEC 19775-1 §20.4.4,
// §20.2.1, §20.2.3). Builds small scene graphs in code, feeds pointer state via
// the context (setPointer/setPointerButton/setPointerPresent), ticks, and reads
// the sensor outputs back off the generated getters (the cascade applies each
// emit thunk during the tick).
//
// Covers the design §7 cases:
//   (1) isOver enter/leave + no-event-when-unchanged
//   (2) resolution = lowest sensor on the path (nested sensors)
//   (3) nearest geometry's sensor
//   (4) hitPoint/hitNormal in the sensor frame under a translated/rotated xform
//   (4b) hitTexCoord mesh (barycentric) AND primitive (§13), frame-invariant
//   (5) isActive + touchTime click (down-over→up-over ⇒ touchTime; up-after-
//       leave ⇒ no touchTime)
//   (6) grab exclusivity
//   (7) enabled=FALSE
//
// Exit code 0 on success; nonzero on any failed check.

#include "PointingSensorSystem.hpp"
#include "X3DDocument.hpp" // inline Scene::addRootNode
#include "X3DExecutionContext.hpp"

#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "x3d/nodes/PlaneSensor.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TextureCoordinate.hpp"
#include "x3d/nodes/TouchSensor.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;

namespace {

int failures = 0;
void check(bool cond, const std::string &what) {
  if (!cond) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else        { std::cout << "ok: " << what << "\n"; }
}
bool feq(float a, float b, float e = 1e-3f) { return std::fabs(a - b) < e; }

// Reflection field setter / child-append helpers (mirror m2d_tick_test).
void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
void addChild(const std::shared_ptr<X3DNode> &p, const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

std::shared_ptr<X3DNode> boxShape(SFVec3f size = {2, 2, 2}) {
  auto shape = std::make_shared<Shape>();
  auto box = std::make_shared<Box>();
  setF(box, "size", std::any(size));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  return shape;
}

// A context wired with the PointingSensorSystem over a built scene.
struct Rig {
  X3DExecutionContext ctx;
  std::shared_ptr<PointingSensorSystem> sys =
      std::make_shared<PointingSensorSystem>();
  void build(Scene &scene) {
    ctx.buildSceneGraph(scene);
    ctx.addSystem(sys);
  }
};

// Drive a ray straight down -Z from z=10 toward the origin.
Ray downZ() { return Ray{{0, 0, 10}, {0, 0, -1}}; }

// ---------------------------------------------------------------------------
// (1) isOver enter/leave + no-event-when-unchanged.
// ---------------------------------------------------------------------------
void test_isOver_and_no_event_when_unchanged() {
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  auto shape = boxShape();
  addChild(group, sensor);
  addChild(group, shape);
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);

  // Pointer present + over the box.
  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ());
  r.ctx.tick(1.0);
  check(sensor->getIsOver() == true, "(1) isOver TRUE when ray hits the box");

  // Unchanged pointer next tick: isOver stays TRUE (and nothing re-fires; we
  // can only observe the state here — it must remain consistent).
  r.ctx.tick(2.0);
  check(sensor->getIsOver() == true, "(1) isOver stays TRUE, no spurious flip");

  // Move the ray off the box → isOver FALSE.
  r.ctx.setPointer(Ray{{100, 0, 10}, {0, 0, -1}});
  r.ctx.tick(3.0);
  check(sensor->getIsOver() == false, "(1) isOver FALSE when ray leaves the box");
}

// ---------------------------------------------------------------------------
// (2) resolution = lowest sensor on the path (nested sensors).
//     outer Group[sensorOuter, inner Group[sensorInner, box]]
// ---------------------------------------------------------------------------
void test_lowest_sensor_wins() {
  auto outer = std::make_shared<Group>();
  auto sensorOuter = std::make_shared<TouchSensor>();
  auto inner = std::make_shared<Group>();
  auto sensorInner = std::make_shared<TouchSensor>();
  auto shape = boxShape();
  addChild(inner, sensorInner);
  addChild(inner, shape);
  addChild(outer, sensorOuter);
  addChild(outer, inner);
  Scene scene; scene.addRootNode(outer);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ());
  r.ctx.tick(1.0);
  check(sensorInner->getIsOver() == true, "(2) lowest (inner) sensor resolves");
  check(sensorOuter->getIsOver() == false, "(2) outer sensor gets nothing");
}

// ---------------------------------------------------------------------------
// (3) nearest geometry's sensor wins (two boxes at different depths).
//     groupNear[sensorNear, boxNear@z=1]  groupFar[sensorFar, boxFar@z=-5]
// ---------------------------------------------------------------------------
void test_nearest_geometry() {
  auto root = std::make_shared<Group>();
  // near
  auto gNear = std::make_shared<Group>();
  auto sNear = std::make_shared<TouchSensor>();
  auto tNear = std::make_shared<Transform>();
  setF(tNear, "translation", std::any(SFVec3f{0, 0, 2})); // box surface ~z=3
  addChild(tNear, boxShape());
  addChild(gNear, sNear);
  addChild(gNear, tNear);
  // far
  auto gFar = std::make_shared<Group>();
  auto sFar = std::make_shared<TouchSensor>();
  auto tFar = std::make_shared<Transform>();
  setF(tFar, "translation", std::any(SFVec3f{0, 0, -5}));
  addChild(tFar, boxShape());
  addChild(gFar, sFar);
  addChild(gFar, tFar);
  addChild(root, gNear);
  addChild(root, gFar);
  Scene scene; scene.addRootNode(root);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ());
  r.ctx.tick(1.0);
  check(sNear->getIsOver() == true, "(3) nearer geometry's sensor resolves");
  check(sFar->getIsOver() == false, "(3) farther sensor gets nothing");
}

// ---------------------------------------------------------------------------
// (4) hitPoint/hitNormal in the sensor frame under a translated Transform.
//     Transform(translation (5,0,0)) > Group[sensor, box]. World hit at
//     (5,0,1) on the +Z face → sensor-local (0,0,1); normal world +Z → +Z.
// ---------------------------------------------------------------------------
void test_hit_in_sensor_frame_translated() {
  auto xform = std::make_shared<Transform>();
  setF(xform, "translation", std::any(SFVec3f{5, 0, 0}));
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  addChild(group, sensor);
  addChild(group, boxShape());
  addChild(xform, group);
  Scene scene; scene.addRootNode(xform);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(Ray{{5, 0, 10}, {0, 0, -1}}); // straight at the moved box
  r.ctx.tick(1.0);
  check(sensor->getIsOver() == true, "(4) over the translated box");
  SFVec3f hp = sensor->getHitPoint_changed();
  check(feq(hp.x, 0) && feq(hp.y, 0) && feq(hp.z, 1),
        "(4) hitPoint in sensor frame == (0,0,1)");
  SFVec3f hn = sensor->getHitNormal_changed();
  check(feq(hn.x, 0) && feq(hn.y, 0) && feq(hn.z, 1),
        "(4) hitNormal in sensor frame == (0,0,1)");
}

// ---------------------------------------------------------------------------
// (4) rotated Transform: 90° about +Y. World +Z face normal rotates so that,
//     pulled back into the sensor frame, hitNormal is again the box-local +Z.
//     A ray from +X hits the rotated box's face that points toward +X in world.
// ---------------------------------------------------------------------------
void test_hit_in_sensor_frame_rotated() {
  auto xform = std::make_shared<Transform>();
  // 90° about Y: box-local +Z maps to world +X.
  setF(xform, "rotation", std::any(SFRotation{0, 1, 0, 1.57079632679f}));
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  addChild(group, sensor);
  addChild(group, boxShape());
  addChild(xform, group);
  Scene scene; scene.addRootNode(xform);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  // Ray from +X toward -X hits the face whose world-normal is +X.
  r.ctx.setPointer(Ray{{10, 0, 0}, {-1, 0, 0}});
  r.ctx.tick(1.0);
  check(sensor->getIsOver() == true, "(4r) over the rotated box");
  // World hit point (1,0,0); pulled back through the 90°-Y rotation → local
  // box +Z face: (0,0,1).
  SFVec3f hp = sensor->getHitPoint_changed();
  check(feq(hp.x, 0) && feq(hp.y, 0) && feq(hp.z, 1),
        "(4r) hitPoint in rotated sensor frame == (0,0,1)");
  SFVec3f hn = sensor->getHitNormal_changed();
  check(feq(hn.x, 0) && feq(hn.y, 0) && feq(hn.z, 1),
        "(4r) hitNormal in rotated sensor frame == box-local +Z");
}

// ---------------------------------------------------------------------------
// (4b) hitTexCoord — PRIMITIVE (Box §13.3.1) AND invariance under the sensor
//      transform. Box +Z face center → texcoord (0.5,0.5).
// ---------------------------------------------------------------------------
void test_hitTexCoord_primitive_invariant() {
  auto makeScene = [](SFVec3f tr) {
    auto xform = std::make_shared<Transform>();
    setF(xform, "translation", std::any(tr));
    auto group = std::make_shared<Group>();
    auto sensor = std::make_shared<TouchSensor>();
    addChild(group, sensor);
    addChild(group, boxShape());
    addChild(xform, group);
    return std::make_pair(xform, sensor);
  };
  // Untransformed.
  {
    auto [xform, sensor] = makeScene({0, 0, 0});
    Scene scene; scene.addRootNode(xform);
    Rig r; r.build(scene);
    r.ctx.setPointerPresent(true);
    r.ctx.setPointer(downZ());
    r.ctx.tick(1.0);
    SFVec2f tc = sensor->getHitTexCoord_changed();
    check(feq(tc.x, 0.5f) && feq(tc.y, 0.5f),
          "(4b) Box +Z face center texcoord == (0.5,0.5)");
  }
  // Translated — texcoord is a surface attribute, must be identical.
  {
    auto [xform, sensor] = makeScene({5, 0, 0});
    Scene scene; scene.addRootNode(xform);
    Rig r; r.build(scene);
    r.ctx.setPointerPresent(true);
    r.ctx.setPointer(Ray{{5, 0, 10}, {0, 0, -1}});
    r.ctx.tick(1.0);
    SFVec2f tc = sensor->getHitTexCoord_changed();
    check(feq(tc.x, 0.5f) && feq(tc.y, 0.5f),
          "(4b) Box texcoord invariant under sensor transform");
  }
}

// ---------------------------------------------------------------------------
// (4b) hitTexCoord — MESH (barycentric). A single quad in the z=0 plane spanning
//      x,y in [-1,1] with texcoords mapping the unit square. A ray down -Z at
//      the quad center hits texcoord (0.5,0.5).
// ---------------------------------------------------------------------------
void test_hitTexCoord_mesh_barycentric() {
  auto ifs = std::make_shared<IndexedFaceSet>();
  auto coord = std::make_shared<Coordinate>();
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{
           {-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}}));
  auto tcn = std::make_shared<TextureCoordinate>();
  setF(tcn, "point",
       std::any(std::vector<SFVec2f>{{0, 0}, {1, 0}, {1, 1}, {0, 1}}));
  setF(ifs, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(ifs, "texCoord", std::any(std::shared_ptr<X3DNode>(tcn)));
  setF(ifs, "coordIndex", std::any(std::vector<int>{0, 1, 2, 3, -1}));
  auto shape = std::make_shared<Shape>();
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(ifs)));

  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  addChild(group, sensor);
  addChild(group, shape);
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}}); // center of the quad
  r.ctx.tick(1.0);
  check(sensor->getIsOver() == true, "(4b-mesh) over the quad");
  SFVec2f tc = sensor->getHitTexCoord_changed();
  check(feq(tc.x, 0.5f, 1e-2f) && feq(tc.y, 0.5f, 1e-2f),
        "(4b-mesh) barycentric texcoord at quad center == (0.5,0.5)");
}

// ---------------------------------------------------------------------------
// (5) isActive + touchTime click. Down-over→up-over ⇒ touchTime==now;
//     up-after-leave ⇒ no touchTime.
// ---------------------------------------------------------------------------
void test_isActive_touchTime() {
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  addChild(group, sensor);
  addChild(group, boxShape());
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ());
  r.ctx.tick(1.0);
  check(sensor->getIsActive() == false, "(5) not active before button");

  // Button down while over → isActive TRUE.
  r.ctx.setPointerButton(true);
  r.ctx.tick(2.0);
  check(sensor->getIsActive() == true, "(5) isActive TRUE on button-down over");

  // Button up still over → isActive FALSE + touchTime == now.
  r.ctx.setPointerButton(false);
  r.ctx.tick(3.0);
  check(sensor->getIsActive() == false, "(5) isActive FALSE on button-up");
  check(std::fabs(sensor->getTouchTime() - 3.0) < 1e-9,
        "(5) touchTime == now on up-while-over");

  // Second click that releases AFTER leaving → no touchTime update.
  double before = sensor->getTouchTime();
  r.ctx.setPointer(downZ());
  r.ctx.setPointerButton(true);
  r.ctx.tick(4.0); // grab begins, over
  check(sensor->getIsActive() == true, "(5) second grab started");
  r.ctx.setPointer(Ray{{100, 0, 10}, {0, 0, -1}}); // leave
  r.ctx.tick(5.0); // still grabbed, no longer over
  r.ctx.setPointerButton(false);
  r.ctx.tick(6.0); // release after leaving
  check(sensor->getIsActive() == false, "(5) isActive FALSE after up-off");
  check(std::fabs(sensor->getTouchTime() - before) < 1e-9,
        "(5) NO touchTime when released off the geometry");
}

// ---------------------------------------------------------------------------
// (DS-2) enabled=FALSE during an active drag: the grabbed sensor must stop
//        tracking and deactivate (isActive FALSE), not keep emitting.
// ---------------------------------------------------------------------------
void test_drag_enabled_false_mid_drag() {
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<PlaneSensor>();
  addChild(group, sensor);
  addChild(group, boxShape());
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ());
  r.ctx.setPointerButton(true);
  r.ctx.tick(1.0);
  check(sensor->getIsActive() == true, "(ds2) drag active on button-down over");

  // Disable the sensor mid-drag, then move the pointer (the system is
  // pointer-event-driven). On that next motion the grabbed-but-disabled sensor
  // must deactivate instead of emitting drag output.
  sensor->setEnabled(false);
  r.ctx.setPointer(Ray{{0, 1, 10}, {0, 0, -1}});
  r.ctx.tick(2.0);
  check(sensor->getIsActive() == false,
        "(ds2) enabled=FALSE mid-drag deactivates the grabbed sensor on next motion");
}

// ---------------------------------------------------------------------------
// (6) grab exclusivity. During an active drag, moving onto a second sensor's
//     geometry must emit no events to the second sensor.
// ---------------------------------------------------------------------------
void test_grab_exclusivity() {
  auto root = std::make_shared<Group>();
  auto gA = std::make_shared<Group>();
  auto sA = std::make_shared<TouchSensor>();
  addChild(gA, sA);
  addChild(gA, boxShape()); // at origin, surface z=1
  auto gB = std::make_shared<Group>();
  auto sB = std::make_shared<TouchSensor>();
  auto tB = std::make_shared<Transform>();
  setF(tB, "translation", std::any(SFVec3f{20, 0, 0}));
  addChild(tB, boxShape());
  addChild(gB, sB);
  addChild(gB, tB);
  addChild(root, gA);
  addChild(root, gB);
  Scene scene; scene.addRootNode(root);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ()); // over A
  r.ctx.tick(1.0);
  r.ctx.setPointerButton(true);
  r.ctx.tick(2.0); // grab A
  check(sA->getIsActive() == true, "(6) sensor A grabbed");

  // Move onto B's geometry while grabbed: B must receive nothing.
  r.ctx.setPointer(Ray{{20, 0, 10}, {0, 0, -1}});
  r.ctx.tick(3.0);
  check(sB->getIsOver() == false, "(6) B gets no isOver during A's grab");
  check(sB->getIsActive() == false, "(6) B never activates during A's grab");
  check(sA->getIsActive() == true, "(6) A still owns the grab");
}

// ---------------------------------------------------------------------------
// (7) enabled=FALSE: disabled sensor produces nothing; next-lowest enabled
//     sensor resolves instead. inner sensor disabled → outer resolves.
// ---------------------------------------------------------------------------
void test_disabled_sensor_skipped() {
  auto outer = std::make_shared<Group>();
  auto sensorOuter = std::make_shared<TouchSensor>();
  auto inner = std::make_shared<Group>();
  auto sensorInner = std::make_shared<TouchSensor>();
  sensorInner->setEnabled(false); // disabled → skipped
  addChild(inner, sensorInner);
  addChild(inner, boxShape());
  addChild(outer, sensorOuter);
  addChild(outer, inner);
  Scene scene; scene.addRootNode(outer);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ());
  r.ctx.tick(1.0);
  check(sensorInner->getIsOver() == false, "(7) disabled inner gets nothing");
  check(sensorOuter->getIsOver() == true,
        "(7) next-lowest enabled (outer) resolves");
}

// ---------------------------------------------------------------------------
// (8) PlaneSensor drag dispatch (M2D §20.4.2 + §20.2.2). A PlaneSensor sibling
//     of a Box at the origin (surface +Z face at z=1, identity sensor frame).
//     - button-down over the box → isActive TRUE, no value yet
//     - drag the ray to x=+0.5 → trackPoint_changed=(0.5,0,1),
//       translation_changed=(0.5,0,0) (default unclamped, offset 0)
//     - button-up with autoOffset (default TRUE) → offset == last translation
//       and isActive FALSE
// ---------------------------------------------------------------------------
void test_planesensor_drag_and_autoOffset() {
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<PlaneSensor>();
  addChild(group, sensor);
  addChild(group, boxShape()); // box at origin; +Z face at z=1
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ()); // over the box at (0,0,1)
  r.ctx.tick(1.0);
  check(sensor->getIsOver() == true, "(8) PlaneSensor isOver over the box");
  check(sensor->getIsActive() == false, "(8) not active before button-down");

  // Button down while over → grab + isActive TRUE.
  r.ctx.setPointerButton(true);
  r.ctx.tick(2.0);
  check(sensor->getIsActive() == true, "(8) isActive TRUE on button-down over");

  // Drag to x=+0.5 (ray still down -Z): tracking plane z=1 unchanged.
  r.ctx.setPointer(Ray{{0.5f, 0, 10}, {0, 0, -1}});
  r.ctx.tick(3.0);
  SFVec3f tp = sensor->getTrackPoint_changed();
  check(feq(tp.x, 0.5f) && feq(tp.y, 0) && feq(tp.z, 1),
        "(8) trackPoint_changed == (0.5,0,1)");
  SFVec3f tr = sensor->getTranslation_changed();
  check(feq(tr.x, 0.5f) && feq(tr.y, 0) && feq(tr.z, 0),
        "(8) translation_changed == (0.5,0,0) [default unclamped, offset 0]");

  // Button up with autoOffset default TRUE → offset ← last translation_changed.
  check(sensor->getAutoOffset() == true, "(8) autoOffset default TRUE");
  r.ctx.setPointerButton(false);
  r.ctx.tick(4.0);
  check(sensor->getIsActive() == false, "(8) isActive FALSE on button-up");
  SFVec3f off = sensor->getOffset();
  check(feq(off.x, 0.5f) && feq(off.y, 0) && feq(off.z, 0),
        "(8) autoOffset wrote offset == last translation_changed (0.5,0,0)");
}

// ---------------------------------------------------------------------------
// (9) PlaneSensor offset accumulation + min/max clamp. With offset=(1,0,0) and
//     minPosition=(0,0) maxPosition=(2,0) (Y locked at 0): a drag of +0.5 in X
//     gives unclamped X = 0.5 + 1.0 = 1.5 (within [0,2]); Y locked to 0.
// ---------------------------------------------------------------------------
void test_planesensor_offset_and_clamp() {
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<PlaneSensor>();
  setF(sensor, "offset", std::any(SFVec3f{1, 0, 0}));
  setF(sensor, "minPosition", std::any(SFVec2f{0, 0}));
  setF(sensor, "maxPosition", std::any(SFVec2f{2, 0})); // X clamps [0,2]; Y locked 0
  addChild(group, sensor);
  addChild(group, boxShape());
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);

  r.ctx.setPointerPresent(true);
  r.ctx.setPointer(downZ());
  r.ctx.tick(1.0);
  r.ctx.setPointerButton(true);
  r.ctx.tick(2.0);
  check(sensor->getIsActive() == true, "(9) active");

  r.ctx.setPointer(Ray{{0.5f, 3.0f, 10}, {0, 0, -1}}); // drag +0.5 X, +3 Y
  r.ctx.tick(3.0);
  SFVec3f tr = sensor->getTranslation_changed();
  check(feq(tr.x, 1.5f), "(9) X = delta(0.5)+offset(1.0)=1.5 within [0,2]");
  check(feq(tr.y, 0.0f), "(9) Y locked to 0 (minY==maxY line sensor)");
}

} // namespace

TEST_CASE("pointing_sensor_test") {
  test_isOver_and_no_event_when_unchanged();
  test_lowest_sensor_wins();
  test_nearest_geometry();
  test_hit_in_sensor_frame_translated();
  test_hit_in_sensor_frame_rotated();
  test_hitTexCoord_primitive_invariant();
  test_hitTexCoord_mesh_barycentric();
  test_isActive_touchTime();
  test_drag_enabled_false_mid_drag();
  test_grab_exclusivity();
  test_disabled_sensor_skipped();
  test_planesensor_drag_and_autoOffset();
  test_planesensor_offset_and_clamp();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all PointingSensorSystem tests passed\n";
  return;
}

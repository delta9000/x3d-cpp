#include "doctest/doctest.h"
// An authored TimeSensor -> Interpolator -> Transform chain animates on tick.
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Transform.hpp"

#include <cmath>
#include <memory>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

TEST_CASE("standard_runtime_test") {
  // TimeSensor drives a PositionInterpolator that translates a Transform 0→10 in x.
  const char *xml =
      "<X3D profile='Interactive'><Scene>"
      "<TimeSensor DEF='TS' cycleInterval='4' loop='true'/>"
      "<PositionInterpolator DEF='PI' key='0 1' keyValue='0 0 0 10 0 0'/>"
      "<Transform DEF='T'><Shape><Box/></Shape></Transform>"
      "<ROUTE fromNode='TS' fromField='fraction_changed' toNode='PI' toField='set_fraction'/>"
      "<ROUTE fromNode='PI' fromField='value_changed' toNode='T' toField='translation'/>"
      "</Scene></X3D>";

  auto doc = x3d::codec::parseDocument(xml);
  Scene &scene = doc.getScene();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  BridgeResult bridge = ctx.buildFrom(scene);
  CHECK_MESSAGE((bridge.ok()), ("ROUTEs wired (TimeSensor→interp→Transform)"));

  attachStandardRuntime(scene, ctx);

  // Find the Transform (a root-level sibling in this scene).
  Transform *t = nullptr;
  for (auto &r : scene.rootNodes)
    if (auto *p = dynamic_cast<Transform *>(r.get())) t = p;
  CHECK_MESSAGE((t != nullptr), ("found the Transform node"));
  if (!t) { CHECK(false); return; }

  const float x0 = t->getTranslation().x;
  CHECK_MESSAGE((std::fabs(x0) < 1e-6f), ("translation starts at 0"));

  // Tick across the cycle; TimeSensor.fraction_changed should drive the interp,
  // whose value_changed routes into the Transform's translation.
  ctx.tick(0.0);
  for (double now = 0.5; now <= 2.0; now += 0.5) ctx.tick(now);

  const float x2 = t->getTranslation().x;
  CHECK_MESSAGE((x2 > 1.0f), ("standard runtime animates: TimeSensor→PositionInterpolator→Transform.translation"));

  return;
}

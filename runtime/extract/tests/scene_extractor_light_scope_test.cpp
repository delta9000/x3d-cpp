// scene_extractor_light_scope_test.cpp —  acceptance: non-global DirectionalLight
// scope is honored by tagging RenderItems with in-scope lights only.
//
// X3D §17 / X3DLightNode semantics:
//   * global=TRUE  => light illuminates the WHOLE scene (all RenderItems).
//   * global=FALSE => light illuminates only geometry in the scope of its parent
//     grouping node (the scopeRoot). A RenderItem is in-scope iff the light's
//     scopeRoot is an ANCESTOR (or equal to the last grouping node on the path)
//     of that item's PathKey.
//
// Scene under test:
//
//   Group (outerGroup)
//     ├─ Group (scopeGroup)            ← scopeRoot for the scoped DirectionalLight
//     │    ├─ DirectionalLight(global=false)   [scoped light: only inside scopeGroup]
//     │    └─ Shape (shapeInside)              [INSIDE scope]
//     ├─ Shape (shapeOutside)                  [outside scope]
//     └─ PointLight(global=true)               [global light: illuminates all]
//
// Assertions:
//   A) shapeInside's RenderItem carries both lights (scoped DL + global PL).
//   B) shapeOutside's RenderItem carries only the global PointLight (not the DL).
//   C) A scene with only a global=TRUE PointLight tags all render items.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include "scene_test_helpers.hpp"
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using x3d::test::setF;
using x3d::test::addChild;

static std::shared_ptr<X3DNode> makeTriShape() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto tri = createX3DNode("TriangleSet");
  setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(tri)));
  return shape;
}

TEST_CASE("scene_extractor_m25_5_test") {
  // A+B) Scoped light tags INSIDE shape but not outside shape
  {
    // Build: outerGroup > { scopeGroup > { DL(global=false), shapeInside },
    //                       shapeOutside, PointLight(global=true) }
    auto dl = createX3DNode("DirectionalLight");
    setF(dl, "global", std::any(false)); // explicit false (spec default for DL)

    auto shapeInside = makeTriShape();
    auto scopeGroup = createX3DNode("Group");
    addChild(scopeGroup, dl);
    addChild(scopeGroup, shapeInside);

    auto shapeOutside = makeTriShape();

    auto pl = createX3DNode("PointLight");
    setF(pl, "global", std::any(true)); // explicit true (spec default for PL)

    auto outerGroup = createX3DNode("Group");
    addChild(outerGroup, scopeGroup);
    addChild(outerGroup, shapeOutside);
    addChild(outerGroup, pl);

    Scene scene;
    scene.addRootNode(outerGroup);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 2)); // exactly two shapes emitted.

    // Find which RenderItemId corresponds to which shape.
    extract::RenderItemId idInside = extract::kInvalidRenderItemId;
    extract::RenderItemId idOutside = extract::kInvalidRenderItemId;
    for (extract::RenderItemId id : snap.added) {
      const extract::RenderItem &rec = ex.item(id);
      for (const X3DNode *n : rec.path) {
        if (n == shapeInside.get()) { idInside = id; break; }
        if (n == shapeOutside.get()) { idOutside = id; break; }
      }
    }
    CHECK((idInside != extract::kInvalidRenderItemId));
    CHECK((idOutside != extract::kInvalidRenderItemId));

    // Retrieve the tagged light lists. lightsOf() indices are positions in
    // snapshotLights() (the cached snapshot from fullSnapshot), not lights().
    const auto &allLights = ex.snapshotLights();
    CHECK((allLights.size() == 2)); // DL + PL collected.

    const std::vector<std::size_t> &insideLights = ex.lightsOf(idInside);
    const std::vector<std::size_t> &outsideLights = ex.lightsOf(idOutside);

    // A) shapeInside must be tagged with both lights.
    CHECK((insideLights.size() == 2));

    // B) shapeOutside must be tagged with only the global PointLight, not the DL.
    CHECK((outsideLights.size() == 1));
    // The one light tagging shapeOutside must be global.
    CHECK((allLights[outsideLights[0]].global == true));
    CHECK((allLights[outsideLights[0]].type == extract::LightDesc::Type::Point));

    // Double-check insideLights contains both types.
    bool hasDir = false, hasPoint = false;
    for (std::size_t li : insideLights) {
      if (allLights[li].type == extract::LightDesc::Type::Directional) hasDir = true;
      if (allLights[li].type == extract::LightDesc::Type::Point) hasPoint = true;
    }
    CHECK((hasDir && hasPoint));
  }

  // C) global=TRUE PointLight alone tags all render items
  {
    auto pl = createX3DNode("PointLight");
    setF(pl, "global", std::any(true));

    auto shape1 = makeTriShape();
    auto shape2 = makeTriShape();

    auto group = createX3DNode("Group");
    addChild(group, pl);
    addChild(group, shape1);
    addChild(group, shape2);

    Scene scene;
    scene.addRootNode(group);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 2));

    for (extract::RenderItemId id : snap.added) {
      const std::vector<std::size_t> &lit = ex.lightsOf(id);
      CHECK((lit.size() == 1)); // exactly the one global light.
    }
  }

  // D) No lights => all items have empty light list
  {
    auto shape = makeTriShape();
    Scene scene;
    scene.addRootNode(shape);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));
    CHECK((ex.lightsOf(snap.added[0]).empty()));
  }

  return;
}

TEST_CASE("delta oracle: lightsChanged exposes current cached descriptors and scope") {
  int edit = 0;
  SUBCASE("on") {}
  SUBCASE("global") { edit = 1; }
  SUBCASE("color") { edit = 2; }
  SUBCASE("ancestor translation") { edit = 3; }
  SUBCASE("enabled from empty cache") { edit = 4; }
  auto light = createX3DNode("PointLight"), parent = createX3DNode("Transform");
  setF(light, "global", false);
  if (edit == 4) setF(light, "on", false);
  addChild(parent, light); addChild(parent, makeTriShape());
  Scene scene; scene.rootNodes = {parent,makeTriShape()};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  REQUIRE(ex.fullSnapshot().added.size() == 2);
  if (edit == 0) ctx.postEvent(light.get(), "on", false);
  if (edit == 1) ctx.postEvent(light.get(), "global", true);
  if (edit == 2) ctx.postEvent(light.get(), "color", SFColor{1,0,0});
  if (edit == 3) ctx.postEvent(parent.get(), "translation", SFVec3f{3,0,0});
  if (edit == 4) ctx.postEvent(light.get(), "on", true);
  ctx.tick(1); CHECK(ex.delta().lightsChanged);
  auto current = ex.lights();
  CHECK(ex.snapshotLights().size() == current.size());
  if (!current.empty() && !ex.snapshotLights().empty()) {
    CHECK(ex.snapshotLights()[0].global == current[0].global);
    CHECK(ex.snapshotLights()[0].color.g == doctest::Approx(current[0].color.g));
    CHECK(ex.snapshotLights()[0].worldLocation.x == doctest::Approx(current[0].worldLocation.x));
  }
  const std::size_t outsideCount = !current.empty() && current[0].global ? 1 : 0;
  CHECK(ex.item(1).lights.size() == outsideCount);
}

TEST_CASE("local lights and projectors retain their USE placement scope") {
  auto shared = createX3DNode("Group");
  auto light = createX3DNode("DirectionalLight");
  auto projector = createX3DNode("TextureProjector");
  setF(light, "global", std::any(false));
  setF(projector, "global", std::any(false));
  addChild(shared, light);
  addChild(shared, projector);
  addChild(shared, makeTriShape());
  auto left = createX3DNode("Transform"), right = createX3DNode("Transform");
  setF(right, "rotation", std::any(SFRotation{0, 1, 0, 1.57079632679f}));
  addChild(left, shared);
  addChild(right, shared);
  Scene scene;
  scene.addRootNode(left);
  scene.addRootNode(right);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  REQUIRE(snap.added.size() == 2);
  REQUIRE(ex.snapshotLights().size() == 2);
  auto projectors = ex.projectors();
  REQUIRE(projectors.size() == 2);
  for (auto id : snap.added) {
    const auto &item = ex.item(id);
    REQUIRE(ex.lightsOf(id).size() == 1);
    auto index = ex.lightsOf(id).front();
    CHECK(ex.snapshotLights()[index].scopePath.front() == item.path.front());
    unsigned applicable = 0;
    for (const auto &p : projectors) {
      if (extract::projectorApplies(p, item)) {
        ++applicable;
        CHECK(p.scopePath.front() == item.path.front());
      }
    }
    CHECK(applicable == 1);
  }
}

TEST_CASE("Billboard light and projector frames follow the tracked view") {
  auto billboard = createX3DNode("Billboard");
  auto light = createX3DNode("PointLight");
  auto projector = createX3DNode("TextureProjector");
  setF(light, "location", std::any(SFVec3f{0, 0, 1}));
  setF(projector, "location", std::any(SFVec3f{0, 0, 1}));
  addChild(billboard, light);
  addChild(billboard, projector);
  Scene scene;
  scene.addRootNode(billboard);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.setHeadPose({0, 0, 10}, {0, 1, 0, 0});
  ctx.tick(0.0);
  extract::SceneExtractor ex(ctx, scene);
  ex.fullSnapshot();
  REQUIRE(ex.lights().size() == 1);
  CHECK(ex.lights()[0].worldLocation.z == doctest::Approx(1));
  ctx.setHeadPose({10, 0, 0}, {0, 1, 0, 0});
  ctx.tick(0.1);
  auto delta = ex.delta();
  CHECK(delta.lightsChanged);
  auto lights = ex.lights();
  CHECK(lights[0].worldLocation.x == doctest::Approx(1));
  CHECK(lights[0].worldLocation.z == doctest::Approx(0).epsilon(0.0001));
  auto projectors = ex.projectors();
  REQUIRE(projectors.size() == 1);
  CHECK(projectors[0].worldLocation.x == doctest::Approx(1));
  CHECK(projectors[0].worldLocation.z == doctest::Approx(0).epsilon(0.0001));
  ctx.tick(0.2);
  CHECK_FALSE(ex.delta().lightsChanged);
}

TEST_CASE("free Billboard lights invalidate on camera roll without eye motion") {
  auto billboard = createX3DNode("Billboard");
  setF(billboard, "axisOfRotation", std::any(SFVec3f{0, 0, 0}));
  auto light = createX3DNode("PointLight");
  setF(light, "location", std::any(SFVec3f{1, 0, 0}));
  addChild(billboard, light);
  Scene scene;
  scene.addRootNode(billboard);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.setHeadPose({0, 0, 10}, {0, 1, 0, 0});
  ctx.tick(0.0);
  extract::SceneExtractor ex(ctx, scene);
  ex.fullSnapshot();
  REQUIRE(ex.snapshotLights().size() == 1);
  CHECK(ex.snapshotLights()[0].worldLocation.x == doctest::Approx(1));
  const auto eye = ctx.cameraWorldPosition();
  ctx.setHeadPose({0, 0, 10}, {0, 0, 1, 1.57079632679f});
  ctx.tick(0.1);
  REQUIRE(ctx.cameraWorldPosition() == eye);
  CHECK(ex.delta().lightsChanged);
  REQUIRE(ex.snapshotLights().size() == 1);
  CHECK(ex.snapshotLights()[0].worldLocation.x == doctest::Approx(0).epsilon(0.0001));
  CHECK(std::fabs(ex.snapshotLights()[0].worldLocation.y) == doctest::Approx(1));
  CHECK(ex.snapshotLights()[0].worldLocation == ex.lights()[0].worldLocation);
  ctx.tick(0.2);
  CHECK_FALSE(ex.delta().lightsChanged);
}

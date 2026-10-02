// Camera-dependent render state must agree with a separate authoritative walk.
#include "SceneExtractor.hpp"
#include "ViewDependentSystem.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"
#include <any>
#include <map>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

namespace {
using Node = std::shared_ptr<X3DNode>;
void setInitial(const Node &node, const char *name, std::any value) {
  for (const auto &field : node->fields())
    if (field.x3dName == name) { field.set(*node, std::move(value)); return; }
  FAIL("fixture field missing");
}
Node makeShape(const char *type) {
  auto shape = createX3DNode("Shape");
  setInitial(shape, "geometry", createX3DNode(type));
  return shape;
}
struct Mirror {
  std::map<RenderItemId, RenderItem> items;
  void apply(const RenderDelta &delta, const SceneExtractor &extractor) {
    for (auto id : delta.removed) items.erase(id);
    for (auto id : delta.added) items.insert_or_assign(id, extractor.item(id));
    for (const auto *ids : {&delta.updatedTransform, &delta.updatedGeometry,
                            &delta.updatedMaterial})
      for (auto id : *ids) {
        REQUIRE(items.count(id) == 1);
        items.insert_or_assign(id, extractor.item(id));
      }
  }
  void check(const X3DExecutionContext &context, const Scene &scene) const {
    SceneExtractor oracle(context, scene);
    const auto snapshot = oracle.fullSnapshot();
    REQUIRE(items.size() == snapshot.added.size());
    for (auto id : snapshot.added) {
      const auto &expected = oracle.item(id);
      auto actual = std::find_if(items.begin(), items.end(), [&](const auto &entry) {
        return entry.second.path == expected.path;
      });
      REQUIRE(actual != items.end());
      for (std::size_t i = 0; i != 16; ++i)
        CHECK(actual->second.worldTransform.m[i] == doctest::Approx(expected.worldTransform.m[i]));
      CHECK(actual->second.mesh->positions == expected.mesh->positions);
    }
  }
};
}

TEST_CASE("view delta: shared Billboards follow tracked head pose without rebuilding meshes") {
  auto viewpoint = createX3DNode("Viewpoint");
  setInitial(viewpoint, "position", SFVec3f{0, 0, 10});
  auto billboard = createX3DNode("Billboard");
  setInitial(billboard, "children", MFNode{makeShape("Box")});
  auto left = createX3DNode("Transform"), right = createX3DNode("Transform");
  setInitial(left, "translation", SFVec3f{-3, 0, 0});
  setInitial(right, "translation", SFVec3f{3, 0, 0});
  setInitial(left, "children", MFNode{billboard});
  setInitial(right, "children", MFNode{billboard});
  Scene scene; scene.rootNodes = {viewpoint, left, right};
  X3DExecutionContext context; context.buildSceneGraph(scene);
  SceneExtractor extractor(context, scene);
  Mirror mirror; mirror.apply(extractor.fullSnapshot(), extractor);
  REQUIRE(mirror.items.size() == 2);
  auto retained = mirror.items.begin()->second.mesh;
  context.setHeadPose({10, 0, -10}, {0, 1, 0, 0});
  context.tick(1);
  auto delta = extractor.delta();
  CHECK(delta.added.empty()); CHECK(delta.removed.empty());
  CHECK(delta.updatedGeometry.empty());
  CHECK(delta.updatedTransform.size() == 2);
  mirror.apply(delta, extractor); mirror.check(context, scene);
  CHECK(mirror.items.begin()->second.mesh == retained);
  CHECK(std::next(mirror.items.begin())->second.mesh == retained);

  context.tick(2); // unchanged view is not a fresh transformation update
  delta = extractor.delta();
  CHECK(delta.updatedTransform.empty());
  mirror.apply(delta, extractor); mirror.check(context, scene);

  context.postEvent(left.get(), "translation", SFVec3f{-6, 0, 1});
  context.tick(3);
  delta = extractor.delta();
  CHECK(delta.updatedTransform.size() == 1);
  mirror.apply(delta, extractor); mirror.check(context, scene);

  context.postEvent(billboard.get(), "axisOfRotation", SFVec3f{0, 0, 0});
  context.setHeadPose({8, 4, -7}, {0, 0, 1, 0.3f});
  context.tick(4);
  delta = extractor.delta();
  CHECK(delta.updatedTransform.size() == 2);
  mirror.apply(delta, extractor); mirror.check(context, scene);
}

TEST_CASE("view delta: translated shared LOD selects every placement after head motion") {
  auto viewpoint = createX3DNode("Viewpoint");
  setInitial(viewpoint, "position", SFVec3f{20, 0, 0});
  auto lod = createX3DNode("LOD");
  setInitial(lod, "range", MFFloat{5});
  setInitial(lod, "children", MFNode{makeShape("Box"), makeShape("Sphere")});
  auto left = createX3DNode("Transform"), right = createX3DNode("Transform");
  setInitial(left, "translation", SFVec3f{-10, 0, 0});
  setInitial(right, "translation", SFVec3f{10, 0, 0});
  setInitial(left, "children", MFNode{lod});
  setInitial(right, "children", MFNode{lod});
  Scene scene; scene.rootNodes = {viewpoint, left, right};
  X3DExecutionContext context; context.buildSceneGraph(scene);
  auto view = std::make_shared<ViewDependentSystem>();
  view->attach(lod.get(), context); context.addSystem(view);
  context.tick(0);
  SceneExtractor extractor(context, scene);
  Mirror mirror; mirror.apply(extractor.fullSnapshot(), extractor);
  context.setHeadPose({-12, 0, 0}, {0, 1, 0, 0});
  context.tick(1);
  auto delta = extractor.delta();
  CHECK(delta.removed.size() == 2); CHECK(delta.added.size() == 2);
  mirror.apply(delta, extractor); mirror.check(context, scene);

  context.tick(2);
  delta = extractor.delta();
  CHECK(delta.removed.empty()); CHECK(delta.added.empty());
  mirror.apply(delta, extractor); mirror.check(context, scene);

  context.postEvent(right.get(), "translation", SFVec3f{30, 0, 0});
  context.tick(3);
  delta = extractor.delta();
  CHECK(delta.removed.size() == 2); CHECK(delta.added.size() == 2);
  mirror.apply(delta, extractor); mirror.check(context, scene);
}

TEST_CASE("view delta: LOD records inactive empty placements") {
  auto viewpoint = createX3DNode("Viewpoint");
  setInitial(viewpoint, "position", SFVec3f{20, 0, 0});
  auto lod = createX3DNode("LOD");
  setInitial(lod, "range", MFFloat{5});
  setInitial(lod, "children", MFNode{makeShape("Box"), createX3DNode("Group")});
  Scene scene; scene.rootNodes = {viewpoint, lod};
  X3DExecutionContext context; context.buildSceneGraph(scene);
  SceneExtractor extractor(context, scene);
  Mirror mirror; mirror.apply(extractor.fullSnapshot(), extractor);
  REQUIRE(mirror.items.empty());
  context.setHeadPose({-20, 0, 0}, {0, 1, 0, 0});
  context.tick(1);
  auto delta = extractor.delta();
  CHECK(delta.added.size() == 1);
  mirror.apply(delta, extractor); mirror.check(context, scene);
}

TEST_CASE("view delta: rematerialized Billboard uses the current tracked frame") {
  auto viewpoint = createX3DNode("Viewpoint");
  setInitial(viewpoint, "position", SFVec3f{0, 0, 10});
  auto coord = createX3DNode("Coordinate");
  const MFVec3f points{{0,0,0}, {1,0,0}, {0,1,0}};
  setInitial(coord, "point", points);
  auto geometry = createX3DNode("TriangleSet");
  setInitial(geometry, "coord", coord);
  auto shape = createX3DNode("Shape");
  setInitial(shape, "geometry", geometry);
  auto billboard = createX3DNode("Billboard");
  setInitial(billboard, "children", MFNode{shape});
  Scene scene; scene.rootNodes = {viewpoint, billboard};
  X3DExecutionContext context; context.buildSceneGraph(scene);
  SceneExtractor extractor(context, scene);
  Mirror mirror; mirror.apply(extractor.fullSnapshot(), extractor);
  REQUIRE(mirror.items.size() == 1);
  context.postEvent(coord.get(), "point", MFVec3f{});
  context.tick(1);
  mirror.apply(extractor.delta(), extractor);
  REQUIRE(mirror.items.empty());
  context.setHeadPose({10,0,-10}, {0,1,0,0});
  context.tick(2);
  mirror.apply(extractor.delta(), extractor);
  CHECK(mirror.items.empty());
  context.postEvent(coord.get(), "point", points);
  context.tick(3);
  auto delta = extractor.delta();
  REQUIRE(delta.added.size() == 1);
  mirror.apply(delta, extractor); mirror.check(context, scene);
  CHECK(extractor.item(delta.added.front()).worldTransform.m[0] == doctest::Approx(0));
}

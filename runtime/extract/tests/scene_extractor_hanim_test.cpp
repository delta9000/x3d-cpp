#include "SceneExtractor.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"

#include <any>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

namespace {
void set(const std::shared_ptr<X3DNode> &node, const char *name, std::any value) {
  for (const auto &field : node->fields())
    if (field.x3dName == name && field.set) { field.set(*node, std::move(value)); return; }
}

std::shared_ptr<X3DNode> shapeWith(const std::shared_ptr<X3DNode> &coord) {
  auto geom = createX3DNode("IndexedFaceSet");
  set(geom, "coord", std::shared_ptr<X3DNode>(coord));
  set(geom, "coordIndex", std::vector<int>{0, 1, 2, -1, 0, 2, 3, -1});
  auto shape = createX3DNode("Shape");
  set(shape, "geometry", std::shared_ptr<X3DNode>(geom));
  return shape;
}
}

TEST_CASE("HAnim skin placement, corner remap, pose delta and snapshot") {
  auto coord = createX3DNode("Coordinate");
  set(coord, "point", std::vector<SFVec3f>{{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}});
  auto skin = shapeWith(coord);
  auto joint = createX3DNode("HAnimJoint");
  auto segment = createX3DNode("HAnimSegment");
  auto segmentCoord = createX3DNode("Coordinate");
  set(segmentCoord, "point", std::vector<SFVec3f>{{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}});
  set(segment, "coord", std::shared_ptr<X3DNode>(segmentCoord));
  auto displacer = createX3DNode("HAnimDisplacer");
  set(displacer, "coordIndex", std::vector<int>{0});
  set(displacer, "displacements", std::vector<SFVec3f>{{0,0,1}});
  set(segment, "displacers", std::vector<std::shared_ptr<X3DNode>>{displacer});
  set(segment, "children", std::vector<std::shared_ptr<X3DNode>>{shapeWith(segmentCoord)});
  set(joint, "children", std::vector<std::shared_ptr<X3DNode>>{segment});
  auto jointDisplacer = createX3DNode("HAnimDisplacer");
  set(jointDisplacer, "coordIndex", std::vector<int>{1});
  set(jointDisplacer, "displacements", std::vector<SFVec3f>{{0,0,1}});
  set(joint, "displacers", std::vector<std::shared_ptr<X3DNode>>{jointDisplacer});
  auto humanoid = createX3DNode("HAnimHumanoid");
  set(humanoid, "skinCoord", std::shared_ptr<X3DNode>(coord));
  set(humanoid, "skin", std::vector<std::shared_ptr<X3DNode>>{skin});
  set(humanoid, "skeleton", std::vector<std::shared_ptr<X3DNode>>{joint});
  set(humanoid, "joints", std::vector<std::shared_ptr<X3DNode>>{joint});
  set(humanoid, "segments", std::vector<std::shared_ptr<X3DNode>>{segment});
  set(humanoid, "sites", std::vector<std::shared_ptr<X3DNode>>{createX3DNode("HAnimSite")});
  Scene scene;
  scene.addRootNode(humanoid);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  SceneExtractor extractor(ctx, scene);
  auto first = extractor.fullSnapshot();
  REQUIRE(first.added.size() == 2); // one skin, one rigid Segment; no references.
  const auto skinId = first.added[1];
  const auto &item = extractor.item(skinId);
  REQUIRE(item.skin.has_value());
  CHECK(item.skin->sourceCoordIndex == std::vector<std::uint32_t>{0,1,2,0,2,3});
  CHECK(item.skin->sourceCoordIndex.size() == item.mesh->positions.size());
  const auto bind = extractor.deformedMesh(skinId);
  CHECK(bind.positions == item.mesh->positions); // rest pose: deformed == bind

  const auto restBinding = extractor.item(skinId).skin->binding;
  ctx.tick(1.0);
  REQUIRE(ctx.writeField(joint.get(), "rotation", std::any(SFRotation{0,0,1,1})) ==
          FieldWriteResult::Ok);
  auto changed = extractor.delta();
  CHECK(changed.updatedSkinPose == std::vector<RenderItemId>{skinId});
  CHECK(changed.updatedGeometry.empty());
  const auto version = extractor.item(skinId).skin->poseVersion;
  CHECK(version > 0);
  extractor.fullSnapshot();
  CHECK(extractor.item(skinId).skin->poseVersion == version);
  CHECK(extractor.item(skinId).skin->binding == restBinding); // pose change: no recompile

  ctx.tick(2.0);
  REQUIRE(ctx.writeField(displacer.get(), "weight", std::any(1.0f)) ==
          FieldWriteResult::Ok);
  auto segmentChange = extractor.delta();
  CHECK(segmentChange.updatedGeometry == std::vector<RenderItemId>{first.added[0]});
  CHECK(segmentChange.updatedSkinPose.empty());
  // Segment displacer (0,0,1) at weight 1 moves the Segment mesh's point 0.
  CHECK(extractor.item(first.added[0]).mesh->positions[0].z == 1.0f);
  CHECK(extractor.item(first.added[0]).mesh->positions[1].z == 0.0f);
  // The authored Coordinate is untouched.
  CHECK(std::dynamic_pointer_cast<Coordinate>(segmentCoord)->getPoint()[0].z == 0.0f);

  const auto oldBinding = extractor.item(skinId).skin->binding;
  ctx.tick(2.5);
  REQUIRE(ctx.writeField(jointDisplacer.get(), "weight", std::any(0.5f)) ==
          FieldWriteResult::Ok);
  auto weightChange = extractor.delta();
  CHECK(weightChange.updatedSkinPose == std::vector<RenderItemId>{skinId});
  CHECK(extractor.item(skinId).skin->binding == oldBinding); // weight is pose, not binding
  ctx.tick(3.0);
  REQUIRE(ctx.writeField(joint.get(), "skinCoordWeight", std::any(std::vector<float>{1.0f})) ==
          FieldWriteResult::Ok);
  auto bindingChange = extractor.delta();
  CHECK(bindingChange.updatedSkinPose == std::vector<RenderItemId>{skinId});
  CHECK(extractor.item(skinId).skin->binding != oldBinding);
}

TEST_CASE("HAnim skin accepts direct geometry") {
  auto coord = createX3DNode("Coordinate");
  set(coord, "point", std::vector<SFVec3f>{{0,0,0}, {1,0,0}, {0,1,0}});
  auto geom = createX3DNode("TriangleSet");
  set(geom, "coord", std::shared_ptr<X3DNode>(coord));
  auto humanoid = createX3DNode("HAnimHumanoid");
  set(humanoid, "skinCoord", std::shared_ptr<X3DNode>(coord));
  set(humanoid, "skin", std::vector<std::shared_ptr<X3DNode>>{geom});
  Scene scene;
  scene.addRootNode(humanoid);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor extractor(ctx, scene);
  auto snapshot = extractor.fullSnapshot();
  REQUIRE(snapshot.added.size() == 1);
  CHECK(extractor.item(snapshot.added.front()).skin->sourceCoordIndex ==
        std::vector<std::uint32_t>{0,1,2});
}

TEST_CASE("Segment-shared geometry keeps per-Segment displacement in snapshot and delta") {
  auto coord = createX3DNode("Coordinate");
  set(coord, "point", std::vector<SFVec3f>{{0,0,0}, {1,0,0}, {0,1,0}, {1,1,0}});
  auto sharedShape = shapeWith(coord);
  auto makeSegment = [&](float displacement, const char *name) {
    auto segment = createX3DNode("HAnimSegment");
    auto displacer = createX3DNode("HAnimDisplacer");
    set(segment, "name", std::string(name));
    set(displacer, "name", std::string(name) + "_action");
    set(displacer, "coordIndex", std::vector<int>{0});
    set(displacer, "displacements", std::vector<SFVec3f>{{0,0,displacement}});
    set(displacer, "weight", 1.0f);
    set(segment, "coord", std::shared_ptr<X3DNode>(coord));
    set(segment, "displacers", std::vector<std::shared_ptr<X3DNode>>{displacer});
    set(segment, "children", std::vector<std::shared_ptr<X3DNode>>{sharedShape});
    return std::pair{segment,displacer};
  };
  auto [firstSegment, firstDisplacer] = makeSegment(1, "first");
  auto [secondSegment, secondDisplacer] = makeSegment(2, "second");
  auto joint = createX3DNode("HAnimJoint");
  set(joint, "name", std::string("humanoid_root"));
  set(joint, "children", std::vector<std::shared_ptr<X3DNode>>{firstSegment,secondSegment});
  auto humanoid = createX3DNode("HAnimHumanoid");
  set(humanoid, "name", std::string("audit_humanoid"));
  set(humanoid, "skeleton", std::vector<std::shared_ptr<X3DNode>>{joint});
  Scene scene; scene.addRootNode(humanoid);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene); ctx.buildFrom(scene);
  SceneExtractor extractor(ctx,scene);
  auto initial = extractor.fullSnapshot();
  REQUIRE(initial.added.size()==2);
  CHECK(extractor.item(initial.added[0]).mesh->positions[0].z==doctest::Approx(1));
  CHECK(extractor.item(initial.added[1]).mesh->positions[0].z==doctest::Approx(2));
  ctx.tick(1);
  REQUIRE(ctx.writeField(secondDisplacer.get(), "weight", std::any(3.0f))==FieldWriteResult::Ok);
  auto changed=extractor.delta();
  REQUIRE(changed.updatedGeometry.size()==1);
  auto deltaFirst=extractor.item(initial.added[0]).mesh->positions[0].z;
  auto deltaSecond=extractor.item(initial.added[1]).mesh->positions[0].z;
  extractor.fullSnapshot();
  CHECK(deltaFirst==doctest::Approx(extractor.item(initial.added[0]).mesh->positions[0].z));
  CHECK(deltaSecond==doctest::Approx(extractor.item(initial.added[1]).mesh->positions[0].z));
}

TEST_CASE("HAnimSite transforms its child geometry") {
  auto coord = createX3DNode("Coordinate");
  set(coord, "point", std::vector<SFVec3f>{{0,0,0}, {1,0,0}, {0,1,0}});
  auto geom = createX3DNode("TriangleSet");
  set(geom, "coord", std::shared_ptr<X3DNode>(coord));
  auto shape = createX3DNode("Shape");
  set(shape, "geometry", std::shared_ptr<X3DNode>(geom));
  auto site = createX3DNode("HAnimSite");
  set(site, "name", std::string("marker_pt"));
  set(site, "translation", SFVec3f{2,3,4});
  set(site, "children", std::vector<std::shared_ptr<X3DNode>>{shape});
  auto humanoid = createX3DNode("HAnimHumanoid");
  set(humanoid, "name", std::string("figure"));
  set(humanoid, "skeleton", std::vector<std::shared_ptr<X3DNode>>{site});
  Scene scene;
  scene.addRootNode(humanoid);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor extractor(ctx, scene);
  auto snapshot = extractor.fullSnapshot();
  REQUIRE(snapshot.added.size() == 1);
  const auto origin = extractor.item(snapshot.added[0]).worldTransform.transformPoint({0,0,0});
  CHECK(origin.x == doctest::Approx(2));
  CHECK(origin.y == doctest::Approx(3));
  CHECK(origin.z == doctest::Approx(4));
}

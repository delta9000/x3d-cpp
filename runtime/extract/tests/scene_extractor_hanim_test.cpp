#include "SceneExtractor.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"

#include <any>
#include <cmath>
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
  CHECK(item.mesh->sourceCoordIndex == std::vector<std::uint32_t>{0,1,2,0,2,3});
  CHECK(item.mesh->sourceCoordIndex.size() == item.mesh->positions.size());
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
  CHECK(extractor.item(snapshot.added.front()).mesh->sourceCoordIndex ==
        std::vector<std::uint32_t>{0,1,2});
}

TEST_CASE("HAnim skin normalIndex follows each corner's coordinate influences") {
  auto coord = createX3DNode("Coordinate");
  set(coord, "point", std::vector<SFVec3f>{{-1,-1,0}, {1,-1,0}, {1,1,0}, {-1,1,0}});
  auto normal = createX3DNode("Normal");
  set(normal, "vector", std::vector<SFVec3f>(4, {0,0,1}));
  auto geom = createX3DNode("IndexedFaceSet");
  set(geom, "coord", std::shared_ptr<X3DNode>(coord));
  set(geom, "normal", std::shared_ptr<X3DNode>(normal));
  set(geom, "coordIndex", std::vector<int>{0,1,2,-1,0,2,3,-1});
  set(geom, "normalIndex", std::vector<int>{3,0,1,-1,3,1,2,-1});
  auto shape = createX3DNode("Shape");
  set(shape, "geometry", std::shared_ptr<X3DNode>(geom));
  auto joint = createX3DNode("HAnimJoint");
  set(joint, "skinCoordIndex", std::vector<int>{0});
  set(joint, "skinCoordWeight", std::vector<float>{1});
  set(joint, "rotation", SFRotation{0,1,0,1});
  auto humanoid = createX3DNode("HAnimHumanoid");
  set(humanoid, "skinCoord", std::shared_ptr<X3DNode>(coord));
  set(humanoid, "skinNormal", std::shared_ptr<X3DNode>(normal));
  set(humanoid, "skin", std::vector<std::shared_ptr<X3DNode>>{shape});
  set(humanoid, "skeleton", std::vector<std::shared_ptr<X3DNode>>{joint});
  set(humanoid, "joints", std::vector<std::shared_ptr<X3DNode>>{joint});
  Scene scene; scene.addRootNode(humanoid);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor extractor(ctx, scene);
  auto snapshot = extractor.fullSnapshot();
  REQUIRE(snapshot.added.size() == 1);
  const auto id = snapshot.added[0];
  CHECK(extractor.item(id).mesh->sourceNormalIndex ==
        std::vector<std::uint32_t>{3,0,1,3,1,2});
  const auto mesh = extractor.deformedMesh(id);
  CHECK(mesh.normals[0].x == doctest::Approx(std::sin(1.0f)));
  CHECK(mesh.normals[3].x == doctest::Approx(std::sin(1.0f)));
  CHECK(mesh.normals[1].x == doctest::Approx(0));
  CHECK(mesh.normals[2].x == doctest::Approx(0));
  CHECK(mesh.normals[5].x == doctest::Approx(0));
}

TEST_CASE("Segment-shared geometry keeps per-Segment displacement in snapshot and delta") {
  auto coord = createX3DNode("Coordinate");
  set(coord, "point", std::vector<SFVec3f>{{0,0,0}, {1,0,0}, {0,1,0}, {1,1,0}});
  auto sharedShape = shapeWith(coord);
  SUBCASE("raw Segment meshes") {}
  SUBCASE("identical texture bake preserves each Segment deformation") {
    auto appearance = createX3DNode("Appearance");
    set(appearance, "textureTransform", createX3DNode("TextureTransform"));
    set(sharedShape, "appearance", appearance);
  }
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
  CHECK(extractor.item(initial.added[0]).geometry != extractor.item(initial.added[1]).geometry);
  const auto firstIdentity = extractor.item(initial.added[0]).geometry;
  const auto secondIdentity = extractor.item(initial.added[1]).geometry;
  auto unchangedMesh = extractor.item(initial.added[0]).mesh;
  ctx.tick(1);
  REQUIRE(ctx.writeField(secondDisplacer.get(), "weight", std::any(3.0f))==FieldWriteResult::Ok);
  auto changed=extractor.delta();
  REQUIRE(changed.updatedGeometry.size()==1);
  CHECK(extractor.item(initial.added[0]).mesh == unchangedMesh);
  CHECK(extractor.item(initial.added[0]).geometry == firstIdentity);
  CHECK(extractor.item(initial.added[1]).geometry != secondIdentity);
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

TEST_CASE("delta oracle: compiled skin inputs match a fresh extractor after edits") {
  int edit = 0;
  SUBCASE("joint displacement payload") {}
  SUBCASE("joint displacement indices") { edit = 1; }
  SUBCASE("joint bind center") { edit = 2; }
  SUBCASE("joint bind scale orientation") { edit = 3; }
  auto coord = createX3DNode("Coordinate");
  set(coord, "point", MFVec3f{{0,0,0},{1,0,0},{1,1,0},{0,1,0}});
  auto joint = createX3DNode("HAnimJoint"), displacer = createX3DNode("HAnimDisplacer");
  set(joint, "skinCoordIndex", MFInt32{0,1,2,3});
  set(joint, "skinCoordWeight", MFFloat{1,1,1,1});
  set(displacer, "coordIndex", MFInt32{0});
  set(displacer, "displacements", MFVec3f{{0,0,1}}); set(displacer, "weight", 1.0f);
  set(joint, "displacers", MFNode{displacer});
  auto h = createX3DNode("HAnimHumanoid");
  set(h, "skinCoord", coord); set(h, "skin", MFNode{shapeWith(coord)});
  set(h, "skeleton", MFNode{joint}); set(h, "joints", MFNode{joint});
  if (edit == 2) {
    set(joint, "rotation", SFRotation{0,0,1,1.57079632679f});
    set(h, "jointBindingRotations", MFRotation{{0,0,1,1.57079632679f}});
  }
  if (edit == 3) {
    set(joint, "scale", SFVec3f{2,1,1});
    set(h, "jointBindingScales", MFVec3f{{2,1,1}});
  }
  Scene scene; scene.rootNodes = {h};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene); REQUIRE(ex.fullSnapshot().added.size() == 1);
  if (edit == 0) ctx.postEvent(displacer.get(), "displacements", MFVec3f{{0,0,2}});
  if (edit == 1) ctx.postEvent(displacer.get(), "coordIndex", MFInt32{1});
  if (edit == 2) ctx.postEvent(joint.get(), "center", SFVec3f{1,0,0});
  if (edit == 3) ctx.postEvent(joint.get(), "scaleOrientation", SFRotation{0,0,1,0.78539816339f});
  ctx.tick(1); auto d = ex.delta();
  CHECK((!d.updatedSkinPose.empty() || !d.added.empty()));
  SceneExtractor oracle(ctx, scene); REQUIRE(oracle.fullSnapshot().added.size() == 1);
  auto actual = ex.deformedMesh(0), expected = oracle.deformedMesh(0);
  REQUIRE(actual.positions.size() == expected.positions.size());
  for (std::size_t i = 0; i < actual.positions.size(); ++i) {
    CHECK(std::fabs(actual.positions[i].x - expected.positions[i].x) < 1e-4f);
    CHECK(std::fabs(actual.positions[i].y - expected.positions[i].y) < 1e-4f);
    CHECK(std::fabs(actual.positions[i].z - expected.positions[i].z) < 1e-4f);
  }
}

TEST_CASE("delta oracle: skeletal Site motion publishes the dependent skin pose") {
  auto coord = createX3DNode("Coordinate");
  set(coord, "point", MFVec3f{{0,0,0},{1,0,0},{1,1,0},{0,1,0}});
  auto root = createX3DNode("HAnimJoint"), joint = createX3DNode("HAnimJoint");
  auto site = createX3DNode("HAnimSite");
  set(joint, "skinCoordIndex", MFInt32{0,1,2,3});
  set(joint, "skinCoordWeight", MFFloat{1,1,1,1});
  set(root, "children", MFNode{site}); set(site, "children", MFNode{joint});
  auto h = createX3DNode("HAnimHumanoid");
  set(h, "skinCoord", coord); set(h, "skin", MFNode{shapeWith(coord)});
  set(h, "skeleton", MFNode{root}); set(h, "joints", MFNode{root,joint});
  Scene scene; scene.rootNodes = {h};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene); REQUIRE(ex.fullSnapshot().added.size() == 1);
  auto mirror = ex.deformedMesh(0);
  ctx.postEvent(site.get(), "translation", SFVec3f{0,3,0}); ctx.tick(1);
  auto d = ex.delta();
  CHECK((!d.updatedSkinPose.empty() || !d.added.empty()));
  if (!d.updatedSkinPose.empty() || !d.added.empty()) mirror = ex.deformedMesh(0);
  SceneExtractor oracle(ctx, scene);
  REQUIRE(oracle.fullSnapshot().added.size() == 1);
  CHECK(mirror.positions == oracle.deformedMesh(0).positions);
  CHECK(mirror.positions.front().y == doctest::Approx(3));
}

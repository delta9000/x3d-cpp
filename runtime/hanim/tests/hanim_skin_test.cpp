#include "HAnimSkin.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/CoordinateDouble.hpp"
#include "x3d/nodes/Normal.hpp"
#include "x3d/nodes/HAnimHumanoid.hpp"
#include "x3d/nodes/HAnimJoint.hpp"
#include "x3d/nodes/HAnimSegment.hpp"
#include "x3d/nodes/HAnimDisplacer.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "parse/X3DParse.hpp"
#include "doctest/doctest.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <numbers>
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime::hanim;
static bool near(float a,float b) { return std::fabs(a-b)<1e-4f; }
static void xy(SFVec3f p,float x,float y) { CHECK(near(p.x,x)); CHECK(near(p.y,y)); }
static std::shared_ptr<HAnimHumanoid> humanoid(std::shared_ptr<Coordinate> c,
    const std::vector<std::shared_ptr<HAnimJoint>> &joints) {
  auto h=std::make_shared<HAnimHumanoid>();
  h->setSkinCoord(c);
  MFNode list; for (auto &j:joints) list.push_back(j);
  h->setJoints(list);
  if (!joints.empty()) h->setSkeleton({joints[0]});
  return h;
}
TEST_CASE("two joint arm, skeleton pose and unweighted vertex") {
  auto c=std::make_shared<Coordinate>();
  c->setPoint({{0,0,0},{2,0,0},{3,0,0}});
  auto shoulder=std::make_shared<HAnimJoint>(), elbow=std::make_shared<HAnimJoint>();
  shoulder->setChildren({elbow});
  elbow->setTranslation({2,0,0});
  elbow->setSkinCoordIndex({2}); elbow->setSkinCoordWeight({1});
  auto h=humanoid(c,{shoulder,elbow});
  h->setTranslation({100,0,0}); // excluded from the humanoid-local pose
  auto b=compileBinding(*h);
  CHECK(b.joints.size()==2);
  elbow->setRotation({0,0,1,std::numbers::pi_v<float>/2});
  auto pose=evaluatePose(b);
  std::vector<SFVec3f> p; deform(b,pose,p,nullptr);
  xy(p[0],0,0); xy(p[1],2,0); xy(p[2],2,3);
}
TEST_CASE("weight normalization, unbounded influences and inverse transpose normals") {
  auto c=std::make_shared<Coordinate>(); c->setPoint({{1,0,0}});
  auto normal=std::make_shared<Normal>(); normal->setVector({{1,1,0}});
  std::vector<std::shared_ptr<HAnimJoint>> joints;
  for (int i=0;i<5;++i) {
    auto j=std::make_shared<HAnimJoint>();
    j->setSkinCoordIndex({0}); j->setSkinCoordWeight({2});
    j->setTranslation({float(i),0,0});
    joints.push_back(j);
  }
  auto h=humanoid(c,joints); h->setSkinNormal(normal);
  h->setSkeleton({joints[0],joints[1],joints[2],joints[3],joints[4]});
  auto b=compileBinding(*h);
  CHECK(b.influences.size()==5);
  for (const auto &in:b.influences) CHECK(near(in.weight,.2f));
  auto pose=evaluatePose(b);
  std::vector<SFVec3f> p,n; deform(b,pose,p,&n);
  xy(p[0],3,0);
  joints[0]->setScale({2,1,1});
  pose=evaluatePose(b); deform(b,pose,p,&n);
  // Blend of four identity normal maps and one inverse-scale map.
  CHECK(near(n[0].x, .9f/std::sqrt(.9f*.9f+1)));
  CHECK(near(n[0].y, 1/std::sqrt(.9f*.9f+1)));
}
TEST_CASE("v2 bind fields use joint-list positions and single-value rule") {
  auto c=std::make_shared<Coordinate>(); c->setPoint({{9,0,0}});
  auto bind=std::make_shared<CoordinateDouble>(); bind->setPoint({{2,0,0}});
  auto currentNormal=std::make_shared<Normal>(); currentNormal->setVector({{0,1,0}});
  auto bindNormal=std::make_shared<Normal>(); bindNormal->setVector({{1,0,0}});
  auto a=std::make_shared<HAnimJoint>(), bJoint=std::make_shared<HAnimJoint>();
  bJoint->setSkinCoordIndex({0}); bJoint->setSkinCoordWeight({1});
  bJoint->setTranslation({4,0,0});
  auto h=humanoid(c,{a,bJoint}); h->setSkeleton({a,bJoint});
  h->setSkinBindingCoords(bind);
  h->setSkinNormal(currentNormal); h->setSkinBindingNormals(bindNormal);
  h->setJointBindingPositions({{1,0,0},{4,0,0}});
  auto b=compileBinding(*h);
  std::vector<SFVec3f> p,n; deform(b,evaluatePose(b),p,&n);
  xy(p[0],2,0);
  xy(n[0],1,0);
  h->setJointBindingPositions({{4,0,0}});
  bJoint->setTranslation({5,0,0});
  b=compileBinding(*h); deform(b,evaluatePose(b),p,nullptr);
  xy(p[0],3,0); // the single authored value also applies to the second joint
  bJoint->setTranslation({4,0,0});
  bJoint->setRotation({0,0,1,std::numbers::pi_v<float>/2});
  bJoint->setScale({2,1,1});
  h->setJointBindingRotations({{0,0,1,std::numbers::pi_v<float>/2}});
  h->setJointBindingScales({{2,1,1}});
  b=compileBinding(*h); deform(b,evaluatePose(b),p,nullptr);
  xy(p[0],2,0);
}
TEST_CASE("CoordinateDouble is accepted as a legacy skinCoord source") {
  auto h=std::make_shared<HAnimHumanoid>();
  auto c=std::make_shared<CoordinateDouble>(); c->setPoint({{1.25,2.5,3.75}});
  h->setSkinCoord(c);
  auto b=compileBinding(*h);
  CHECK(b.bindPositions.size()==1);
  CHECK(near(b.bindPositions[0].x,1.25f));
  CHECK(b.influenceOffset.size()==2);
  CHECK(b.influenceOffset[1]==0);
}
TEST_CASE("joint and segment displacers read live weights") {
  auto c=std::make_shared<Coordinate>(); c->setPoint({{1,0,0}});
  auto j=std::make_shared<HAnimJoint>();
  j->setSkinCoordIndex({0}); j->setSkinCoordWeight({1});
  j->setRotation({0,0,1,std::numbers::pi_v<float>/2});
  auto d=std::make_shared<HAnimDisplacer>();
  d->setCoordIndex({0}); d->setDisplacements({{1,0,0}}); d->setWeight(2);
  j->setDisplacers({d});
  auto h=humanoid(c,{j});
  auto b=compileBinding(*h);
  std::vector<SFVec3f> p; deform(b,evaluatePose(b),p,nullptr); xy(p[0],0,3);
  d->setWeight(-1); deform(b,evaluatePose(b),p,nullptr); xy(p[0],0,0);
  auto segment=std::make_shared<HAnimSegment>();
  auto shape=std::make_shared<Shape>();
  auto reusedShape=std::make_shared<Shape>();
  auto geo=std::make_shared<IndexedFaceSet>();
  geo->setCoord(c); shape->setGeometry(geo); reusedShape->setGeometry(geo);
  segment->setChildren({shape,reusedShape}); // USE of the same geometry/Coordinate
  segment->setDisplacers({d}); h->setSegments({segment});
  compileBinding(*h);
  p=c->getPoint();
  CHECK(displaceSegmentPoints(*c,p)); xy(p[0],0,0);
}
TEST_CASE("bad input diagnostics and weighted skeleton-only joints") {
  auto c=std::make_shared<Coordinate>(); c->setPoint({{1,0,0}});
  auto a=std::make_shared<HAnimJoint>(), extra=std::make_shared<HAnimJoint>();
  a->setChildren({extra});
  extra->setSkinCoordIndex({0,4}); extra->setSkinCoordWeight({-1,1});
  auto h=humanoid(c,{a,a});
  auto b=compileBinding(*h);
  CHECK(b.joints.size()==2);
  CHECK(b.influences.empty());
  CHECK(b.diagnostics.size()>=2);
  extra->setSkinCoordIndex({0,1,2});
  b=compileBinding(*h);
  CHECK(b.diagnostics.size()>=2);
}

TEST_CASE("optional archive skin smoke and timing") {
  const char *root=std::getenv("X3D_ARCHIVE_DIR");
  if (!root) { MESSAGE("X3D_ARCHIVE_DIR unset; archive smoke skipped"); return; }
  namespace fs=std::filesystem;
  const auto base=fs::path(root)/"HumanoidAnimation";
  struct Fixture { const char *file; size_t influences; bool rest; };
  for (const auto &fixture : {
         Fixture{"Skin/BoxMan2.x3d",240,true},
         Fixture{"WinterAndSpring/Leif.x3d",86492,true},
         Fixture{"WinterAndSpring/GrampsAnimations.x3d",321453,false}}) {
    auto doc=x3d::codec::parseFile((base/fixture.file).string());
    std::shared_ptr<X3DNode> found;
    std::unordered_set<const X3DNode *> seen;
    auto search=[&](auto &&self,const std::shared_ptr<X3DNode> &n)->void {
      if (!n || found || !seen.insert(n.get()).second) return;
      if (dynamic_cast<HAnimHumanoid *>(n.get())) { found=n; return; }
      for (const auto &f:n->fields()) {
        if (!f.get) continue;
        if (f.type==X3DFieldType::SFNode)
          self(self,std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n)));
        else if (f.type==X3DFieldType::MFNode)
          for (const auto &c:std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n))) self(self,c);
      }
    };
    for (const auto &n:doc.scene.rootNodes) search(search,n);
    REQUIRE(found);
    auto t0=std::chrono::steady_clock::now();
    auto b=compileBinding(*found);
    auto t1=std::chrono::steady_clock::now();
    CHECK(b.influences.size()==fixture.influences);
    auto pose=evaluatePose(b);
    std::vector<SFVec3f> p;
    deform(b,pose,p,nullptr);
    auto t2=std::chrono::steady_clock::now();
    if (fixture.rest) {
      REQUIRE(p.size()==b.bindPositions.size());
      for (size_t i=0;i<p.size();++i) {
        CHECK(near(p[i].x,b.bindPositions[i].x));
        CHECK(near(p[i].y,b.bindPositions[i].y));
        CHECK(near(p[i].z,b.bindPositions[i].z));
      }
    }
    std::cout << fixture.file << " compile="
      << std::chrono::duration<double,std::milli>(t1-t0).count() << "ms pose+deform="
      << std::chrono::duration<double,std::milli>(t2-t1).count() << "ms\n";
  }
}

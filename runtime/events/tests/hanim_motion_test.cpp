#include "doctest/doctest.h"
#include "X3DSceneBridge.hpp"
#include "X3DParse.hpp"
#include "x3d/nodes/HAnimHumanoid.hpp"
#include "x3d/nodes/HAnimJoint.hpp"
#include "x3d/nodes/HAnimMotion.hpp"
#include "x3d/nodes/Transform.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {
struct Fixture {
  Scene scene;
  X3DExecutionContext ctx;
  std::shared_ptr<HAnimHumanoid> human = std::make_shared<HAnimHumanoid>();
  std::shared_ptr<HAnimJoint> root = std::make_shared<HAnimJoint>();
  std::shared_ptr<HAnimJoint> arm = std::make_shared<HAnimJoint>();
  std::shared_ptr<HAnimMotion> motion = std::make_shared<HAnimMotion>();
  std::shared_ptr<Transform> mirror = std::make_shared<Transform>();
  Fixture() {
    root->setName("root"); arm->setName("arm");
    human->setJoints(MFNode{root, arm});
    human->setSkeleton(MFNode{root, arm});
    human->setMotions(MFNode{motion});
    motion->setJoints("root, arm");
    motion->setChannels("4 Xposition, Yposition, Zrotation Xrotation, 2 Yposition Zrotation");
    motion->setValues(MFFloat{
      1, 2, 90, 90, 5, 45,
      3, 4, 0, 0, 6, 90,
      7, 8, 0, 0, 9, 180});
    motion->setFrameDuration(0.1);
    scene.addRootNode(human);
    scene.addRootNode(mirror);
    ctx.buildSceneGraph(scene);
    ctx.addRoute({root.get(), "translation"}, {mirror.get(), "translation"});
    attachStandardRuntime(scene, ctx);
  }
  void tick(double t) { ctx.tick(t); }
};
SFVec3f rotate(SFRotation r, SFVec3f v) {
  const float c = std::cos(r.angle), s = std::sin(r.angle);
  const SFVec3f a{r.x, r.y, r.z};
  const float dot = a.x*v.x+a.y*v.y+a.z*v.z;
  return {v.x*c+(a.y*v.z-a.z*v.y)*s+a.x*dot*(1-c),
          v.y*c+(a.z*v.x-a.x*v.z)*s+a.y*dot*(1-c),
          v.z*c+(a.x*v.y-a.y*v.x)*s+a.z*dot*(1-c)};
}
}

TEST_CASE("HAnimMotion frames, ordered Euler degrees and context events") {
  Fixture f;
  int routed = 0;
  f.ctx.addFieldWriteListener([&](const FieldAddress &a) {
    if (a.node == f.root.get() && a.field == "rotation") ++routed;
  });
  f.tick(1.0);
  CHECK(f.motion->getFrameCount() == 3);
  CHECK(f.motion->getCycleTime() == doctest::Approx(1.0));
  CHECK(f.motion->getElapsedTime() == doctest::Approx(0.0));
  CHECK(f.root->getTranslation() == (SFVec3f{1, 2, 0}));
  CHECK(f.mirror->getTranslation() == (SFVec3f{1, 2, 0}));
  CHECK(f.arm->getTranslation() == (SFVec3f{0, 5, 0}));
  const auto v = rotate(f.root->getRotation(), {0, 1, 0});
  CHECK(v.x == doctest::Approx(0).epsilon(0.001));
  CHECK(v.y == doctest::Approx(0).epsilon(0.001));
  CHECK(v.z == doctest::Approx(1).epsilon(0.001));
  CHECK(routed == 1);
  f.tick(1.1);
  CHECK(f.motion->getFrameIndex() == 1);
  CHECK(f.root->getTranslation() == (SFVec3f{3, 4, 0}));
  CHECK(f.motion->getElapsedTime() == doctest::Approx(0.1));
  f.tick(1.2);
  CHECK(f.motion->getFrameIndex() == 2);
  CHECK(f.arm->getTranslation() == (SFVec3f{0, 9, 0}));
  f.tick(1.3);
  CHECK(f.motion->getFrameIndex() == 2);
  f.tick(1.4);
  CHECK(f.motion->getFrameIndex() == 2);
}

TEST_CASE("HAnimMotion pause, reverse, loops and manual steps") {
  Fixture f;
  f.motion->setFrameIncrement(0);
  f.tick(0);
  f.tick(1);
  CHECK(f.motion->getFrameIndex() == 0);
  CHECK(f.motion->getElapsedTime() == doctest::Approx(0));
  f.ctx.postEvent(f.motion.get(), "next", std::any(SFBool{true})); f.ctx.process();
  CHECK(f.motion->getFrameIndex() == 1);
  CHECK(f.root->getTranslation().x == doctest::Approx(3));
  f.ctx.postEvent(f.motion.get(), "previous", std::any(SFBool{true})); f.ctx.process();
  CHECK(f.motion->getFrameIndex() == 0);
  f.ctx.postEvent(f.motion.get(), "previous", std::any(SFBool{true})); f.ctx.process();
  CHECK(f.motion->getFrameIndex() == 2);
  f.ctx.postEvent(f.motion.get(), "next", std::any(SFBool{false})); f.ctx.process();
  CHECK(f.motion->getFrameIndex() == 2);
  f.motion->setFrameIncrement(-1);
  f.motion->setLoop(true);
  f.tick(1.1);
  CHECK(f.motion->getFrameIndex() == 1);
  f.tick(1.2);
  CHECK(f.motion->getFrameIndex() == 0);
  f.tick(1.3);
  CHECK(f.motion->getFrameIndex() == 2);
  CHECK(f.motion->getCycleTime() == doctest::Approx(1.3));
}

TEST_CASE("HAnimMotion gating, IGNORED, enabled channels and range") {
  Fixture f;
  f.motion->setJoints("root, IGNORED, arm");
  f.motion->setChannels("1 Xposition, 1 Xposition, 1 Yposition");
  f.motion->setValues(MFFloat{1, 100, 4, 2, 200, 5, 3, 300, 6});
  f.motion->setChannelsEnabled(MFBool{false, true, true});
  f.human->setMotionsEnabled(MFBool{false});
  f.tick(0);
  CHECK(f.motion->getFrameCount() == 3);
  CHECK(f.root->getTranslation().x == 0);
  f.human->setMotionsEnabled(MFBool{});
  f.tick(0.1);
  CHECK(f.root->getTranslation().x == 0);
  CHECK(f.arm->getTranslation().y == 4);
  f.motion->setEnabled(false);
  f.tick(0.2);
  CHECK(f.arm->getTranslation().y == 4);
  f.motion->setEnabled(true);
  f.motion->setStartFrame(1);
  f.motion->setEndFrame(2);
  f.ctx.postEvent(f.motion.get(), "frameIndex", std::any(SFInt32{20})); f.ctx.process();
  f.tick(0.3);
  CHECK(f.motion->getFrameIndex() == 2);
  CHECK(f.arm->getTranslation().y == 6);
  f.motion->setFrameIncrement(0);
  f.ctx.postEvent(f.motion.get(), "next", std::any(SFBool{true})); f.ctx.process();
  CHECK(f.motion->getFrameIndex() == 1);
  CHECK(f.arm->getTranslation().y == 5);
  f.motion->setEnabled(false);
  f.ctx.postEvent(f.motion.get(), "frameIndex", std::any(SFInt32{20})); f.ctx.process();
  f.tick(0.4);
  CHECK(f.motion->getFrameIndex() == 2);
  CHECK(f.arm->getTranslation().y == 5);
}

TEST_CASE("HAnimMotion Korean archive smoke when X3D_ARCHIVE_DIR is set") {
  const char *dir = std::getenv("X3D_ARCHIVE_DIR");
  if (!dir) return;
  const auto file = std::filesystem::path(dir) / "HumanoidAnimation" / "Specifications" /
      "KoreanCharacterMotionAnnexD01Jin.x3d";
  std::ifstream stream(file);
  REQUIRE(stream.good());
  std::ostringstream text;
  text << stream.rdbuf();
  auto doc = x3d::codec::parseDocument(text.str());
  Scene &scene = doc.getScene();
  HAnimMotion *motion = nullptr;
  HAnimJoint *hip = nullptr, *knee = nullptr;
  detail::forEachNode(scene, [&](X3DNode *n) {
    if (auto *m = dynamic_cast<HAnimMotion *>(n)) motion = m;
    if (auto *j = dynamic_cast<HAnimJoint *>(n)) {
      if (j->getName() == "l_hip") hip = j;
      if (j->getName() == "l_knee") knee = j;
    }
  });
  REQUIRE(motion);
  REQUIRE(hip);
  REQUIRE(knee);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  attachStandardRuntime(scene, ctx);
  ctx.tick(0);
  CHECK(motion->getFrameCount() == 392);
  const auto hip0 = hip->getRotation();
  const auto knee0 = knee->getRotation();
  for (int i = 1; i <= 10; ++i) ctx.tick(i * motion->getFrameDuration());
  CHECK(hip->getRotation() != hip0);
  CHECK(knee->getRotation() != knee0);
}

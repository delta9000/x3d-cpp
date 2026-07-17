#include "doctest/doctest.h"
// loadsensor_test.cpp — LoadSensorSystem behavior (§9.4.3 Networking). A
// time-driven System observes each LoadSensor's watched X3DUrlObject children
// per tick through the AssetResolver seam and emits isActive/isLoaded/loadTime/
// progress. Tests inject scripted resolvers so no I/O happens here.
#include "LoadSensorSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/ImageTexture.hpp"
#include "x3d/nodes/LoadSensor.hpp"

#include <memory>

using namespace x3d::core;
using namespace x3d::runtime;

namespace {

// Shared harness: one LoadSensor watching one ImageTexture, wired onto a fresh
// context with an injected resolver. Mirrors standard_runtime_test's scene
// setup (addRootNode → buildSceneGraph) so postEvent's emit-thunks are live.
struct Rig {
  Scene scene;
  X3DExecutionContext ctx;
  std::shared_ptr<x3d::nodes::LoadSensor> ls;
  std::shared_ptr<x3d::nodes::ImageTexture> tex;
  std::shared_ptr<LoadSensorSystem> sys;
  explicit Rig(extract::AssetResolver r) {
    ls = std::make_shared<x3d::nodes::LoadSensor>();
    tex = std::make_shared<x3d::nodes::ImageTexture>();
    tex->setUrl(MFString{"a.png"});
    ls->setChildren(MFNode{tex});
    scene.addRootNode(ls);
    ctx.buildSceneGraph(scene);
    sys = std::make_shared<LoadSensorSystem>(std::move(r));
    sys->setScene(&scene);
    sys->attach(ls.get(), ctx);
    ctx.addSystem(sys);
  }
};

} // namespace

TEST_CASE("LoadSensor: pending child activates the sensor") {
  Rig rig([](const std::string &, extract::AssetKind) {
    return extract::AssetResult::makePending();
  });
  rig.ctx.tick(1.0);
  CHECK(rig.ls->getIsActive());
  CHECK_FALSE(rig.ls->getIsLoaded());
  CHECK(rig.ls->getProgress() == 0.0f);
}

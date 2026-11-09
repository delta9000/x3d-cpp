#include "doctest/doctest.h"
#include "x3d/sdk.hpp"
#include "ViewDependentSystem.hpp"
#include "ViewpointBindSystem.hpp"

#include <algorithm>
#include <any>
#include <cmath>
#include <memory>

namespace {
namespace sdk = x3d::sdk;
using namespace x3d::core;
using namespace x3d::runtime;

void checkMatrix(const sdk::Mat4 &actual, const sdk::Mat4 &expected) {
  for (std::size_t i = 0; i < actual.m.size(); ++i)
    CHECK(std::fabs(actual.m[i] - expected.m[i]) < 0.001f);
}

template <class T> T field(const sdk::X3DNode &node, const char *name) {
  for (const auto &f : node.fields())
    if (f.x3dName == name && f.get) return std::any_cast<T>(f.get(node));
  FAIL("missing field");
  return T{};
}

TEST_CASE("viewer_world_pose_lifetime_and_authored_query") {
  auto doc = sdk::parseDocument(R"(<X3D><Scene>
    <Transform translation='3 4 5' rotation='0 1 0 0.7'>
      <Viewpoint position='1 2 3' orientation='1 0 0 0.2'/>
    </Transform>
    <OrthoViewpoint position='0 0 100' jump='false'/>
  </Scene></X3D>)", sdk::Encoding::XML);
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  attachViewpointBind(ctx);
  ctx.tick(0);
  const sdk::Mat4 authored = sdk::Mat4::translation({3, 4, 5}) *
      sdk::Mat4::rotation({0, 1, 0, 0.7f}) * sdk::Mat4::translation({1, 2, 3}) *
      sdk::Mat4::rotation({1, 0, 0, 0.2f});
  checkMatrix(ctx.boundViewpointWorldPose(), authored);
  ctx.setViewpointOffset(ctx.boundViewpoint(), {sdk::Mat4::translation({0, 0, -2})});
  ctx.setHeadPose({0, 1, 0}, {0, 0, 1, 0.3f});
  ctx.tick(1);
  const sdk::Mat4 relative = ctx.viewMatrix();
  const sdk::Mat4 absolute = sdk::Mat4::translation({-7, 8, 9}) *
                            sdk::Mat4::rotation({0, 0, 1, 0.8f});
  ctx.setViewerWorldPose(absolute);
  checkMatrix(ctx.viewMatrix(), absolute.inverse());
  checkMatrix(ctx.boundViewpointWorldPose(), authored);
  auto eye = ctx.cameraWorldPosition();
  CHECK(eye.x == doctest::Approx(-7));
  CHECK(eye.y == doctest::Approx(8));
  CHECK(eye.z == doctest::Approx(9));
  auto up = ctx.cameraWorldUp();
  CHECK(up.x == doctest::Approx(-std::sin(0.8f)));
  CHECK(up.y == doctest::Approx(std::cos(0.8f)));
  sdk::SceneExtractor extractor(ctx, doc.scene);
  checkMatrix(extractor.camera().viewMatrix, absolute.inverse());

  // jump=false must preserve the relative camera, not copy the host camera
  // into the new viewpoint's offset. The override survives the bind itself.
  auto *second = doc.scene.rootNodes[1].get();
  ctx.postEvent(second, "set_bind", std::any(SFBool{true}));
  ctx.tick(2);
  REQUIRE(ctx.boundViewpoint() == second);
  checkMatrix(ctx.viewMatrix(), absolute.inverse());
  checkMatrix(ctx.boundViewpointWorldPose(), sdk::Mat4::translation({0, 0, 100}));
  ctx.clearViewerWorldPose();
  checkMatrix(ctx.viewMatrix(), relative);

  ctx.setViewerWorldPose(absolute);
  ctx.setHeadPose({2, 3, 4}, {0, 1, 0, 0.4f});
  ctx.setViewpointOffset(second, {sdk::Mat4::translation({0, 0, -6})});
  checkMatrix(ctx.viewMatrix(), absolute.inverse());
  ctx.clearViewerWorldPose();
  checkMatrix(ctx.viewMatrix(), (sdk::Mat4::translation({0, 0, 100}) *
      sdk::Mat4::translation({0, 0, -6}) * sdk::Mat4::translation({2, 3, 4}) *
      sdk::Mat4::rotation({0, 1, 0, 0.4f})).inverse());
  ctx.clearViewerWorldPose(); // clearing twice is harmless

  sdk::X3DExecutionContext empty;
  checkMatrix(empty.boundViewpointWorldPose(), sdk::Mat4::identity());
  empty.setViewerWorldPose(absolute);
  checkMatrix(empty.viewMatrix(), absolute.inverse());
  empty.clearViewerWorldPose();
  checkMatrix(empty.viewMatrix(), sdk::Mat4::identity());
}

TEST_CASE("viewer_world_pose_preserves_relative_bind_transition") {
  auto doc = sdk::parseDocument(R"(<X3D><Scene>
    <NavigationInfo transitionTime='2' transitionType='"LINEAR"'/>
    <Viewpoint position='0 0 10'/>
    <Viewpoint position='0 0 110'/>
  </Scene></X3D>)", sdk::Encoding::XML);
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  attachViewpointBind(ctx);
  ctx.tick(0);
  const auto absolute = sdk::Mat4::translation({7, 8, 9});
  ctx.setViewerWorldPose(absolute);
  ctx.postEvent(doc.scene.rootNodes[2].get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(1);
  ctx.tick(2);
  checkMatrix(ctx.viewMatrix(), absolute.inverse());
  ctx.clearViewerWorldPose();
  CHECK(ctx.cameraWorldPosition().z == doctest::Approx(60));
  ctx.setViewerWorldPose(absolute);
  ctx.tick(3);
  checkMatrix(ctx.viewMatrix(), absolute.inverse());
  ctx.clearViewerWorldPose();
  CHECK(ctx.cameraWorldPosition().z == doctest::Approx(110));
}

TEST_CASE("viewer_world_pose_drives_view_dependent_systems") {
  auto doc = sdk::parseDocument(R"(<X3D><Scene>
    <Viewpoint position='0 0 100'/>
    <ProximitySensor size='4 4 4'/>
    <VisibilitySensor center='-5 0 0' size='1 1 1'/>
    <LOD range='5'><Shape><Box/></Shape><Shape><Sphere/></Shape></LOD>
    <Billboard><Shape><Box/></Shape></Billboard>
  </Scene></X3D>)", sdk::Encoding::XML);
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  auto system = std::make_shared<ViewDependentSystem>();
  for (const auto &node : doc.scene.rootNodes) system->attach(node.get(), ctx);
  ctx.addSystem(system);
  auto ps = doc.scene.rootNodes[1];
  auto vs = doc.scene.rootNodes[2];
  auto lod = doc.scene.rootNodes[3];
  int level = -1;
  system->setLevelChangedHook([&](sdk::X3DNode *node, int value) {
    if (node == lod.get()) level = value;
  });
  ctx.tick(0);
  CHECK_FALSE(field<SFBool>(*ps, "isActive"));
  CHECK(level == 1);
  // Eye inside the proximity box, facing -X; visibility box is ahead.
  ctx.setViewerWorldPose(sdk::Mat4::translation({1, 0, 0}) *
                        sdk::Mat4::rotation({0, 1, 0, 1.5707963268f}));
  ctx.tick(1);
  CHECK(field<SFBool>(*ps, "isActive"));
  CHECK(field<SFBool>(*vs, "isActive"));
  CHECK(level == 0);
  auto position = field<SFVec3f>(*ps, "position_changed");
  CHECK(position.x == doctest::Approx(1));
  checkMatrix(sdk::Mat4::rotation(field<SFRotation>(*ps, "orientation_changed")),
              sdk::Mat4::rotation({0, 1, 0, 1.5707963268f}));
  sdk::SceneExtractor extractor(ctx, doc.scene);
  auto snapshot = extractor.fullSnapshot();
  bool billboard = false;
  auto bb = doc.scene.rootNodes[4];
  auto children = field<MFNode>(*bb, "children");
  for (auto id : snapshot.added) {
    const auto &item = extractor.item(id);
    if (std::find(item.path.begin(), item.path.end(), children.front().get()) == item.path.end()) continue;
    billboard = true;
    auto facing = item.worldTransform.transformDirection({0, 0, 1});
    CHECK(facing.x == doctest::Approx(1));
    CHECK(std::fabs(facing.z) < 0.0001f);
  }
  CHECK(billboard);
  // Same eye, opposite orientation: visibility must use override rotation.
  ctx.setViewerWorldPose(sdk::Mat4::translation({1, 0, 0}) *
                        sdk::Mat4::rotation({0, 1, 0, -1.5707963268f}));
  ctx.tick(2);
  CHECK_FALSE(field<SFBool>(*vs, "isActive"));
  ctx.clearViewerWorldPose();
  ctx.tick(3);
  CHECK_FALSE(field<SFBool>(*ps, "isActive"));
  CHECK(level == 1);
}

TEST_CASE("viewer_world_pose_rejects_non_finite_and_non_rigid_matrices") {
  sdk::X3DExecutionContext ctx;
  const sdk::Mat4 good = sdk::Mat4::translation({1, 2, 3}) *
                         sdk::Mat4::rotation({0, 1, 0, 0.5f});
  REQUIRE(ctx.setViewerWorldPose(good));
  checkMatrix(ctx.viewMatrix(), good.inverse());
  auto rejected = [&](sdk::Mat4 bad) {
    CHECK_FALSE(ctx.setViewerWorldPose(bad));
    checkMatrix(ctx.viewMatrix(), good.inverse()); // previous pose kept
  };
  sdk::Mat4 nan = good; nan.m[13] = std::nanf("");
  rejected(nan);
  sdk::Mat4 inf = good; inf.m[0] = INFINITY;
  rejected(inf);
  sdk::Mat4 scaled = good; for (int i = 0; i < 3; ++i) scaled.m[i] *= 2.0f;
  rejected(scaled);
  sdk::Mat4 sheared = good; sheared.m[4] += 0.2f;
  rejected(sheared);
  sdk::Mat4 mirrored = good; for (int i = 0; i < 3; ++i) mirrored.m[8 + i] = -mirrored.m[8 + i];
  rejected(mirrored);
  sdk::Mat4 projective = good; projective.m[3] = 0.5f;
  rejected(projective);
  rejected(sdk::Mat4{}); // all-zero: singular
  // Float drift inside the tolerance is accepted and re-orthonormalized.
  sdk::Mat4 drift = good; drift.m[0] *= 1.0002f; drift.m[5] += 0.0003f;
  REQUIRE(ctx.setViewerWorldPose(drift));
  const sdk::Mat4 stored = ctx.viewMatrix().inverse();
  for (int c = 0; c < 3; ++c) {
    const float len = std::sqrt(stored.m[c * 4] * stored.m[c * 4] +
                                stored.m[c * 4 + 1] * stored.m[c * 4 + 1] +
                                stored.m[c * 4 + 2] * stored.m[c * 4 + 2]);
    CHECK(len == doctest::Approx(1.0f).epsilon(1e-5));
  }
  CHECK(stored.m[12] == doctest::Approx(1));
  CHECK(stored.m[13] == doctest::Approx(2));
  CHECK(stored.m[14] == doctest::Approx(3));
}
} // namespace

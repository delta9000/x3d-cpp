#include "RuntimeSession.hpp"
#include "X3DParse.hpp"
#include "doctest/doctest.h"

using namespace x3d::runtime;
using namespace x3d::core;

namespace {
void sameMatrix(const Mat4 &actual, const Mat4 &expected) {
  for (int i = 0; i < 16; ++i) CHECK(actual.m[i] == doctest::Approx(expected.m[i]));
}
} // namespace

TEST_CASE("Unbound viewer: origin is preserved and head tracking is effective") {
  auto implicit = RuntimeSession::create(x3d::codec::parseDocument(
      "<X3D profile='Full' version='4.0'><Scene/></X3D>"));
  auto authored = RuntimeSession::create(x3d::codec::parseDocument(
      "<X3D profile='Full' version='4.0'><Scene>"
      "<Viewpoint position='0 0 0'/></Scene></X3D>"));
  auto &ctx = implicit->context();
  CHECK(ctx.boundViewpoint() == nullptr);
  CHECK(implicit->scene().rootNodes.empty());
  sameMatrix(ctx.viewMatrix(), Mat4::identity());
  CHECK(ctx.cameraWorldPosition().z == doctest::Approx(0));
  sameMatrix(ctx.viewMatrix(), authored->context().viewMatrix());
  // The authored query still reports that no authored viewpoint is bound.
  sameMatrix(ctx.boundViewpointWorldPose(), Mat4::identity());

  const SFVec3f head{1, 2, -3};
  const SFRotation rotation{0, 1, 0, 0.7f};
  ctx.setHeadPose(head, rotation);
  authored->context().setHeadPose(head, rotation);
  sameMatrix(ctx.viewMatrix(), authored->context().viewMatrix());
  CHECK(ctx.cameraWorldPosition().x == doctest::Approx(1));
  CHECK(ctx.cameraWorldPosition().y == doctest::Approx(2));
  CHECK(ctx.cameraWorldPosition().z == doctest::Approx(-3));
  sameMatrix(implicit->extractor().camera().viewMatrix, ctx.viewMatrix());

  const auto worldPose = Mat4::translation(SFVec3f{20, 30, 40});
  ctx.setViewerWorldPose(worldPose);
  sameMatrix(ctx.viewMatrix(), worldPose.inverse());
  ctx.setHeadPose(SFVec3f{4, 5, 6}, rotation);
  sameMatrix(ctx.viewMatrix(), worldPose.inverse());
  ctx.clearViewerWorldPose();
  authored->context().setHeadPose(SFVec3f{4, 5, 6}, rotation);
  sameMatrix(ctx.viewMatrix(), authored->context().viewMatrix());

  // Later topology discovery binds a real viewpoint and keeps stored tracking.
  auto incoming = x3d::codec::parseDocument(
      "<X3D profile='Full' version='4.0'><Scene><Viewpoint/></Scene></X3D>");
  implicit->scene().addRootNode(incoming.scene.rootNodes.front());
  ctx.buildSceneGraph(implicit->scene());
  CHECK(ctx.boundViewpoint() != nullptr);
  sameMatrix(ctx.viewMatrix(),
             (Mat4::translation(SFVec3f{0, 0, 10}) *
              Mat4::translation(SFVec3f{4, 5, 6}) * Mat4::rotation(rotation)).inverse());
  // The fallback also exists on the low-level context before scene wiring.
  X3DExecutionContext empty;
  empty.setHeadPose(head, rotation);
  sameMatrix(empty.viewMatrix(),
             (Mat4::translation(head) * Mat4::rotation(rotation)).inverse());
}

TEST_CASE("Unbound viewer: head tracking changes LOD selection from the origin") {
  auto session = RuntimeSession::create(x3d::codec::parseDocument(R"(
    <X3D profile='Full' version='4.0'><Scene>
      <LOD range='5'>
        <Shape><Box/></Shape><Shape><Sphere/></Shape>
      </LOD>
    </Scene></X3D>)"));
  CHECK(session->context().boundViewpoint() == nullptr);
  auto initial = session->fullSnapshot();
  REQUIRE(initial.added.size() == 1);
  CHECK(session->extractor().item(initial.added.front()).geometry.node->nodeTypeName() ==
        "Box");
  session->context().setHeadPose({0, 0, 10}, {0, 0, 1, 0});
  session->tick(0);
  auto changed = session->delta();
  REQUIRE(changed.added.size() == 1);
  CHECK(session->extractor().item(changed.added.front()).geometry.node->nodeTypeName() ==
        "Sphere");
}

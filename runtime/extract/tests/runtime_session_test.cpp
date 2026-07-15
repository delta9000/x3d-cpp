// runtime_session_test.cpp — RuntimeSession contract.
//
// The session exists to own the buildSceneGraph + buildFrom + SceneExtractor
// ceremony and the unwritten lifetime contract between the three objects. These
// cases pin that it is EQUIVALENT to the low-level path (it must not become a
// policy layer with its own behaviour), that it does the wiring a caller can
// silently forget, and that the pieces stay reachable.

#include "RuntimeSession.hpp"

#include "X3DParse.hpp"
#include "doctest/doctest.h"

#include <memory>
#include <string>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;

namespace {

// A Transform driven by a TimeSensor->PositionInterpolator ROUTE chain: renders
// from buildSceneGraph, but only ANIMATES if buildFrom resolved the ROUTEs.
const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Viewpoint DEF="VP" position="0 0 10"/>
    <Transform DEF="Mover">
      <Shape><Appearance><Material/></Appearance><Box size="1 1 1"/></Shape>
    </Transform>
    <TimeSensor DEF="Clock" cycleInterval="4" loop="true"/>
    <PositionInterpolator DEF="Path" key="0 1"
                          keyValue="0 0 0 10 0 0"/>
    <ROUTE fromNode="Clock" fromField="fraction_changed"
           toNode="Path" toField="set_fraction"/>
    <ROUTE fromNode="Path" fromField="value_changed"
           toNode="Mover" toField="translation"/>
  </Scene>
</X3D>)X3D";

X3DDocument parse() {
  return x3d::codec::parseDocument(kScene, x3d::codec::Encoding::XML);
}

} // namespace

TEST_CASE("RuntimeSession: wires both builds, so ROUTEs actually fire") {
  auto s = RuntimeSession::create(parse());

  // buildFrom ran: both authored ROUTEs resolved.
  CHECK(s->routes().routesAdded == 2);
  CHECK(s->routes().rejected.empty());

  s->fullSnapshot();
  CHECK(s->extractor().itemCount() == 1);

  // buildSceneGraph ran: the Viewpoint bound, so viewMatrix is NOT identity.
  // (Skipping buildSceneGraph is silent -- it leaves the camera at identity.)
  const Mat4 view = s->context().viewMatrix();
  CHECK(view.m[14] != 0.0f);

  // The ROUTE chain drives the Transform: tick and the delta reports it.
  s->tick(0.0);
  s->delta();
  s->tick(1.0);
  RenderDelta d = s->delta();
  CHECK_FALSE(d.updatedTransform.empty());
}

TEST_CASE("RuntimeSession: equivalent to the low-level path, not a policy layer") {
  // Same document, both ways -- the session must add no behaviour of its own.
  auto s = RuntimeSession::create(parse());
  RenderDelta viaSession = s->fullSnapshot();

  X3DDocument doc = parse();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  const BridgeResult bridge = ctx.buildFrom(doc.scene);
  extract::SceneExtractor ex(ctx, doc.scene);
  RenderDelta viaLowLevel = ex.fullSnapshot();

  CHECK(s->routes().routesAdded == bridge.routesAdded);
  CHECK(viaSession.added.size() == viaLowLevel.added.size());
  CHECK(s->extractor().itemCount() == ex.itemCount());
  CHECK(viaSession.cameraChanged == viaLowLevel.cameraChanged);
}

TEST_CASE("RuntimeSession: build order does NOT matter (buildFrom first is fine)") {
  // Pins the fact the session's doc comment asserts: the two builds write
  // disjoint state and commute. If this ever stops being true, the session's
  // "you cannot get the order wrong" claim becomes load-bearing rather than
  // convenient -- and this test is where that shows up.
  X3DDocument doc = parse();
  X3DExecutionContext ctx;
  ctx.buildFrom(doc.scene);      // REVERSED order...
  ctx.buildSceneGraph(doc.scene);
  extract::SceneExtractor ex(ctx, doc.scene);
  RenderDelta f0 = ex.fullSnapshot();

  auto s = RuntimeSession::create(parse()); // ...vs the session's order.
  RenderDelta ref = s->fullSnapshot();

  CHECK(f0.added.size() == ref.added.size());
  CHECK(ex.itemCount() == s->extractor().itemCount());
}

TEST_CASE("RuntimeSession: owns the document, so the scene outlives the caller's") {
  // The low-level path's real hazard: doc and ctx must outlive the extractor,
  // which holds references into both. The session takes ownership so a caller
  // cannot drop the document while the extractor still reads it.
  std::unique_ptr<RuntimeSession> s;
  {
    X3DDocument doc = parse();
    s = RuntimeSession::create(std::move(doc)); // caller's doc dies here.
  }
  RenderDelta f0 = s->fullSnapshot(); // still valid: the session owns it.
  CHECK(f0.added.size() == 1);
  CHECK(s->document().scene.rootNodes.size() == 4);
  CHECK(s->scene().resolve("Mover") != nullptr);
}

TEST_CASE("RuntimeSession: delta() totality is inherited, not re-implemented") {
  auto s = RuntimeSession::create(parse());

  // No fullSnapshot() first => delta() promotes to the baseline.
  RenderDelta first = s->delta();
  CHECK(first.added.size() == 1);

  // No tick() in between => empty, not a re-diff.
  RenderDelta second = s->delta();
  CHECK(second.added.empty());
  CHECK(second.updatedTransform.empty());
}

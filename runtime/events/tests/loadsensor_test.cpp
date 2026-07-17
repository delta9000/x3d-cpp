#include "doctest/doctest.h"
// loadsensor_test.cpp — LoadSensorSystem behavior (§9.4.3 Networking). A
// time-driven System observes each LoadSensor's watched X3DUrlObject children
// per tick through the AssetResolver seam and emits isActive/isLoaded/loadTime/
// progress. Tests inject scripted resolvers so no I/O happens here.
#include "InlineExpand.hpp"
#include "LoadSensorSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/Anchor.hpp"
#include "x3d/nodes/ImageTexture.hpp"
#include "x3d/nodes/Inline.hpp"
#include "x3d/nodes/LoadSensor.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Viewpoint.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::runtime;

namespace {

using extract::AssetKind;
using extract::AssetResolver;
using extract::AssetResult;

// A scripted resolver: per-URL queue of results, popped one per call. When a
// URL's queue drains, the last-returned result sticks (so an always-Pending
// tail is easy). Held via shared_ptr so the test inspects `calls` after ticking
// even though the system copies the std::function by value.
struct ScriptState {
  std::map<std::string, std::deque<AssetResult>> seq;
  std::map<std::string, AssetResult> last;
  int calls = 0;
  AssetResult next(const std::string &u) {
    ++calls;
    auto &dq = seq[u];
    if (!dq.empty()) {
      auto r = dq.front();
      dq.pop_front();
      last[u] = r;
      return r;
    }
    auto it = last.find(u);
    return it != last.end() ? it->second : AssetResult::makeFailed();
  }
};

AssetResolver scripted(std::shared_ptr<ScriptState> s) {
  return [s](const std::string &u, AssetKind) { return s->next(u); };
}

std::shared_ptr<x3d::nodes::ImageTexture> tex(std::vector<std::string> urls) {
  auto t = std::make_shared<x3d::nodes::ImageTexture>();
  t->setUrl(MFString(urls.begin(), urls.end()));
  return t;
}

AssetResult ready() { return AssetResult::makeReady({}); }
AssetResult pending() { return AssetResult::makePending(); }
AssetResult failed() { return AssetResult::makeFailed(); }

// Build + wire a LoadSensorSystem over `ls` (already populated and added to
// `scene`) with the given resolver. buildSceneGraph first so emit-thunks live.
std::shared_ptr<LoadSensorSystem> wire(Scene &scene, X3DExecutionContext &ctx,
                                       x3d::nodes::LoadSensor *ls,
                                       AssetResolver r) {
  ctx.buildSceneGraph(scene);
  auto sys = std::make_shared<LoadSensorSystem>(std::move(r));
  sys->setScene(&scene);
  sys->attach(ls, ctx);
  ctx.addSystem(sys);
  return sys;
}

// Shared harness kept for the simplest single-child cases.
struct Rig {
  Scene scene;
  X3DExecutionContext ctx;
  std::shared_ptr<x3d::nodes::LoadSensor> ls;
  std::shared_ptr<x3d::nodes::ImageTexture> texNode;
  std::shared_ptr<LoadSensorSystem> sys;
  explicit Rig(AssetResolver r) {
    ls = std::make_shared<x3d::nodes::LoadSensor>();
    texNode = tex({"a.png"});
    ls->setChildren(MFNode{texNode});
    scene.addRootNode(ls);
    sys = wire(scene, ctx, ls.get(), std::move(r));
  }
};

} // namespace

TEST_CASE("LoadSensor: pending child activates the sensor") {
  Rig rig([](const std::string &, AssetKind) { return pending(); });
  rig.ctx.tick(1.0);
  CHECK(rig.ls->getIsActive());
  CHECK_FALSE(rig.ls->getIsLoaded());
  CHECK(rig.ls->getProgress() == 0.0f);
}

TEST_CASE("LoadSensor: first-tick all-Ready is the NSN-9 burst, no isActive pulse") {
  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {ready()};
  Rig rig(scripted(s));
  rig.ctx.tick(1.0);
  CHECK_FALSE(rig.ls->getIsActive()); // R7: already-loaded case emits no pulse
  CHECK(rig.ls->getIsLoaded());
  CHECK(rig.ls->getLoadTime() == 1.0);
  CHECK(rig.ls->getProgress() == 1.0f);
}

TEST_CASE("LoadSensor: pending,pending,ready activates then bursts success") {
  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {pending(), pending(), ready()};
  Rig rig(scripted(s));

  rig.ctx.tick(1.0);
  CHECK(rig.ls->getIsActive());
  CHECK_FALSE(rig.ls->getIsLoaded());
  CHECK(rig.ls->getProgress() == 0.0f);

  rig.ctx.tick(2.0);
  CHECK(rig.ls->getIsActive());

  rig.ctx.tick(3.0);
  CHECK_FALSE(rig.ls->getIsActive());
  CHECK(rig.ls->getIsLoaded());
  CHECK(rig.ls->getLoadTime() == 3.0);
  CHECK(rig.ls->getProgress() == 1.0f);
}

TEST_CASE("LoadSensor: two children, one ready one pending → progress 0.5 then 1") {
  Scene scene;
  X3DExecutionContext ctx;
  auto ls = std::make_shared<x3d::nodes::LoadSensor>();
  ls->setChildren(MFNode{tex({"a.png"}), tex({"b.png"})});
  scene.addRootNode(ls);
  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {ready()};
  s->seq["b.png"] = {pending(), ready()};
  auto sys = wire(scene, ctx, ls.get(), scripted(s));

  ctx.tick(1.0);
  CHECK(ls->getIsActive());
  CHECK_FALSE(ls->getIsLoaded());
  CHECK(ls->getProgress() == 0.5f);

  ctx.tick(2.0);
  CHECK_FALSE(ls->getIsActive());
  CHECK(ls->getIsLoaded());
  CHECK(ls->getProgress() == 1.0f);
  CHECK(ls->getLoadTime() == 2.0);
}

TEST_CASE("LoadSensor: MFString fallback — first candidate fails, second loads") {
  Scene scene;
  X3DExecutionContext ctx;
  auto ls = std::make_shared<x3d::nodes::LoadSensor>();
  ls->setChildren(MFNode{tex({"bad.png", "good.png"})});
  scene.addRootNode(ls);
  auto s = std::make_shared<ScriptState>();
  s->seq["bad.png"] = {failed()};
  s->seq["good.png"] = {ready()};
  auto sys = wire(scene, ctx, ls.get(), scripted(s));

  ctx.tick(1.0); // ≤1 resolver call/tick: bad.png fails, child stays Loading
  CHECK(ls->getIsActive());
  CHECK_FALSE(ls->getIsLoaded());

  ctx.tick(2.0); // good.png resolves Ready → success
  CHECK_FALSE(ls->getIsActive());
  CHECK(ls->getIsLoaded());
  CHECK(ls->getProgress() == 1.0f);
}

TEST_CASE("LoadSensor: parse-time expanded Inline is pre-seeded Ready (NSN-9)") {
  Scene scene;
  auto ls = std::make_shared<x3d::nodes::LoadSensor>();
  auto inl = std::make_shared<x3d::nodes::Inline>();
  inl->setUrl(MFString{"child"});
  ls->setChildren(MFNode{inl});
  scene.addRootNode(ls);

  // Expand the Inline at "parse time": an in-memory resolver returns a tiny
  // child scene, so the Inline in LoadSensor.children becomes a synthetic Group
  // recorded in scene.expandedInlines (design §1 pre-seed).
  InlineResolver resolver = [](const std::vector<std::string> &,
                               const std::string &) {
    auto s = std::make_shared<Scene>();
    s->addRootNode(X3DNodeFactory::create("Shape"));
    return s;
  };
  std::vector<InlineWarning> warnings;
  expandInlines(scene, resolver, "", warnings);

  REQUIRE(ls->getChildren().size() == 1);
  X3DNode *group = ls->getChildren().at(0).get();
  REQUIRE(scene.expandedInlines.count(group) == 1);

  // A resolver that fails every real call — proves the pre-seed short-circuits it.
  int calls = 0;
  auto sys = std::make_shared<LoadSensorSystem>(
      [&](const std::string &, AssetKind) {
        ++calls;
        return failed();
      });
  sys->setScene(&scene);
  ChildStatus seen = ChildStatus::NotStarted;
  sys->setChildStateHook(
      [&](X3DNode *, X3DNode *, ChildStatus st) { seen = st; });
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  sys->attach(ls.get(), ctx);
  ctx.addSystem(sys);

  ctx.tick(1.0);
  CHECK_FALSE(ls->getIsActive()); // R7: pre-seeded already-loaded, no pulse
  CHECK(ls->getIsLoaded());
  CHECK(ls->getProgress() == 1.0f);
  CHECK(ls->getLoadTime() == 1.0);
  CHECK(seen == ChildStatus::Ready); // childHook observed the pre-seed
  CHECK(calls == 0);                 // pre-seed asked no resolver
}

// ── Task 5: watched-child change resets (NSN-7) ───────────────────────────

TEST_CASE("LoadSensor: url change after success re-activates, no isLoaded=FALSE") {
  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {ready()};
  s->seq["b.png"] = {pending(), ready()};
  Rig rig(scripted(s));

  rig.ctx.tick(1.0); // first url loads → success burst
  CHECK(rig.ls->getIsLoaded());
  CHECK_FALSE(rig.ls->getIsActive());
  CHECK(rig.ls->getLoadTime() == 1.0);

  rig.texNode->setUrl(MFString{"b.png"}); // author changes the url
  rig.ctx.tick(3.0);
  CHECK(rig.ls->getIsActive());  // NSN-7 reset re-activates the sensor
  CHECK(rig.ls->getIsLoaded());  // R4: no spurious isLoaded=FALSE on reset

  rig.ctx.tick(4.0);
  CHECK_FALSE(rig.ls->getIsActive());
  CHECK(rig.ls->getIsLoaded());
  CHECK(rig.ls->getLoadTime() == 4.0); // second success burst
}

TEST_CASE("LoadSensor: url change restarts the timeout window") {
  auto alwaysPending = [](const std::string &, AssetKind) { return pending(); };
  Rig rig(alwaysPending);
  rig.ls->setTimeOut(5.0);

  rig.ctx.tick(1.0); // window opens at t=1
  CHECK(rig.ls->getIsActive());

  rig.texNode->setUrl(MFString{"c.png"});
  rig.ctx.tick(4.0); // url change restarts the window at t=4
  CHECK(rig.ls->getIsActive());

  rig.ctx.tick(8.0); // 8-4 = 4 < 5 — would have timed out at t=6 without restart
  CHECK(rig.ls->getIsActive());

  rig.ctx.tick(10.0); // 10-4 = 6 > 5 → timeout
  CHECK_FALSE(rig.ls->getIsActive());
  CHECK_FALSE(rig.ls->getIsLoaded());
}

TEST_CASE("LoadSensor: load TRUE→FALSE idles the sensor, FALSE→TRUE re-resolves") {
  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {ready()};
  Rig rig(scripted(s));

  rig.ctx.tick(1.0);
  CHECK(rig.ls->getIsLoaded());
  CHECK(rig.ls->getLoadTime() == 1.0);

  rig.texNode->setLoad(false);
  rig.ctx.tick(2.0); // child drops to NotStarted; nothing loading → idle
  CHECK_FALSE(rig.ls->getIsActive());

  rig.texNode->setLoad(true);
  rig.ctx.tick(3.0); // re-resolves (memoized Ready) → fresh success burst
  CHECK(rig.ls->getIsLoaded());
  CHECK(rig.ls->getLoadTime() == 3.0); // proves a new success, not a stale value
}

TEST_CASE("LoadSensor: membership change — empty then add a new child") {
  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {ready()};
  s->seq["d.png"] = {ready()};
  Rig rig(scripted(s));

  rig.ctx.tick(1.0);
  CHECK(rig.ls->getIsLoaded());
  CHECK(rig.ls->getLoadTime() == 1.0);

  rig.ls->setChildren(MFNode{});
  rig.ctx.tick(2.0); // empty watch set after success → quiet
  CHECK(rig.ls->getLoadTime() == 1.0);

  rig.ls->setChildren(MFNode{tex({"d.png"})});
  rig.ctx.tick(3.0); // new child evaluates fresh → new success burst
  CHECK(rig.ls->getIsLoaded());
  CHECK(rig.ls->getLoadTime() == 3.0);
}

TEST_CASE("LoadSensor: USE-duplicated child dedups the progress denominator (R5)") {
  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {ready()};
  Scene scene;
  X3DExecutionContext ctx;
  auto ls = std::make_shared<x3d::nodes::LoadSensor>();
  auto t = tex({"a.png"});
  ls->setChildren(MFNode{t, t}); // the same node USE'd twice
  scene.addRootNode(ls);
  auto sys = wire(scene, ctx, ls.get(), scripted(s));

  ctx.tick(1.0);
  CHECK(ls->getIsLoaded());
  CHECK(ls->getProgress() == 1.0f); // denominator is 1, not 2
  CHECK_FALSE(ls->getIsActive());
  CHECK(s->calls == 1); // deduped → a single resolver call
}

// ── Task 4: timeOut deadline + enabled lifecycle ──────────────────────────

TEST_CASE("LoadSensor: timeOut deadline fires terminal failure, no loadTime") {
  auto alwaysPending = [](const std::string &, AssetKind) { return pending(); };
  Rig rig(alwaysPending);
  rig.ls->setTimeOut(5.0);

  rig.ctx.tick(1.0); // activate; window opens at t=1
  CHECK(rig.ls->getIsActive());

  rig.ctx.tick(7.0); // 7-1 = 6 > 5 → timeout
  CHECK_FALSE(rig.ls->getIsActive());
  CHECK_FALSE(rig.ls->getIsLoaded());
  CHECK(rig.ls->getLoadTime() == 0.0); // loadTime is never sent on failure
}

TEST_CASE("LoadSensor: timeOut=0 monitors indefinitely") {
  auto alwaysPending = [](const std::string &, AssetKind) { return pending(); };
  Rig rig(alwaysPending); // default timeOut is 0

  rig.ctx.tick(1.0);
  CHECK(rig.ls->getIsActive());
  rig.ctx.tick(1000.0);
  CHECK(rig.ls->getIsActive()); // still monitoring
  CHECK_FALSE(rig.ls->getIsLoaded());
}

TEST_CASE("LoadSensor: enabled=FALSE deactivates; re-enable starts fresh") {
  auto alwaysPending = [](const std::string &, AssetKind) { return pending(); };
  Rig rig(alwaysPending);

  rig.ctx.tick(1.0);
  CHECK(rig.ls->getIsActive());

  rig.ls->setEnabled(false);
  rig.ctx.tick(2.0);
  CHECK_FALSE(rig.ls->getIsActive()); // deactivated
  CHECK_FALSE(rig.ls->getIsLoaded()); // no isLoaded emitted by a disable

  rig.ls->setEnabled(true);
  rig.ctx.tick(3.0);
  CHECK(rig.ls->getIsActive()); // fresh cycle re-activates
}

TEST_CASE("LoadSensor: timeOut window starts at activation, not scene start") {
  auto alwaysPending = [](const std::string &, AssetKind) { return pending(); };
  Rig rig(alwaysPending);
  rig.ls->setTimeOut(5.0);

  rig.ctx.tick(10.0); // sensor first activates at t=10
  CHECK(rig.ls->getIsActive());

  rig.ctx.tick(14.0); // 14-10 = 4 < 5, even though 14 > 5 from scene start
  CHECK(rig.ls->getIsActive());

  rig.ctx.tick(16.0); // 16-10 = 6 > 5 → timeout
  CHECK_FALSE(rig.ls->getIsActive());
  CHECK_FALSE(rig.ls->getIsLoaded());
}

TEST_CASE("LoadSensor: all candidates fail → isLoaded FALSE, not active") {
  ChildStatus seen = ChildStatus::NotStarted;
  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {failed()};
  Scene scene;
  X3DExecutionContext ctx;
  auto ls = std::make_shared<x3d::nodes::LoadSensor>();
  ls->setChildren(MFNode{tex({"a.png"})});
  scene.addRootNode(ls);
  ctx.buildSceneGraph(scene);
  auto sys = std::make_shared<LoadSensorSystem>(scripted(s));
  sys->setScene(&scene);
  sys->setChildStateHook(
      [&](X3DNode *, X3DNode *, ChildStatus st) { seen = st; });
  sys->attach(ls.get(), ctx);
  ctx.addSystem(sys);

  ctx.tick(1.0);
  CHECK_FALSE(ls->getIsActive());
  CHECK_FALSE(ls->getIsLoaded());
  CHECK(seen == ChildStatus::Failed);
}

// ── Task 6: edge rulings + policy hook + ROUTE end-to-end ──────────────────

TEST_CASE("LoadSensor: R3 — empty url with load=TRUE is a vacuous success") {
  int calls = 0;
  Scene scene;
  X3DExecutionContext ctx;
  auto ls = std::make_shared<x3d::nodes::LoadSensor>();
  ls->setChildren(MFNode{tex({})}); // empty url list
  scene.addRootNode(ls);
  auto sys = wire(scene, ctx, ls.get(),
                  [&](const std::string &, AssetKind) { ++calls; return failed(); });
  ctx.tick(1.0);
  CHECK(ls->getIsLoaded());
  CHECK(ls->getProgress() == 1.0f);
  CHECK_FALSE(ls->getIsActive());
  CHECK(calls == 0); // R3 short-circuits the resolver
}

TEST_CASE("LoadSensor: R6 — empty children is a vacuous first-tick success") {
  int calls = 0;
  Scene scene;
  X3DExecutionContext ctx;
  auto ls = std::make_shared<x3d::nodes::LoadSensor>();
  ls->setChildren(MFNode{});
  scene.addRootNode(ls);
  auto sys = wire(scene, ctx, ls.get(),
                  [&](const std::string &, AssetKind) { ++calls; return failed(); });
  ctx.tick(1.0);
  CHECK(ls->getIsLoaded());
  CHECK(ls->getProgress() == 1.0f);
  CHECK(ls->getLoadTime() == 1.0);
  CHECK(calls == 0);
}

TEST_CASE("LoadSensor: embedded scheme resolves Ready with no resolver call") {
  int calls = 0;
  Scene scene;
  X3DExecutionContext ctx;
  auto ls = std::make_shared<x3d::nodes::LoadSensor>();
  auto scr = std::make_shared<x3d::nodes::Script>();
  scr->setUrl(MFString{"ecmascript:function initialize(){}"});
  ls->setChildren(MFNode{scr});
  scene.addRootNode(ls);
  auto sys = wire(scene, ctx, ls.get(),
                  [&](const std::string &, AssetKind) { ++calls; return failed(); });
  ctx.tick(1.0);
  CHECK(ls->getIsLoaded());
  CHECK(calls == 0); // bytes are in the url string
}

TEST_CASE("LoadSensor: Anchor '#Name' → Ready iff a Viewpoint DEF exists") {
  { // viewpoint present → Ready without a resolver call
    int calls = 0;
    Scene scene;
    X3DExecutionContext ctx;
    auto vp = std::make_shared<x3d::nodes::Viewpoint>();
    vp->setDEF("Doorway");
    scene.addRootNode(vp);
    auto anchor = std::make_shared<x3d::nodes::Anchor>();
    anchor->setUrl(MFString{"#Doorway"});
    auto ls = std::make_shared<x3d::nodes::LoadSensor>();
    ls->setChildren(MFNode{anchor});
    scene.addRootNode(ls);
    auto sys = wire(scene, ctx, ls.get(), [&](const std::string &, AssetKind) {
      ++calls;
      return failed();
    });
    ctx.tick(1.0);
    CHECK(ls->getIsLoaded());
    CHECK(calls == 0);
  }
  { // no such viewpoint → resolver asked, fails → child Failed
    int calls = 0;
    Scene scene;
    X3DExecutionContext ctx;
    auto anchor = std::make_shared<x3d::nodes::Anchor>();
    anchor->setUrl(MFString{"#Missing"});
    auto ls = std::make_shared<x3d::nodes::LoadSensor>();
    ls->setChildren(MFNode{anchor});
    scene.addRootNode(ls);
    auto sys = wire(scene, ctx, ls.get(), [&](const std::string &, AssetKind) {
      ++calls;
      return failed();
    });
    ctx.tick(1.0);
    CHECK_FALSE(ls->getIsLoaded());
    CHECK(calls >= 1);
  }
}

TEST_CASE("LoadSensor: policy hook — watch=false ignores, vacuousReady short-circuits") {
  { // watch=false drops the child; resolver never called
    int calls = 0;
    Scene scene;
    X3DExecutionContext ctx;
    auto ls = std::make_shared<x3d::nodes::LoadSensor>();
    ls->setChildren(MFNode{tex({"x.png"})});
    scene.addRootNode(ls);
    ctx.buildSceneGraph(scene);
    auto sys = std::make_shared<LoadSensorSystem>(
        [&](const std::string &, AssetKind) { ++calls; return failed(); });
    sys->setScene(&scene);
    sys->setChildLoadPolicy([](X3DNode *, const Scene &) {
      ChildLoadPlan p;
      p.watch = false;
      return p;
    });
    sys->attach(ls.get(), ctx);
    ctx.addSystem(sys);
    ctx.tick(1.0);
    CHECK(calls == 0);
    CHECK(ls->getIsLoaded()); // empty watch set → vacuous success
  }
  { // vacuousReady → watched but Ready with no resolver call
    int calls = 0;
    Scene scene;
    X3DExecutionContext ctx;
    auto ls = std::make_shared<x3d::nodes::LoadSensor>();
    ls->setChildren(MFNode{tex({"x.png"})});
    scene.addRootNode(ls);
    ctx.buildSceneGraph(scene);
    auto sys = std::make_shared<LoadSensorSystem>(
        [&](const std::string &, AssetKind) { ++calls; return failed(); });
    sys->setScene(&scene);
    sys->setChildLoadPolicy([](X3DNode *, const Scene &) {
      ChildLoadPlan p;
      p.vacuousReady = true;
      return p;
    });
    sys->attach(ls.get(), ctx);
    ctx.addSystem(sys);
    ctx.tick(1.0);
    CHECK(calls == 0);
    CHECK(ls->getIsLoaded());
    CHECK(ls->getProgress() == 1.0f);
  }
}

TEST_CASE("LoadSensor: ROUTE loadTime end-to-end via attachStandardRuntime") {
  const char *xml =
      "<X3D profile='Interactive'><Scene>"
      "<LoadSensor DEF='LS'>"
      "<ImageTexture containerField='children' url='\"a.png\"'/>"
      "</LoadSensor>"
      "<TimeSensor DEF='TS'/>"
      "<ROUTE fromNode='LS' fromField='loadTime' toNode='TS' toField='startTime'/>"
      "</Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml);
  Scene &scene = doc.getScene();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);

  auto s = std::make_shared<ScriptState>();
  s->seq["a.png"] = {pending(), ready()};
  attachStandardRuntime(scene, ctx, scripted(s), "");

  x3d::nodes::TimeSensor *ts = nullptr;
  for (auto &r : scene.rootNodes)
    if (auto *p = dynamic_cast<x3d::nodes::TimeSensor *>(r.get()))
      ts = p;
  REQUIRE(ts != nullptr);

  ctx.tick(1.0); // child still loading → no loadTime (proves it is really watched)
  CHECK(ts->getStartTime() == 0.0);

  ctx.tick(2.0); // child loads → loadTime=2.0 routed into TS.startTime
  CHECK(ts->getStartTime() == 2.0);
}

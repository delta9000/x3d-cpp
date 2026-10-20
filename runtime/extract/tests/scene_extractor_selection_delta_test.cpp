#include "RuntimeSession.hpp"
#include "SceneExtractor.hpp"
#include "ViewDependentSystem.hpp"
#include "X3DParse.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"

#include <algorithm>
#include <any>
#include <functional>
#include <map>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

namespace {
using Node = std::shared_ptr<X3DNode>;
void set(const Node &node, const char *name, std::any value) {
  for (const auto &field : node->fields())
    if (field.x3dName == name) { field.set(*node, std::move(value)); return; }
  FAIL("fixture field missing");
}
Node shape() {
  auto node = createX3DNode("Shape");
  set(node, "geometry", createX3DNode("Box"));
  return node;
}
void apply(std::map<RenderItemId, RenderItem> &live, const RenderDelta &delta,
           const SceneExtractor &ex) {
  for (auto id : delta.removed) live.erase(id);
  for (auto id : delta.added) live.insert_or_assign(id, ex.item(id));
  for (const auto *ids : {&delta.updatedTransform, &delta.updatedGeometry, &delta.updatedMaterial})
    for (auto id : *ids) {
      REQUIRE(live.count(id) == 1);
      live.insert_or_assign(id, ex.item(id));
    }
}
void compare(const std::map<RenderItemId, RenderItem> &live,
             const X3DExecutionContext &ctx, const Scene &scene) {
  SceneExtractor oracle(ctx, scene);
  auto snap = oracle.fullSnapshot();
  REQUIRE(live.size() == snap.added.size());
  for (auto id : snap.added) {
    const auto &expected = oracle.item(id);
    auto actual = std::find_if(live.begin(), live.end(), [&](const auto &entry) {
      return entry.second.path == expected.path;
    });
    REQUIRE(actual != live.end());
    CHECK(actual->second.worldTransform.m == expected.worldTransform.m);
    CHECK(actual->second.mesh->positions == expected.mesh->positions);
    CHECK(actual->second.localFog == expected.localFog);
    CHECK(actual->second.lights == expected.lights);
  }
}
}

TEST_CASE("selection delta: Switch preserves unrelated IDs through empty selection and reactivation") {
  auto sw = createX3DNode("Switch");
  auto first = shape(), second = shape(), fixed = shape();
  set(sw, "children", MFNode{first, second}); set(sw, "whichChoice", SFInt32{0});
  auto parent = createX3DNode("Transform"); set(parent, "children", MFNode{sw});
  Scene scene; scene.rootNodes = {parent, fixed};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  REQUIRE(snap.added.size() == 2);
  const auto fixedId = snap.added[1], firstId = snap.added[0];
  const auto fixedMesh = ex.item(fixedId).mesh;
  std::map<RenderItemId, RenderItem> live; apply(live, snap, ex);
  for (int choice : {1, -1, 0, 1, 0}) {
    ctx.postEvent(sw.get(), "whichChoice", SFInt32{choice});
    ctx.postEvent(parent.get(), "translation", SFVec3f{float(choice + 3), 0, 0});
    ctx.tick(1);
    auto delta = ex.delta();
    CHECK(delta.added.size() == (choice == -1 ? 0 : 1));
    CHECK(delta.removed.size() == (live.size() == 1 ? 0 : 1));
    CHECK(std::count(delta.added.begin(), delta.added.end(), fixedId) == 0);
    CHECK(std::count(delta.removed.begin(), delta.removed.end(), fixedId) == 0);
    CHECK(ex.item(fixedId).mesh == fixedMesh);
    apply(live, delta, ex); compare(live, ctx, scene);
    REQUIRE(live.count(fixedId) == 1);
    if (choice == 0) CHECK(live.count(firstId) == 1);
    CHECK_FALSE(delta.lightsChanged);
  }
}

TEST_CASE("selection delta: shared LOD changes only the crossing placement") {
  auto lod = createX3DNode("LOD");
  set(lod, "range", MFFloat{5}); set(lod, "children", MFNode{shape(), shape()});
  auto left = createX3DNode("Transform"), right = createX3DNode("Transform");
  set(left, "children", MFNode{lod}); set(right, "children", MFNode{lod});
  set(right, "translation", SFVec3f{20, 0, 0});
  auto fixed = shape();
  auto viewpoint = createX3DNode("Viewpoint");
  set(viewpoint, "position", SFVec3f{0, 0, 0});
  Scene scene; scene.rootNodes = {left, right, fixed, viewpoint};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  REQUIRE(ctx.boundViewpoint() == viewpoint.get());
  REQUIRE(ctx.cameraWorldPosition().x == doctest::Approx(0));
  auto view = std::make_shared<ViewDependentSystem>();
  view->attach(lod.get(), ctx); ctx.addSystem(view);
  SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  REQUIRE(snap.added.size() == 3);
  const auto stableId = snap.added[1], fixedId = snap.added[2];
  const auto stableMesh = ex.item(stableId).mesh;
  std::map<RenderItemId, RenderItem> live; apply(live, snap, ex);
  ctx.tick(0); // first level_changed announcement does not change selection
  auto delta = ex.delta();
  CHECK(delta.added.empty()); CHECK(delta.removed.empty());
  ctx.setHeadPose({8, 0, 0}, {0, 1, 0, 0}); ctx.tick(1);
  REQUIRE(ctx.cameraWorldPosition().x == doctest::Approx(8));
  delta = ex.delta();
  REQUIRE(delta.added.size() == 1); REQUIRE(delta.removed.size() == 1);
  CHECK(delta.removed[0] == snap.added[0]);
  CHECK(ex.item(delta.added[0]).path.front() == left.get());
  apply(live, delta, ex); compare(live, ctx, scene);
  CHECK(live.count(stableId) == 1); CHECK(live.count(fixedId) == 1);
  CHECK(ex.item(stableId).mesh == stableMesh);
  CHECK_FALSE(delta.lightsChanged);
}

TEST_CASE("selection delta: inactive geometry edits are read on reactivation") {
  auto sw = createX3DNode("Switch"); auto item = shape();
  auto box = geombounds::getNode(*item, "geometry");
  set(sw, "children", MFNode{item}); set(sw, "whichChoice", SFInt32{0});
  Scene scene; scene.rootNodes = {sw};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene); auto snap = ex.fullSnapshot();
  const auto oldGeom = ex.item(snap.added[0]).geometry;
  ctx.postEvent(sw.get(), "whichChoice", SFInt32{-1}); ctx.tick(1); ex.delta();
  ctx.postEvent(box.get(), "size", SFVec3f{8, 8, 8}); ctx.tick(2);
  auto idle = ex.delta(); CHECK(idle.added.empty());
  ctx.postEvent(sw.get(), "whichChoice", SFInt32{0}); ctx.tick(3);
  auto delta = ex.delta(); REQUIRE(delta.added.size() == 1);
  CHECK(ex.item(delta.added[0]).geometry.contentVersion != oldGeom.contentVersion);
  std::map<RenderItemId, RenderItem> live; apply(live, delta, ex); compare(live, ctx, scene);
}

TEST_CASE("light delta: idle and unrelated animation are quiet; rendered light changes notify") {
  auto light = createX3DNode("PointLight"), moving = createX3DNode("Transform");
  auto lightFrame = createX3DNode("Transform"); set(lightFrame, "children", MFNode{light});
  Scene scene; scene.rootNodes = {lightFrame, moving};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene); ex.fullSnapshot();
  ctx.tick(0); CHECK_FALSE(ex.delta().lightsChanged);
  ctx.postEvent(moving.get(), "translation", SFVec3f{3, 0, 0}); ctx.tick(1);
  CHECK_FALSE(ex.delta().lightsChanged);
  ctx.postEvent(light.get(), "intensity", SFFloat{0.5f}); ctx.tick(2);
  CHECK(ex.delta().lightsChanged);
  ctx.postEvent(light.get(), "intensity", SFFloat{0.5f}); ctx.tick(3);
  CHECK_FALSE(ex.delta().lightsChanged);
  ctx.postEvent(lightFrame.get(), "translation", SFVec3f{4, 0, 0}); ctx.tick(4);
  CHECK(ex.delta().lightsChanged);
  ctx.postEvent(light.get(), "on", false); ctx.tick(5); CHECK(ex.delta().lightsChanged);
  ctx.tick(6); CHECK_FALSE(ex.delta().lightsChanged);
  ctx.postEvent(light.get(), "on", true); ctx.tick(7); CHECK(ex.delta().lightsChanged);
}

TEST_CASE("tick propagation reuses descendant local matrices and local bounds") {
  auto root = createX3DNode("Transform"); MFNode children;
  for (int i = 0; i < 128; ++i) {
    auto child = createX3DNode("Transform"); set(child, "children", MFNode{shape()});
    children.push_back(child);
  }
  set(root, "children", children);
  Scene scene; scene.rootNodes = {root};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  const auto before = TransformSystem::localMatrixCallCount();
  ctx.postEvent(root.get(), "translation", SFVec3f{7, 0, 0});
  X3DExecutionContext::TickTimings timings; ctx.tick(1, &timings);
  CHECK(TransformSystem::localMatrixCallCount() - before == 1);
  for (const auto &child : children) {
    CHECK(ctx.worldTransform(child.get()).m[12] == doctest::Approx(7));
    CHECK((ctx.dirtyTracker().flags(child.get()) & DirtyBounds) == 0);
    CHECK(ctx.localBounds(child.get()).min.x == doctest::Approx(-1));
  }
}

TEST_CASE("selection delta: nested selections rebuild scoped light and fog indices") {
  auto light = createX3DNode("PointLight"); set(light, "global", false);
  auto fog = createX3DNode("LocalFog"); set(fog, "visibilityRange", SFFloat{10});
  auto branch = createX3DNode("Group"); set(branch, "children", MFNode{light, fog, shape()});
  auto nested = createX3DNode("Switch");
  set(nested, "children", MFNode{branch}); set(nested, "whichChoice", SFInt32{0});
  auto outer = createX3DNode("Switch");
  set(outer, "children", MFNode{nested}); set(outer, "whichChoice", SFInt32{0});
  auto fixed = shape(); Scene scene; scene.rootNodes = {outer, fixed};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene); auto snap = ex.fullSnapshot();
  REQUIRE(snap.added.size() == 2); const auto fixedId = snap.added[1];
  std::map<RenderItemId, RenderItem> live; apply(live, snap, ex);
  for (int choice : {-1, 0, -1, 0}) {
    ctx.postEvent(outer.get(), "whichChoice", SFInt32{choice}); ctx.tick(1);
    auto delta = ex.delta(); CHECK(delta.lightsChanged);
    CHECK(delta.added.size() == (choice < 0 ? 0 : 1));
    CHECK(delta.removed.size() == (choice < 0 ? 1 : 0));
    apply(live, delta, ex); compare(live, ctx, scene);
    REQUIRE(live.count(fixedId) == 1);
    CHECK(ex.item(fixedId).localFog == -1); CHECK(ex.item(fixedId).lights.empty());
  }
  // Nested selection dependencies were rebuilt when the outer branch returned.
  ctx.postEvent(nested.get(), "whichChoice", SFInt32{-1}); ctx.tick(2);
  auto delta = ex.delta(); CHECK(delta.removed.size() == 1); CHECK(delta.lightsChanged);
  apply(live, delta, ex); compare(live, ctx, scene);
}

namespace {
// Performs queued writes from inside a tick, as a System would (writeField
// reaches fields, such as initializeOnly geoSystem, that events do not).
struct WriterSystem : System {
  std::vector<std::function<void(X3DExecutionContext &)>> pending;
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    auto writes = std::move(pending);
    pending.clear();
    for (auto &write : writes) write(ctx);
  }
  ~WriterSystem() override { retireCallbacksBeforeDestruction(); }
};
std::string document(const std::string &body) {
  return "<X3D profile='Full' version='4.0'><Scene>" + body + "</Scene></X3D>";
}
bool lists(const std::vector<RenderItemId> &ids, RenderItemId id) {
  return std::count(ids.begin(), ids.end(), id) > 0;
}
} // namespace

TEST_CASE("selection delta: a surviving record whose texture resolved is published") {
  auto doc = x3d::codec::parseDocument(document(
      "<Switch DEF='S' whichChoice='0'><Shape><Sphere/></Shape></Switch>"
      "<Shape><Appearance><ImageTexture url='&quot;t.png&quot;'/></Appearance><Box/></Shape>"),
      x3d::codec::Encoding::XML);
  int calls = 0;
  SessionOptions opts;
  opts.textureResolver = [&](const std::string &) {
    if (++calls == 1) return TexturePixelResult::makePending();
    TexturePixels p; p.width = 1; p.height = 1; p.rgba = {1, 2, 3, 4};
    return TexturePixelResult::makeReady(std::move(p));
  };
  auto session = RuntimeSession::create(std::move(doc), std::move(opts));
  auto snap = session->fullSnapshot();
  RenderItemId boxId = 0;
  for (auto id : snap.added)
    if (session->extractor().item(id).geometry.node->nodeTypeName() == "Box") boxId = id;
  REQUIRE(session->extractor().item(boxId).material.textures.at(0).resolvedPixels.pending());
  session->context().postEvent(session->scene().defs.at("S").get(), "whichChoice",
                               std::any(SFInt32{-1}));
  session->tick(1);
  auto d = session->delta();
  REQUIRE(session->extractor().item(boxId).material.textures.at(0).resolvedPixels.ready());
  CHECK(lists(d.updatedMaterial, boxId));
  CHECK_FALSE(lists(d.added, boxId));
  CHECK_FALSE(lists(d.removed, boxId));

  // An unchanged surviving record is not republished.
  session->context().postEvent(session->scene().defs.at("S").get(), "whichChoice",
                               std::any(SFInt32{0}));
  session->tick(2);
  d = session->delta();
  CHECK(d.added.size() == 1);
  CHECK_FALSE(lists(d.updatedMaterial, boxId));
}

TEST_CASE("light delta: lights outside the rendered walk still notify") {
  auto doc = x3d::codec::parseDocument(document(
      "<Group visible='false'><PointLight DEF='Hidden' global='true'/>"
      "  <LOD range='5'><Group/><PointLight DEF='Far' global='true'/></LOD></Group>"
      "<Collision><PointLight DEF='Proxy' global='true' containerField='proxy'/></Collision>"
      "<Shape><Box/></Shape>"), x3d::codec::Encoding::XML);
  auto session = RuntimeSession::create(std::move(doc));
  session->fullSnapshot();
  auto &ctx = session->context();
  double now = 0;
  auto expectChange = [&](const char *def) {
    INFO(def);
    const auto before = session->extractor().lights();
    ctx.postEvent(session->scene().defs.at(def).get(), "intensity", std::any(SFFloat{0.25f}));
    session->tick(++now);
    auto d = session->delta();
    CHECK(session->extractor().lights() != before);
    CHECK(d.lightsChanged);
  };
  expectChange("Hidden");
  expectChange("Proxy");
  // An LOD below the invisible group switches with the viewer alone.
  const auto before = session->extractor().lights();
  ctx.setHeadPose({8, 0, 0}, {0, 1, 0, 0});
  session->tick(++now);
  auto d = session->delta();
  REQUIRE(session->extractor().lights().size() == before.size() + 1);
  CHECK(d.lightsChanged);
  expectChange("Far"); // the newly selected level's light is now tracked
  session->tick(++now);
  CHECK_FALSE(session->delta().lightsChanged);
}

TEST_CASE("selection delta: an LOD boundary crossing reuses the deselected level's mesh") {
  auto lod = createX3DNode("LOD");
  auto nearShape = shape(), farShape = shape();
  auto nearBox = geombounds::getNode(*nearShape, "geometry");
  set(lod, "range", MFFloat{5}); set(lod, "children", MFNode{nearShape, farShape});
  Scene scene; scene.rootNodes = {lod};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  REQUIRE(snap.added.size() == 1);
  const auto nearId = snap.added[0];
  const auto nearGeom = ex.item(nearId).geometry;
  const auto nearMesh = ex.item(nearId).mesh;
  double now = 0;
  auto cross = [&](float x) {
    ctx.setHeadPose({x, 0, 0}, {0, 1, 0, 0}); ctx.tick(++now);
    return ex.delta();
  };
  for (int i = 0; i < 3; ++i) {
    auto away = cross(8);
    REQUIRE(away.removed.size() == 1);
    auto back = cross(0);
    REQUIRE(back.added == std::vector<RenderItemId>{nearId});
    CHECK(ex.item(nearId).geometry == nearGeom);
    CHECK(ex.item(nearId).mesh == nearMesh);
  }
  // A dormant level's edit is still read on reselection, under a new GeomId.
  cross(8);
  ctx.postEvent(nearBox.get(), "size", SFVec3f{4, 4, 4}); ctx.tick(++now); ex.delta();
  auto back = cross(0);
  REQUIRE(back.added == std::vector<RenderItemId>{nearId});
  CHECK(ex.item(nearId).geometry != nearGeom);
  CHECK(ex.item(nearId).mesh != nearMesh);
  std::map<RenderItemId, RenderItem> live; apply(live, back, ex); compare(live, ctx, scene);
}

TEST_CASE("dirty classification: whichChoice is a selection only on a Switch") {
  auto doc = x3d::codec::parseDocument(document(
      "<Switch DEF='S' whichChoice='0'><Shape><Box/></Shape></Switch>"
      "<Script DEF='Sc'><field name='whichChoice' type='SFInt32' accessType='inputOutput' value='0'/></Script>"),
      x3d::codec::Encoding::XML);
  X3DExecutionContext ctx; ctx.buildSceneGraph(doc.scene);
  auto *sw = doc.scene.defs.at("S").get();
  auto *script = doc.scene.defs.at("Sc").get();
  ctx.postEvent(sw, "whichChoice", std::any(SFInt32{-1}));
  ctx.postEvent(script, "whichChoice", std::any(SFInt32{3}));
  ctx.tick(1);
  CHECK((ctx.dirtyTracker().flags(sw) & DirtyActiveChild) != 0);
  CHECK((ctx.dirtyTracker().flags(sw) & DirtyField) == 0);
  CHECK((ctx.dirtyTracker().flags(script) & DirtyField) != 0);
  CHECK((ctx.dirtyTracker().flags(script) & DirtyActiveChild) == 0);
}

TEST_CASE("tick propagation refreshes a geo transform's cached local matrix on geoSystem") {
  auto doc = x3d::codec::parseDocument(document(
      "<Transform DEF='A'><GeoTransform DEF='G' geoCenter='0.5 0.2 100' geoSystem='\"GD\" \"WE\"'>"
      "<Shape><Box/></Shape></GeoTransform></Transform>"), x3d::codec::Encoding::XML);
  X3DExecutionContext ctx; ctx.buildSceneGraph(doc.scene);
  auto writer = std::make_shared<WriterSystem>(); ctx.addSystem(writer);
  auto *a = doc.scene.defs.at("A").get();
  auto *g = doc.scene.defs.at("G").get();
  const Mat4 before = TransformSystem::localMatrix(g, ctx.geoProjection());
  writer->pending.push_back([g](X3DExecutionContext &c) {
    (void)c.writeField(g, "geoSystem", std::any(MFString{"GC"}));
  });
  ctx.tick(1);
  const Mat4 fresh = TransformSystem::localMatrix(g, ctx.geoProjection());
  REQUIRE(fresh.m != before.m);
  auto same = [](const Mat4 &x, const Mat4 &y) {
    for (std::size_t i = 0; i < 16; ++i) CHECK(x.m[i] == doctest::Approx(y.m[i]).epsilon(1e-4));
  };
  same(ctx.worldTransform(g), fresh);
  // An ancestor move recomposes G from its cached local matrix.
  ctx.postEvent(a, "translation", std::any(SFVec3f{1, 2, 3}));
  ctx.tick(2);
  same(ctx.worldTransform(g), Mat4::translation({1, 2, 3}) * fresh);
}

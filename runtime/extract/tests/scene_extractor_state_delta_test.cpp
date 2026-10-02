// Stateful descriptor deltas must reproduce a fresh snapshot without consumers
// rereading unrelated fields when only updatedTransform/updatedGeometry is set.
#include "SceneExtractor.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"

#include <algorithm>
#include <any>
#include <map>
#include <memory>
#include <set>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

namespace {
using Node = std::shared_ptr<X3DNode>;
void set(const Node &node, const char *name, std::any value) {
  for (const auto &field : node->fields())
    if (field.x3dName == name && field.set) {
      field.set(*node, std::move(value));
      return;
    }
  FAIL("missing initialization field");
}
Node shape(Node geometry = createX3DNode("Box")) {
  auto result = createX3DNode("Shape");
  set(result, "geometry", geometry);
  return result;
}
Node group(std::vector<Node> children, const char *type = "Transform") {
  auto result = createX3DNode(type);
  set(result, "children", std::move(children));
  return result;
}

struct StateMirror {
  std::map<RenderItemId, RenderItem> items;
  std::vector<LocalFogDesc> fogs;
  void apply(const RenderDelta &delta, const SceneExtractor &ex) {
    std::set<RenderItemId> removed, added;
    for (auto id : delta.removed) {
      REQUIRE(removed.insert(id).second);
      REQUIRE(items.erase(id) == 1);
    }
    for (auto id : delta.added) {
      REQUIRE(added.insert(id).second);
      REQUIRE(items.emplace(id, ex.item(id)).second);
    }
    // Each incremental channel grants access only to its named content. Copying
    // whole items here would hide a missing descriptor-update notification.
    for (auto id : delta.updatedTransform) {
      REQUIRE(items.count(id) == 1);
      items.at(id).worldTransform = ex.item(id).worldTransform;
    }
    for (auto id : delta.updatedGeometry) {
      REQUIRE(items.count(id) == 1);
      items.at(id).geometry = ex.item(id).geometry;
      items.at(id).mesh = ex.item(id).mesh;
      items.at(id).geometry_ext = ex.item(id).geometry_ext;
    }
    for (auto id : delta.updatedMaterial) {
      REQUIRE(items.count(id) == 1);
      items.at(id).material = ex.item(id).material;
    }
    if (delta.fogChanged) fogs = ex.snapshotLocalFogs();
  }
  void compare(const X3DExecutionContext &ctx, const Scene &scene) const {
    SceneExtractor oracle(ctx, scene);
    auto snapshot = oracle.fullSnapshot();
    REQUIRE(items.size() == snapshot.added.size());
    const auto &expectedFogs = oracle.snapshotLocalFogs();
    REQUIRE(fogs.size() == expectedFogs.size());
    for (std::size_t i = 0; i < fogs.size(); ++i) {
      CHECK(fogs[i].color == expectedFogs[i].color);
      CHECK(fogs[i].fogType == expectedFogs[i].fogType);
      CHECK(fogs[i].visibilityRange == doctest::Approx(expectedFogs[i].visibilityRange));
      CHECK(fogs[i].scopeRoot == expectedFogs[i].scopeRoot);
    }
    for (auto id : snapshot.added) {
      const auto &expected = oracle.item(id);
      const auto found = std::find_if(items.begin(), items.end(), [&](const auto &entry) {
        return entry.second.path == expected.path;
      });
      REQUIRE(found != items.end());
      const auto &actual = found->second;
      CHECK(actual.castShadow == expected.castShadow);
      CHECK(actual.localFog == expected.localFog);
      REQUIRE(actual.clipPlanes.size == expected.clipPlanes.size);
      for (std::size_t i = 0; i < actual.clipPlanes.size; ++i) {
        CHECK(actual.clipPlanes.items[i].node == expected.clipPlanes.items[i].node);
        const auto &a = actual.clipPlanes.items[i].planeWorld;
        const auto &e = expected.clipPlanes.items[i].planeWorld;
        CHECK(a.x == doctest::Approx(e.x));
        CHECK(a.y == doctest::Approx(e.y));
        CHECK(a.z == doctest::Approx(e.z));
        CHECK(a.w == doctest::Approx(e.w));
      }
      for (int i = 0; i < 16; ++i)
        CHECK(actual.worldTransform.m[i] == doctest::Approx(expected.worldTransform.m[i]));
      CHECK(actual.mesh->positions == expected.mesh->positions);
      CHECK(actual.mesh->indices == expected.mesh->indices);
    }
  }
};

struct Fixture {
  Scene scene;
  X3DExecutionContext ctx;
  SceneExtractor ex;
  StateMirror mirror;
  explicit Fixture(std::vector<Node> roots) : ex(ctx, scene) {
    scene.rootNodes = std::move(roots);
    ctx.buildSceneGraph(scene);
    mirror.apply(ex.fullSnapshot(), ex);
    mirror.compare(ctx, scene);
  }
  RenderDelta event(const Node &node, const char *field, std::any value) {
    ctx.postEvent(node.get(), field, std::move(value));
    ctx.tick(1.0); // repeated timestamps deliberately exercise the tick guard
    auto delta = ex.delta();
    mirror.apply(delta, ex);
    mirror.compare(ctx, scene);
    return delta;
  }
};
void replacement(const RenderDelta &delta, std::size_t before, std::size_t after) {
  CHECK(delta.removed.size() == before);
  CHECK(delta.added.size() == after);
  CHECK(delta.updatedTransform.empty());
  CHECK(delta.updatedGeometry.empty());
  CHECK(delta.updatedMaterial.empty());
  CHECK(delta.updatedSkinPose.empty());
  CHECK(delta.fogChanged);
}
} // namespace

TEST_CASE("render state delta: visible changes active membership across shared and hidden paths") {
  auto shared = shape();
  auto left = group({shared});
  auto right = group({shared});
  auto hidden = group({shape()});
  set(hidden, "visible", false);
  Fixture f({left, right, hidden});
  REQUIRE(f.mirror.items.size() == 2);
  replacement(f.event(shared, "visible", false), 2, 0);
  CHECK((f.ctx.dirtyTracker().flags(shared.get()) & DirtyChildren) != 0);
  replacement(f.event(shared, "visible", true), 0, 2);
  replacement(f.event(left, "visible", false), 2, 1);
  replacement(f.event(hidden, "visible", true), 1, 2);
  replacement(f.event(left, "visible", true), 2, 3);
}

TEST_CASE("render state delta: castShadow replaces records for every shared placement") {
  auto shared = shape();
  Fixture f({group({shared}), group({shared}), shape()});
  for (bool casts : {false, true, false}) {
    replacement(f.event(shared, "castShadow", casts), 3, 3);
    for (const auto &[id, item] : f.mirror.items) {
      (void)id;
      if (item.path.back() == shared.get()) CHECK(item.castShadow == casts);
    }
  }
}

TEST_CASE("render state delta: clip fields and ancestor frames replace world plane descriptors") {
  auto clip = createX3DNode("ClipPlane");
  auto movingShape = group({shape()});
  auto shared = group({shape(), clip, movingShape});
  auto left = group({shared});
  auto right = group({shared});
  set(right, "translation", SFVec3f{0, 10, 0});
  auto unrelated = group({shape()});
  Fixture f({left, right, unrelated});
  replacement(f.event(clip, "plane", SFVec4f{0, 1, 0, -2}), 5, 5);
  replacement(f.event(left, "translation", SFVec3f{0, 5, 0}), 5, 5);
  std::vector<float> distances;
  for (const auto &[id, item] : f.mirror.items) {
    (void)id;
    if (item.clipPlanes.size) distances.push_back(item.clipPlanes.items[0].planeWorld.w);
  }
  std::sort(distances.begin(), distances.end());
  REQUIRE(distances.size() == 2);
  CHECK(distances[0] == doctest::Approx(-12));
  CHECK(distances[1] == doctest::Approx(-7));
  replacement(f.event(clip, "enabled", false), 5, 5);
  replacement(f.event(clip, "enabled", true), 5, 5);
  auto delta = f.event(unrelated, "translation", SFVec3f{3, 0, 0});
  CHECK(delta.added.empty()); CHECK(delta.removed.empty());
  CHECK(delta.updatedTransform.size() == 1);
  // An item's own frame below the clip changes no clip equation, either.
  delta = f.event(movingShape, "translation", SFVec3f{4, 0, 0});
  CHECK(delta.added.empty()); CHECK(delta.removed.empty());
  CHECK(delta.updatedTransform.size() == 2);
}

TEST_CASE("render state delta: disabled clip dependencies survive an empty geometry baseline") {
  auto clip = createX3DNode("ClipPlane");
  set(clip, "enabled", false);
  auto coord = createX3DNode("Coordinate");
  auto geometry = createX3DNode("TriangleSet");
  set(geometry, "coord", coord);
  auto frame = group({clip, shape(geometry)});
  Fixture f({frame});
  REQUIRE(f.mirror.items.empty());
  replacement(f.event(clip, "enabled", true), 0, 0);
  replacement(f.event(frame, "translation", SFVec3f{0, 5, 0}), 0, 0);
  auto delta = f.event(coord, "point", MFVec3f{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}});
  REQUIRE(delta.added.size() == 1);
  const auto &item = f.mirror.items.at(delta.added[0]);
  REQUIRE(item.clipPlanes.size == 1);
  CHECK(item.clipPlanes.items[0].planeWorld.w == doctest::Approx(-5));
}

TEST_CASE("render state delta: local fog edits refresh descriptors and nearest scope indices") {
  auto fog = createX3DNode("LocalFog");
  set(fog, "visibilityRange", 10.0f);
  auto outer = createX3DNode("LocalFog");
  set(outer, "visibilityRange", 2.0f);
  auto left = group({fog, shape()});
  auto right = group({fog, shape()}); // one source, two distinct enclosing scopes
  set(right, "scale", SFVec3f{2, 2, 2});
  auto unrelated = group({shape()});
  Fixture f({group({outer, left, right, unrelated})});
  replacement(f.event(fog, "visibilityRange", 20.0f), 3, 3);
  replacement(f.event(fog, "color", SFColor{0.1f, 0.2f, 0.3f}), 3, 3);
  replacement(f.event(fog, "fogType", FogTypeChoices::EXPONENTIAL), 3, 3);
  replacement(f.event(left, "scale", SFVec3f{3, 3, 3}), 3, 3);
  REQUIRE(f.mirror.fogs.size() == 3);
  CHECK(f.mirror.fogs[1].visibilityRange == doctest::Approx(60));
  CHECK(f.mirror.fogs[2].visibilityRange == doctest::Approx(40));
  replacement(f.event(fog, "enabled", false), 3, 3);
  REQUIRE(f.mirror.fogs.size() == 1);
  for (const auto &[id, item] : f.mirror.items) {
    (void)id;
    CHECK(item.localFog == 0); // outer fog wins after inner fog is disabled
  }
  replacement(f.event(fog, "enabled", true), 3, 3);
  auto delta = f.event(unrelated, "translation", SFVec3f{3, 0, 0});
  CHECK(delta.added.empty()); CHECK(delta.removed.empty());
  CHECK(delta.updatedTransform.size() == 1);
}

TEST_CASE("render state delta: disabled fog and frame remain tracked without render items") {
  auto fog = createX3DNode("LocalFog");
  set(fog, "enabled", false);
  set(fog, "visibilityRange", 10.0f);
  auto hidden = shape();
  set(hidden, "visible", false);
  auto frame = group({fog, hidden});
  Fixture f({frame});
  REQUIRE(f.mirror.items.empty());
  replacement(f.event(fog, "enabled", true), 0, 0);
  replacement(f.event(frame, "scale", SFVec3f{3, 3, 3}), 0, 0);
  REQUIRE(f.mirror.fogs.size() == 1);
  CHECK(f.mirror.fogs[0].visibilityRange == doctest::Approx(30));
  replacement(f.event(hidden, "visible", true), 0, 1);
  CHECK(f.mirror.items.begin()->second.localFog == 0);
}

TEST_CASE("render state delta: root state and simultaneous edits coalesce into one replacement") {
  auto clip = createX3DNode("ClipPlane");
  auto fog = createX3DNode("LocalFog");
  set(clip, "enabled", false);
  set(fog, "enabled", false);
  auto item = shape();
  auto hidden = group({item});
  set(hidden, "visible", false);
  Fixture f({clip, fog, hidden});
  f.ctx.postEvent(clip.get(), "enabled", true);
  f.ctx.postEvent(clip.get(), "plane", SFVec4f{0, 1, 0, -4});
  f.ctx.postEvent(fog.get(), "enabled", true);
  f.ctx.postEvent(fog.get(), "visibilityRange", 7.0f);
  f.ctx.postEvent(hidden.get(), "visible", true);
  f.ctx.postEvent(item.get(), "castShadow", false);
  f.ctx.tick(1.0);
  auto delta = f.ex.delta();
  replacement(delta, 0, 1);
  f.mirror.apply(delta, f.ex);
  f.mirror.compare(f.ctx, f.scene);
  const auto &actual = f.mirror.items.begin()->second;
  CHECK_FALSE(actual.castShadow);
  REQUIRE(actual.clipPlanes.size == 1);
  CHECK(actual.clipPlanes.items[0].planeWorld.w == doctest::Approx(-4));
  CHECK(actual.localFog == 0);
  REQUIRE(f.mirror.fogs.size() == 1);
  CHECK(f.mirror.fogs[0].visibilityRange == doctest::Approx(7));
  auto empty = f.ex.delta();
  CHECK(empty.added.empty()); CHECK(empty.removed.empty());
  CHECK_FALSE(empty.fogChanged);
}

TEST_CASE("render state delta: tracked Billboard clip frames replace world planes per placement") {
  auto viewpoint = createX3DNode("Viewpoint");
  set(viewpoint, "position", SFVec3f{0, 0, 10});
  auto clip = createX3DNode("ClipPlane");
  set(clip, "plane", SFVec4f{0, 0, 1, -2});
  auto billboard = group({clip, shape()}, "Billboard");
  Fixture f({viewpoint, billboard});
  REQUIRE(f.mirror.items.size() == 1);
  REQUIRE(f.mirror.items.begin()->second.clipPlanes.size == 1);
  CHECK(f.mirror.items.begin()->second.clipPlanes.items[0].planeWorld.z == doctest::Approx(1));
  f.ctx.setHeadPose({10, 0, -10}, {0, 1, 0, 0}); // tracked eye now at +X
  f.ctx.tick(1.0);
  auto delta = f.ex.delta();
  replacement(delta, 1, 1);
  f.mirror.apply(delta, f.ex);
  f.mirror.compare(f.ctx, f.scene);
  const auto &plane = f.mirror.items.begin()->second.clipPlanes.items[0].planeWorld;
  CHECK(plane.x == doctest::Approx(1));
  CHECK(plane.y == doctest::Approx(0));
  CHECK(plane.z == doctest::Approx(0));
  CHECK(plane.w == doctest::Approx(-2));
  // A stable view must not continually force new baselines.
  f.ctx.tick(1.0);
  delta = f.ex.delta();
  CHECK(delta.added.empty()); CHECK(delta.removed.empty());
  f.mirror.apply(delta, f.ex);
  f.mirror.compare(f.ctx, f.scene);
  // axisOfRotation is a scalar edit on the scoped Billboard frame itself.
  replacement(f.event(billboard, "axisOfRotation", SFVec3f{1, 0, 0}), 1, 1);
}

TEST_CASE("render state delta: empty shared Billboard clip scopes retain both tracked frames") {
  auto viewpoint = createX3DNode("Viewpoint");
  set(viewpoint, "position", SFVec3f{0, 0, 10});
  auto clip = createX3DNode("ClipPlane");
  set(clip, "plane", SFVec4f{0, 0, 1, -2});
  set(clip, "enabled", false);
  auto coord = createX3DNode("Coordinate");
  auto geometry = createX3DNode("TriangleSet");
  set(geometry, "coord", coord);
  auto billboard = group({clip, shape(geometry)}, "Billboard");
  auto left = group({billboard}), right = group({billboard});
  set(left, "translation", SFVec3f{-5, 0, 0});
  set(right, "translation", SFVec3f{5, 0, 0});
  Fixture f({viewpoint, left, right});
  REQUIRE(f.mirror.items.empty());
  replacement(f.event(clip, "enabled", true), 0, 0);
  f.ctx.setHeadPose({10, 0, -10}, {0, 1, 0, 0});
  f.ctx.tick(1.0);
  auto delta = f.ex.delta();
  replacement(delta, 0, 0);
  f.mirror.apply(delta, f.ex);
  f.mirror.compare(f.ctx, f.scene);
  delta = f.event(coord, "point", MFVec3f{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}});
  REQUIRE(delta.added.size() == 2);
  for (const auto &[id, item] : f.mirror.items) {
    (void)id;
    REQUIRE(item.clipPlanes.size == 1);
    const auto &plane = item.clipPlanes.items[0].planeWorld;
    CHECK(plane.x == doctest::Approx(1));
    CHECK(plane.y == doctest::Approx(0));
    CHECK(plane.z == doctest::Approx(0));
    CHECK(plane.w == doctest::Approx(item.path.front() == left.get() ? 3 : -7));
  }
}

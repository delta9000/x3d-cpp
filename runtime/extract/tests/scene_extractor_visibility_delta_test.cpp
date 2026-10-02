// #155: optional origin-distance hints must travel through advertised channels.
#include "SceneExtractor.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"

#include <algorithm>
#include <any>
#include <cmath>
#include <map>
#include <memory>
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
  auto node = createX3DNode("Shape");
  set(node, "geometry", geometry);
  return node;
}
Node frame(std::vector<Node> children, SFVec3f translation = {}) {
  auto node = createX3DNode("Transform");
  set(node, "children", std::move(children));
  set(node, "translation", translation);
  return node;
}
Node viewpoint(float far = 20) {
  auto node = createX3DNode("Viewpoint");
  set(node, "position", SFVec3f{0, 0, 10});
  set(node, "farDistance", far);
  return node;
}
struct Fixture {
  Scene scene;
  X3DExecutionContext ctx;
  MeshBuildOptions options;
  SceneExtractor ex;
  std::map<RenderItemId, RenderItem> mirror;
  explicit Fixture(std::vector<Node> roots, MeshBuildOptions opts = {})
      : options(std::move(opts)), ex(ctx, scene, options) {
    scene.rootNodes = std::move(roots);
    ctx.buildSceneGraph(scene);
    apply(ex.fullSnapshot());
  }
  void apply(const RenderDelta &delta) {
    for (auto id : delta.removed) REQUIRE(mirror.erase(id) == 1);
    for (auto id : delta.added) REQUIRE(mirror.emplace(id, ex.item(id)).second);
    // Copy ONLY each advertised channel. A complete-record copy on an update
    // would conceal the stale host hint even if the extractor changed it.
    for (auto id : delta.updatedTransform) mirror.at(id).worldTransform = ex.item(id).worldTransform;
    for (auto id : delta.updatedGeometry) {
      mirror.at(id).geometry = ex.item(id).geometry;
      mirror.at(id).mesh = ex.item(id).mesh;
      mirror.at(id).geometry_ext = ex.item(id).geometry_ext;
    }
    for (auto id : delta.updatedMaterial) mirror.at(id).material = ex.item(id).material;
    SceneExtractor oracle(ctx, scene, options);
    auto snapshot = oracle.fullSnapshot();
    REQUIRE(snapshot.added.size() == mirror.size());
    for (auto id : snapshot.added) {
      const auto &expected = oracle.item(id);
      const auto found = std::find_if(mirror.begin(), mirror.end(), [&](const auto &item) {
        return item.second.path == expected.path;
      });
      REQUIRE(found != mirror.end());
      CHECK(found->second.beyondVisibilityLimit == expected.beyondVisibilityLimit);
    }
  }
  RenderDelta tick() {
    ctx.tick(1.0); // repeated timestamps: generation, not wall time, is the guard
    auto delta = ex.delta();
    apply(delta);
    if (!delta.removed.empty() && !delta.added.empty()) {
      // Every crossing settles: consuming an unchanged following tick must not
      // repeatedly replace the new baseline, for AoS and packed items alike.
      ctx.tick(1.0);
      const auto stable = ex.delta();
      CHECK(stable.removed.empty());
      CHECK(stable.added.empty());
      apply(stable);
    }
    return delta;
  }
  RenderDelta event(const Node &node, const char *field, std::any value) {
    ctx.postEvent(node.get(), field, std::move(value));
    return tick();
  }
  const RenderItem &under(const Node &ancestor) const {
    for (const auto &[id, item] : mirror) {
      (void)id;
      if (std::find(item.path.begin(), item.path.end(), ancestor.get()) != item.path.end())
        return item;
    }
    FAIL("missing placement");
    return mirror.begin()->second;
  }
};
void replacement(const RenderDelta &delta, std::size_t count) {
  CHECK(delta.removed.size() == count);
  CHECK(delta.added.size() == count);
  CHECK(delta.updatedTransform.empty());
  CHECK(delta.updatedGeometry.empty());
  CHECK(delta.updatedMaterial.empty());
}
void retained(const RenderDelta &delta) {
  CHECK(delta.removed.empty());
  CHECK(delta.added.empty());
}
} // namespace

TEST_CASE("visibility hint delta: farDistance precedence fallback and disabled limits") {
  auto vp = viewpoint();
  auto nav = createX3DNode("NavigationInfo");
  set(nav, "visibilityLimit", 4.0f);
  auto box = shape();
  Fixture f({vp, nav, box});
  CHECK_FALSE(f.under(box).beyondVisibilityLimit); // origin distance 10 < far 20
  replacement(f.event(vp, "farDistance", 5.0f), 1);
  CHECK(f.under(box).beyondVisibilityLimit);
  auto mesh = f.under(box).mesh;
  retained(f.event(nav, "visibilityLimit", 100.0f)); // farDistance wins
  CHECK(f.under(box).beyondVisibilityLimit);
  CHECK(f.under(box).mesh == mesh);
  replacement(f.event(vp, "farDistance", -1.0f), 1); // fallback to nav 100
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  replacement(f.event(nav, "visibilityLimit", 5.0f), 1);
  CHECK(f.under(box).beyondVisibilityLimit);
  replacement(f.event(nav, "visibilityLimit", 0.0f), 1); // finite -> unlimited
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  retained(f.event(vp, "farDistance", 10.0f)); // equality is not beyond
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  replacement(f.event(vp, "farDistance", 9.0f), 1);
  CHECK(f.under(box).beyondVisibilityLimit);
  replacement(f.event(vp, "farDistance", 10.0f), 1);
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  retained(f.ex.delta()); // same generation stays empty
}

TEST_CASE("visibility hint delta: binding changes re-evaluate effective limits") {
  auto vp = viewpoint(-1);
  auto close = viewpoint(5);
  auto nav = createX3DNode("NavigationInfo");
  auto otherNav = createX3DNode("NavigationInfo");
  set(nav, "visibilityLimit", 20.0f);
  set(otherNav, "visibilityLimit", 5.0f);
  auto box = shape();
  Fixture f({vp, close, nav, otherNav, box});
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  replacement(f.event(otherNav, "set_bind", true), 1);
  CHECK(f.under(box).beyondVisibilityLimit);
  replacement(f.event(nav, "set_bind", true), 1);
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  replacement(f.event(close, "set_bind", true), 1);
  CHECK(f.under(box).beyondVisibilityLimit);
  replacement(f.event(vp, "set_bind", true), 1);
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
}

TEST_CASE("visibility hint delta: per-placement ancestor translation scale and rotation") {
  auto shared = shape();
  auto child = frame({shared}, {0, 0, 15});
  auto left = frame({child});
  auto right = frame({child});
  auto unrelated = frame({shape()});
  Fixture f({viewpoint(), left, right, unrelated});
  CHECK_FALSE(f.under(left).beyondVisibilityLimit); // distance 5
  auto mesh = f.under(unrelated).mesh;
  retained(f.event(left, "translation", SFVec3f{1, 0, 0})); // sqrt(26) < 20
  CHECK(f.under(unrelated).mesh == mesh);
  CHECK(f.under(left).mesh == f.under(right).mesh);
  replacement(f.event(left, "translation", SFVec3f{30, 0, 0}), 3);
  CHECK(f.under(left).beyondVisibilityLimit);
  CHECK_FALSE(f.under(right).beyondVisibilityLimit);
  replacement(f.event(left, "translation", SFVec3f{0, 0, 0}), 3);
  replacement(f.event(left, "scale", SFVec3f{3, 3, 3}), 3); // origin z45, distance35
  CHECK(f.under(left).beyondVisibilityLimit);
  replacement(f.event(left, "scale", SFVec3f{1, 1, 1}), 3);
  replacement(f.event(left, "rotation", SFRotation{0, 1, 0, 3.14159265f}), 3);
  CHECK(f.under(left).beyondVisibilityLimit); // origin z-15, distance25
  replacement(f.event(left, "rotation", SFRotation{0, 1, 0, 0}), 3);
  CHECK_FALSE(f.under(left).beyondVisibilityLimit);
}

TEST_CASE("visibility hint delta: tracked eye and viewpoint motion only replace at crossings") {
  auto vp = viewpoint();
  auto box = shape();
  Fixture f({vp, box});
  auto mesh = f.under(box).mesh;
  f.ctx.setHeadPose({1, 0, 0}, {0, 1, 0, 0});
  retained(f.tick());
  CHECK(f.under(box).mesh == mesh);
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  f.ctx.setHeadPose({30, 0, 0}, {0, 1, 0, 0});
  replacement(f.tick(), 1);
  CHECK(f.under(box).beyondVisibilityLimit);
  f.ctx.setHeadPose({0, 0, 0}, {0, 1, 0, 0});
  replacement(f.tick(), 1);
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  replacement(f.event(vp, "position", SFVec3f{0, 0, 30}), 1);
  CHECK(f.under(box).beyondVisibilityLimit);
  replacement(f.event(vp, "position", SFVec3f{0, 0, 10}), 1);
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
  mesh = f.under(box).mesh;
  for (int i = 0; i < 20; ++i) {
    f.ctx.setHeadPose({float(i % 2), 0, 0}, {0, 1, 0, 0});
    retained(f.tick());
    CHECK(f.under(box).mesh == mesh);
  }
}

TEST_CASE("visibility hint delta: unlimited scenes retain meshes during eye and TRS changes") {
  auto vp = viewpoint(-1);
  auto box = shape();
  auto parent = frame({box});
  Fixture f({vp, parent});
  auto mesh = f.under(box).mesh;
  for (int i = 0; i < 20; ++i) {
    f.ctx.setHeadPose({float(i * 100), 0, 0}, {0, 1, 0, 0});
    retained(f.event(parent, "translation", SFVec3f{0, float(i * 100), 0}));
    CHECK_FALSE(f.under(box).beyondVisibilityLimit);
    CHECK(f.under(box).mesh == mesh);
  }
}

TEST_CASE("visibility hint delta: initial empty and dormant placements publish current hints on activation") {
  auto vp = viewpoint();
  auto coord = createX3DNode("Coordinate");
  auto geometry = createX3DNode("TriangleSet");
  set(geometry, "coord", coord);
  auto item = shape(geometry);
  auto parent = frame({item});
  auto unrelated = shape();
  Fixture f({vp, parent, unrelated});
  auto mesh = f.under(unrelated).mesh;
  retained(f.event(parent, "translation", SFVec3f{30, 0, 0}));
  const MFVec3f triangle{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  auto delta = f.event(coord, "point", triangle);
  CHECK(delta.added.size() == 1);
  CHECK(delta.removed.empty());
  CHECK(f.under(item).beyondVisibilityLimit);
  CHECK(f.under(unrelated).mesh == mesh);
  delta = f.event(coord, "point", MFVec3f{});
  CHECK(delta.removed.size() == 1);
  retained(f.event(parent, "translation", SFVec3f{0, 0, 0}));
  delta = f.event(coord, "point", triangle);
  CHECK(delta.added.size() == 1);
  CHECK(delta.removed.empty());
  CHECK_FALSE(f.under(item).beyondVisibilityLimit);
  CHECK((f.under(item).worldTransform.transformPoint({0, 0, 0}) == SFVec3f{0, 0, 0}));
  CHECK(f.under(unrelated).mesh == mesh);
  f.event(coord, "point", MFVec3f{});
  retained(f.event(vp, "farDistance", 12.0f)); // unrelated origin stays within limit
  retained(f.event(parent, "translation", SFVec3f{30, 0, 0}));
  delta = f.event(coord, "point", triangle);
  CHECK(delta.added.size() == 1);
  CHECK(f.under(item).beyondVisibilityLimit);
  CHECK(f.under(unrelated).mesh == mesh);
  f.event(coord, "point", MFVec3f{});
  retained(f.event(vp, "farDistance", 20.0f));
  f.ctx.setHeadPose({15, 0, 0}, {0, 1, 0, 0});
  retained(f.tick()); // both old live and future revived origins are <20 away
  delta = f.event(coord, "point", triangle);
  CHECK(delta.added.size() == 1);
  CHECK_FALSE(f.under(item).beyondVisibilityLimit);
  CHECK(f.under(unrelated).mesh == mesh);

}

TEST_CASE("visibility hint delta: packed geometry uses the same origin rule without repeated replacements") {
  MeshBuildOptions options;
  options.externalGeometryResolver = [](const X3DNode *, AssetResolver) {
    PackedMesh mesh;
    VertexBufferView positions;
    positions.component_type = ComponentType::Float;
    positions.components_per_vertex = 3;
    positions.vertex_count = 3;
    mesh.set_attrib(VertexAttrib::Position, positions, std::vector<std::uint8_t>(36));
    mesh.vertex_count = 3;
    return mesh;
  };
  auto vp = viewpoint(5);
  auto packed = shape(createX3DNode("NurbsTrimmedSurface"));
  Fixture f({vp, packed}, options);
  REQUIRE(f.mirror.size() == 1);
  CHECK(f.under(packed).geometry_ext.is_packed());
  CHECK(f.under(packed).beyondVisibilityLimit); // origin distance10 > 5
  for (int i = 0; i < 3; ++i) retained(f.tick());
  replacement(f.event(vp, "farDistance", 20.0f), 1);
  CHECK_FALSE(f.under(packed).beyondVisibilityLimit);
  for (int i = 0; i < 3; ++i) retained(f.tick());
}

TEST_CASE("visibility hint delta: Billboard crossings use current child origins") {
  auto box = shape();
  auto child = frame({box}, {10, 0, 0});
  auto billboard = createX3DNode("Billboard");
  set(billboard, "children", std::vector<Node>{child});
  Fixture f({viewpoint(25), billboard});
  CHECK_FALSE(f.under(box).beyondVisibilityLimit); // sqrt(200) < 25
  f.ctx.setHeadPose({30, 0, 0}, {0, 1, 0, 0});
  replacement(f.tick(), 1);
  // A stale (10,0,0) origin would be only sqrt(500) away and incorrectly false.
  // The facing frame moves it to (sqrt(10),0,-3*sqrt(10)); distance sqrt(1100).
  CHECK(f.under(box).beyondVisibilityLimit);
  CHECK(f.under(box).worldTransform.m[12] == doctest::Approx(std::sqrt(10.0f)));
  CHECK(f.under(box).worldTransform.m[14] == doctest::Approx(-3 * std::sqrt(10.0f)));
  replacement(f.event(billboard, "axisOfRotation", SFVec3f{1, 0, 0}), 1);
  CHECK_FALSE(f.under(box).beyondVisibilityLimit); // X-only rotation leaves origin (10,0,0)
  replacement(f.event(billboard, "axisOfRotation", SFVec3f{0, 1, 0}), 1);
  CHECK(f.under(box).beyondVisibilityLimit);
  f.ctx.setHeadPose({0, 0, 0}, {0, 1, 0, 0});
  replacement(f.tick(), 1);
  CHECK_FALSE(f.under(box).beyondVisibilityLimit);
}
